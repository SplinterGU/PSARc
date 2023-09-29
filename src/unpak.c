/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file unpak.c
 * @brief Implementation of the unpak utility for the PSARc project.
 *
 * This file implements the unpak utility, which is used to extract files from PSARC archives
 * in the PSARc project. It provides functionality for unpacking archive contents.
 *
 * This file is part of the PSARc project.
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @author Juan José Ponteprino
 * @date September 2023
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <locale.h>
#include <libgen.h>
#include <zlib.h>
#include <lzma.h>
#include <unistd.h>

#include "common.h"
#include "psarc.h"
#include "md5.h"
#include "inettypes.h"
#include "hashset.h"
#include "file_utils.h"
#include "report.h"

static uint8_t *source_buffer = NULL;
static uint8_t *target_buffer = NULL;
REPORT *report = NULL;

/**
 * Decompresses a file entry from the PSARC archive.
 *
 * This function reads and decompresses a portion of a PSARC archive file, up to a maximum
 * of 65536 bytes at a time. It manages the decompression process, including handling zlib
 * and LZMA compression methods.
 *
 * @param archive_file      The PSARC archive file.
 * @param output_file       The output file where the decompressed data is written (can be NULL).
 * @param output_buffer     The buffer for storing the decompressed data (if output_file is NULL).
 * @param fi                Information about the file entry being decompressed.
 * @param blocktable        The table containing block sizes for the PSARC archive.
 *
 * @return                  0 on success, 1 on error.
 */

static int decompress_entry(FILE *archive_file, FILE *output_file, unsigned char *output_buffer, FILEINFO *fi, uint32_t *blocktable) {
    // Get the open block for this file
    uint32_t open_block = fi->block_index;
    int64_t remaining_size = fi->uncompressed_size;

    fseek(archive_file, fi->offset, SEEK_SET);

    int64_t chunk_size = _ArchiveInfo.block_size;

    while (remaining_size > 0) {
        // Get the size of the current block
        uint32_t block_size = blocktable[open_block];

        // block_size == 0 => is chunk_size
        if (!block_size) block_size = chunk_size;

        size_t bytes_read = fread(source_buffer, block_size, 1, archive_file) * block_size;
        if (bytes_read != block_size) {
            // Error reading compressed data
            return 1;
        }

        if (remaining_size < chunk_size)
            chunk_size = remaining_size;

        /*
        78 01   No Compression (no preset dictionary)
        78 5E   Best speed (no preset dictionary)
        78 9C   Default Compression (no preset dictionary)
        78 DA   Best Compression (no preset dictionary)
        78 20   No Compression (with preset dictionary)
        78 7D   Best speed (with preset dictionary)
        78 BB   Default Compression (with preset dictionary)
        78 F9   Best Compression (with preset dictionary)
        */

        // Check if it's a valid zlib block
        if (bytes_read > 2 && source_buffer[0] == 0x78 &&
            (source_buffer[1] == 0x01 || source_buffer[1] == 0x5E || source_buffer[1] == 0x9C || source_buffer[1] == 0xDA)) {
            // Decompress only if it's a valid zlib block
            uLongf dest_len = chunk_size;
            if (!output_file) {
                int result = uncompress(output_buffer + (fi->uncompressed_size - remaining_size), &dest_len, source_buffer, block_size);
                if (result != Z_OK) {
                    // Error decompressing data
                    return 1;
                }
            } else {
                int result = uncompress(target_buffer, &dest_len, source_buffer, block_size);
                if (result != Z_OK) {
                    // Error decompressing data
                    return 1;
                }
                fwrite(target_buffer, 1, dest_len, output_file);
            }
        }
        // Check if it's a valid LZMA block
        else if (bytes_read > 6 && memcmp(source_buffer, "\xFD\x37\x7A\x58\x5A\x00", 6) == 0) {
            // Decompress only if it's a valid LZMA block
            size_t dest_len = chunk_size;
            lzma_stream strm = LZMA_STREAM_INIT;

            if (lzma_stream_decoder(&strm, UINT64_MAX, 0) != LZMA_OK) {
                // Error initializing LZMA stream
                return 1;
            }

            strm.next_in = source_buffer;
            strm.avail_in = bytes_read;
            strm.avail_out = dest_len;

            if (!output_file) {
                strm.next_out = output_buffer + (fi->uncompressed_size - remaining_size);
            } else {
                strm.next_out = target_buffer;
            }

            lzma_ret ret;
            if ((ret = lzma_code(&strm, LZMA_FINISH)) != LZMA_STREAM_END) {
                // Error decoding LZMA data
                lzma_end(&strm);
                return 1;
            }
            lzma_end(&strm);
            if (output_file) fwrite(target_buffer, 1, dest_len, output_file);
        } else {
            // It's not a valid block, dump it as is
            if (!output_file) {
                memmove(output_buffer + (fi->uncompressed_size - remaining_size), source_buffer, bytes_read);
            } else {
                fwrite(source_buffer, 1, bytes_read, output_file);
            }
        }

        // Update remaining size and open block
        remaining_size -= chunk_size;
        open_block++;
    }

    return 0;
}

/**
 * Reads and parses the PSARC archive header.
 *
 * This function reads and interprets the header of a PSARC archive file. It extracts
 * information such as version, compression type, TOC length, and block size.
 *
 * @param archive_file      The PSARC archive file.
 *
 * @return                  0 on success, 1 on error.
 */

static int read_header(FILE *archive_file) {
    // Read the header of the compressed file
    PSARCHEADER header;

    fseek(archive_file, 0, SEEK_SET);

    size_t bytes_read = fread(&header, sizeof(header), 1, archive_file);
    if (bytes_read != 1) return 1;

    // Read the Table of Contents (TOC)
    uint16_t *pversion = (uint16_t *)&header.version;
    _ArchiveInfo.version.high = ntohs(*pversion);
    _ArchiveInfo.version.low = ntohs(*(pversion + 1));
    _ArchiveInfo.compression_type = !strncmp(header.compression_type, "lzma", 4) ? PSARC_LZMA : PSARC_ZLIB;
    _ArchiveInfo.toc_length = ntohl(header.toc_length);
    _ArchiveInfo.toc_entries = ntohl(header.toc_entries);
    _ArchiveInfo.block_size = htonl(header.block_size);
    _ArchiveInfo.archive_flags = htonl(header.archive_flags);

    return 0;
}

/**
 * Reads and stores the table of contents (TOC) from the PSARC archive.
 *
 * This function reads and stores the TOC entries from a PSARC archive file. It extracts
 * information about each file entry, including name digest, block index, uncompressed size,
 * and offset.
 *
 * @param archive_file      The PSARC archive file.
 *
 * @return                  An array of FILEINFO structures representing TOC entries.
 */

static FILEINFO *read_toc_table(FILE *archive_file) {
    FILEINFO *files_info_table = (FILEINFO *)calloc(_ArchiveInfo.toc_entries, sizeof(FILEINFO));
    if (!files_info_table) return NULL;

    fseek(archive_file, sizeof(PSARCHEADER), SEEK_SET);

    for (uint32_t i = 0; i < _ArchiveInfo.toc_entries; i++) {
        PSARCTOC toc;
        size_t bytes_read = fread(&toc, sizeof(toc), 1, archive_file);
        bytes_read = bytes_read; // Remove warning
        memmove(files_info_table[i].name_digest, toc.name_digest, sizeof(files_info_table[i].name_digest));
        files_info_table[i].block_index = ntohl(toc.block_offset);
        files_info_table[i].uncompressed_size = ntoh40((const uint8_t *)toc.uncompressed_size);
        files_info_table[i].offset = ntoh40((const uint8_t *)toc.file_offset);
    }

    return files_info_table;
}

/**
 * Reads and stores the block size table from the PSARC archive.
 *
 * This function reads and stores the block size table from a PSARC archive file. It determines
 * the size of each data block within the archive.
 *
 * @param archive_file      The PSARC archive file.
 *
 * @return                  An array of block sizes.
 */

static uint32_t *read_blocktable(FILE *archive_file) {
    int bsize = get_blocktable_item_size();

    // Calculate the number of blocks and the size of the block table
    size_t blocktable_size = (_ArchiveInfo.toc_length - (sizeof(PSARCHEADER) + _ArchiveInfo.toc_entries * sizeof(PSARCTOC))) / bsize;

    // Read the block table
    uint32_t *blocktable = (uint32_t *)malloc(blocktable_size * sizeof(uint32_t));
    if (!blocktable) return NULL;

    uint32_t blk;
    for (uint32_t i = 0; i < blocktable_size; i++) {
        if (!fread(&blk, bsize, 1, archive_file)) {
            free(blocktable);
            return NULL;
        }
        switch (bsize) {
            case 1:
                blocktable[i] = (uint32_t)blk;
                break;
            case 2:
                blocktable[i] = (uint32_t)ntohs((uint16_t)blk);
                break;
            case 3:
                blocktable[i] = ntoh24((uint8_t *)&blk);
                break;
            case 4:
                blocktable[i] = ntohl(blk);
                break;
        }
    }

    return blocktable;
}

/**
 * Reads and associates filenames with file information from the PSARC archive.
 *
 * This function reads the filenames from the PSARC archive and associates them with their
 * corresponding file information. It decompresses the filenames and fills the FILEINFO
 * structures.
 *
 * @param archive_file      The PSARC archive file.
 * @param files_info_table  An array of FILEINFO structures.
 * @param blocktable        The table containing block sizes for the PSARC archive.
 *
 * @return                  0 on success, 1 on error.
 */

static int read_filenames(FILE *archive_file, FILEINFO *files_info_table, uint32_t *blocktable) {
    char *names = malloc(files_info_table[0].uncompressed_size + 1);
    if (decompress_entry(archive_file, NULL, (unsigned char *)names, &files_info_table[0], blocktable) != 0) return 1;

    names[files_info_table[0].uncompressed_size] = '\0';

    const char *delimiter = "\x0a"; // Newline as delimiter
    char *token = strtok(names, delimiter); // Get the first name

    // Fill the FILEINFO structure with file names
    for (uint32_t i = 1; i < _ArchiveInfo.toc_entries && token != NULL; i++) {
        files_info_table[i].filename = strdup(token); // Copy the file name
        if (!files_info_table[i].filename) {
            // If memory fail, free allocated filenames
            for( --i; i > 0; i--){
                free(files_info_table[i].filename);
                files_info_table[i].filename = NULL;
            }
            free(names);
            return 1;
        }
        token = strtok(NULL, delimiter); // Get the next name
    }

    free(names);

    return 0;
}

/**
 * Decompresses files from the PSARC archive and writes them to the output directory.
 *
 * This function decompresses files from a PSARC archive and writes them to the output directory.
 * It creates subdirectories as needed and handles the decompression process.
 *
 * @param archive_file          The PSARC archive file.
 * @param files_info_table      An array of FILEINFO structures.
 * @param blocktable            The table containing block sizes for the PSARC archive.
 * @param files                 Array of input file paths.
 * @param num_files             Number of input files.
 *
 * @return                      0 if successful
 *                              1 on error.
 *                              2 on error with output format
 */

static uint64_t _total_bytes = 0LL;
static uint64_t _errors = 0LL;
static uint64_t _successful = 0LL;

static int decompress_files(FILE *archive_file, FILEINFO *files_info_table, uint32_t *blocktable, char **files, size_t num_files) {
    HASHSET *hset = NULL;

    size_t files_count = 0;

    if ( num_files ) {
        hset = hashset_init(65536);
        for ( size_t i = 0; i < num_files; i++ ) {
            if ( _ArchiveInfo.archive_flags & AF_ICASE ) {
                char * f = lcase(files[i]);
                if ( !f ) {
                    fprintf( stderr, APPNAME": not enough memory\n");
                    hashset_free(hset);
                    return 1;
                }
                hashset_add(hset, f);
                files_count++;
                free(f);
            } else {
                hashset_add(hset, files[i]);
                files_count++;
            }
        }
    } else {
        files_count = _ArchiveInfo.toc_entries - 1;
    }

    report_open_file_section(report);

    // Create destination files and perform decompression
    for (uint32_t i = 1; i < _ArchiveInfo.toc_entries; i++) {
        if ( hset ) {
            if ( _ArchiveInfo.archive_flags & AF_ICASE ) {
                char * f = lcase(files_info_table[i].filename);
                if ( !f ) {
                    fprintf( stderr, APPNAME": not enough memory\n");
                    hashset_free(hset);
                    return 1;
                }
                int exists = hashset_contains(hset, f);
                free(f);
                if ( !exists ) continue;
            } else {
                if ( !hashset_contains(hset, files_info_table[i].filename) ) continue;
            }
            hashset_del(hset, files_info_table[i].filename);
        }

        char *outdir_copy = strdup(files_info_table[i].filename);
        char *outfile_copy = strdup(files_info_table[i].filename);
        if (!outdir_copy || !outfile_copy) {
            free(outdir_copy);
            free(outfile_copy);
            fprintf( stderr, APPNAME": not enough memory\n");
            if ( hset ) hashset_free( hset );
            return 1;
        }

        char *outdir = dirname(outdir_copy);
        char *outfile = basename(outfile_copy);

        if (*outdir == '/' || ( *outdir == '.' && !*(outdir+1) ) ) outdir++;

        char *filepath = malloc(strlen(outdir) + strlen(outfile) + 2);
        if (!filepath) {
            fprintf( stderr, APPNAME": not enough memory\n");
            free(outfile_copy);
            free(outdir_copy);
            if ( hset ) hashset_free( hset );
            return 1;
        }
        sprintf(filepath, "%s%s%s", outdir, *outdir ? "/" : "", outfile);
        free(outfile_copy);

        char *filepath_for_open = NULL;

        if ( _Config.trim_path_flag ) {
            filepath_for_open = strrchr(filepath, '/');
            if ( filepath_for_open ) {
                filepath_for_open++;
            } else {
                filepath_for_open = filepath;
            }
        } else {
            mkpath(outdir, 0777);
            filepath_for_open = filepath;
        }

        free(outdir_copy);

        report_open_file_item(report, &files_info_table[i]);

        int fileexists = access(filepath_for_open, F_OK) == 0;

        files_count--;

        if (fileexists && !_Config.overwrite_flag) {
             if ( _Config.skip_existing_files_flag ) {
                report_close_file_item(report, 0, 0, "skipped (file exists)", files_count);
                _total_bytes += files_info_table[i].uncompressed_size;
                _successful++;
                free(filepath);
                continue;
            } else {
                report_close_file_item(report, 0, 0, "fail (file already exists)", files_count);
                _errors++; 
                free(filepath);
                continue;
            }
        }

        FILE *output_file = fopen(filepath_for_open, "wb");

        free(filepath);

        if (output_file) {
            if (decompress_entry(archive_file, output_file, NULL, &files_info_table[i], blocktable) != 0) {
                report_close_file_item(report, 0, 0, "fail", files_count);
                _errors++;
            } else {
                report_close_file_item(report, 0, 0, "ok", files_count);
                _total_bytes += files_info_table[i].uncompressed_size;
                _successful++;
            }
            fclose(output_file);
        } else {
            report_close_file_item(report, 0, 0, "fail", files_count);
            _errors++;
        }
    }

    if ( hset ) {
#ifdef SHOW_FILES_NOT_FOUND
        for ( size_t i = 0; i < num_files; i++ ) {
            if ( hashset_contains( hset, files[i] ) ) {
                printf("error: '%s' not found\n", files[i]);
            }
        }
#endif
        hashset_free( hset );
    }

    report_close_file_section(report);

    return _errors > 0 ? 2 : 0;
}

/**
 * Lists the files in the PSARC archive along with compression ratios.
 *
 * This function lists the files contained in the PSARC archive and provides information about
 * their compressed and uncompressed sizes, as well as compression ratios.
 *
 * @param files_info_table   An array of FILEINFO structures.
 * @param blocktable         The table containing block sizes for the PSARC archive.
 */

static void list_archive_files(FILEINFO *files_info_table, uint32_t *blocktable) {
    report_open_file_section(report);
    for (uint32_t i = 1; i < _ArchiveInfo.toc_entries; i++) {
        files_info_table[i].compressed_size = get_compressed_size(&files_info_table[i], blocktable);
        report_open_file_item(report, &files_info_table[i]);
        report_close_file_item(report, 0, 0, NULL, i < _ArchiveInfo.toc_entries - 1);
    }
    report_close_file_section(report);
}

/**
 * Processes the PSARC archive specified by the input file.
 *
 * This function handles the overall processing of a PSARC archive. It manages the reading of
 * the header, TOC, block sizes, filenames, and performs actions based on the specified mode.
 *
 * @param input_file         The input PSARC archive file.
 * @param mode               The processing mode (1 for compression, 2 for decompression,
 *                           3 for listing, 4 for detailed info).
 * @param files              Array names of files to process
 * @param num_files          Number of files in array
 *
 * @return                   0 if successful
 *                           1 on error.
 *                           2 on error with output format
 */

int process_archive(char *input_file, int mode, char **files, size_t num_files) {
    int ret = 1;

    // Open the compressed file
    FILE *archive_file = fopen(input_file, "rb");
    if (!archive_file) {
        fprintf( stderr, APPNAME": error opening archive %s\n", _Config.archive_file );
        return 1;
    }

    source_buffer = malloc(_ArchiveInfo.block_size * 2);
    if (!source_buffer) {
        fprintf( stderr, APPNAME": not enough memory\n");
        fclose(archive_file);
        return 1;
    }

    target_buffer = malloc(_ArchiveInfo.block_size * 2);
    if (!target_buffer) {
        fprintf( stderr, APPNAME": not enough memory\n" );
        free(source_buffer);
        return 1;
    }

    // Read the PSARC header
    if (read_header(archive_file) != 0) {
        fprintf( stderr, APPNAME": error reading header from archive\n" );
        free(source_buffer);
        free(target_buffer);
        fclose(archive_file);
        return 1;
    }

    // Read the Table of Contents (TOC)
    FILEINFO *files_info_table = read_toc_table(archive_file);
    if (files_info_table == NULL) {
        fprintf( stderr, APPNAME": error reading files info\n" );
        free(source_buffer);
        free(target_buffer);
        fclose(archive_file);
        return 1;
    }

    // Read the block table
    uint32_t *blocktable = read_blocktable(archive_file);
    if (blocktable == NULL) {
        fprintf( stderr, APPNAME": error reading block size table\n" );
        free(source_buffer);
        free(target_buffer);
        free(files_info_table);
        fclose(archive_file);
        return 1;
    }

    // Read file names
    if (read_filenames(archive_file, files_info_table, blocktable) != 0) {
        fprintf( stderr, APPNAME": error reading filenames\n" );
        free(source_buffer);
        free(target_buffer);
        free(files_info_table);
        free(blocktable);
        fclose(archive_file);
        return 1;
    }

    switch (mode) {
        case 2:
            report = report_open(REPORT_TYPE_UNPAK, _Config.archive_file);
            if ( !report ) {
                fprintf( stderr, APPNAME": not enough memory\n");
                free(source_buffer);
                free(target_buffer);
                free(files_info_table);
                free(blocktable);
                fclose(archive_file);
                return 1;
            }
            ret = decompress_files(archive_file, files_info_table, blocktable, files, num_files);
            report_close(report, (_successful + _errors) > 0, 0, _total_bytes, 0, 0, _successful, _errors);
            break;

        case 3:
            report = report_open(REPORT_TYPE_LIST, _Config.archive_file);
            if ( !report ) {
                fprintf( stderr, APPNAME": not enough memory\n");
                free(source_buffer);
                free(target_buffer);
                free(files_info_table);
                free(blocktable);
                fclose(archive_file);
                return 1;
            }
            list_archive_files(files_info_table, blocktable);
            report_close(report, 1, 0, 0, 0, 0, _ArchiveInfo.toc_entries - 1, 0);
            ret = 0;
            break;

        case 4:
            show_info( _Config.archive_file, files_info_table, blocktable);
            ret = 0;
            break;
    }

    // Free resources
    for (uint32_t i = 0; i < _ArchiveInfo.toc_entries; i++)
        free(files_info_table[i].filename);

    free(source_buffer);
    free(target_buffer);

    free(files_info_table);
    free(blocktable);

    fclose(archive_file);

    return ret;
}
