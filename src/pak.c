/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file pak.c
 * @brief Implementation of PSARC file compression for the PSARc project.
 *
 * This file contains the implementation of functions responsible for compressing files into the
 * PSARC format used in the PSARc project. It provides the core functionality for creating compressed
 * PSARC archives.
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
#include <unistd.h>
#include <zlib.h>
#include <lzma.h>
#include <sys/stat.h>

#include "common.h"
#include "psarc.h"
#include "md5.h"
#include "inettypes.h"
#include "file_utils.h"
#include "report.h"
#include "threads.h"

static uint8_t *source_buffer = NULL;
static uint8_t *target_buffer = NULL;
static REPORT * report = NULL;

typedef struct {
    int is_first_block;
    int is_last_block;
    int is_not_last_file;
    size_t data_size;
    FILE *fp;
    FILEINFO *fi;
    size_t *total_size;
    uint32_t *blocktable;
    uint32_t blocktable_idx;
    // allocate buffers data from here
    uint8_t buffers;
} PAKDATA;

void *compress_entry_thread(void *arg) {
    THREADS_INFO *ti = (THREADS_INFO *) arg;
    PAKDATA *pkd = (PAKDATA *)THREAD_GET_USER_DATA(ti);

    uint8_t *buffers[2] = {
            &pkd->buffers,
            &pkd->buffers + _ArchiveInfo.block_size * 2
        };

    size_t bytes_write;

    uint8_t *write_buffer = buffers[1];

    // Compress based on the compression type
    switch (_ArchiveInfo.compression_type) {
        case PSARC_ZLIB: {
//            uLongf dest_len = compressBound(pkd->data_size);
            uLongf dest_len = _ArchiveInfo.block_size * 2;

            compress2(write_buffer, &dest_len, buffers[0], pkd->data_size, _Config.compression_level);
            bytes_write = dest_len;

            // if compressed is > that plain data, then save plain data
            if (bytes_write >= pkd->data_size) {
                bytes_write = pkd->data_size;
                write_buffer = buffers[0];
            }
            break;
        }

        case PSARC_LZMA: {
            lzma_ret ret;

            uint64_t dest_len = _ArchiveInfo.block_size * 2;

            // Structure for configuring compression options
            lzma_options_lzma lzma_options;
            lzma_lzma_preset(&lzma_options, _Config.compression_level | ( _Config.extreme_compression_flag ? LZMA_PRESET_EXTREME : 0 ));

            // Structure for configuring compression filter
            lzma_filter filters[] = {
//                    { .id = LZMA_FILTER_X86, .options = NULL },
//                    { .id = LZMA_FILTER_ARM, .options = NULL },
//                    { .id = LZMA_FILTER_ARM64, .options = NULL },
//                    { .id = LZMA_FILTER_ARMTHUMB, .options = NULL },
//                    { .id = LZMA_FILTER_POWERPC, .options = NULL },
//                    { .id = LZMA_FILTER_POWERPC, .options = NULL },
//                    { .id = LZMA_FILTER_IA64, .options = NULL },
//                    { .id = LZMA_FILTER_SPARC, .options = NULL },
//                    { .id = LZMA_FILTER_DELTA, .options = NULL },
                { .id = LZMA_FILTER_LZMA2, .options = &lzma_options },
                { .id = LZMA_VLI_UNKNOWN, .options = NULL },
            };

            // Structure for compression
            lzma_stream strm = LZMA_STREAM_INIT;

            // Initialize the compression object
            if ((ret = lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC64)) == LZMA_OK) {
                strm.next_in = buffers[0];
                strm.avail_in = pkd->data_size;
                strm.next_out = write_buffer;
                strm.avail_out = dest_len;

                if ((ret = lzma_code(&strm, LZMA_FINISH)) == LZMA_STREAM_END || ret == LZMA_OK) {
                    bytes_write = dest_len - strm.avail_out;
                    // if compressed is >= that plain data, then save plain data
                    if (bytes_write >= pkd->data_size) {
                        bytes_write = pkd->data_size;
                        write_buffer = buffers[0];
                    }
                } else {
                    // Error, save block without compression
                    bytes_write = pkd->data_size;
                    write_buffer = buffers[0];
                }

                lzma_end(&strm);
            } else {
                // Error, save block without compression
                bytes_write = pkd->data_size;
                write_buffer = buffers[0];
            }

            break;
        }

        default:
        case PSARC_STORE: {
            // Store compression type, simply copy the input to the output
            bytes_write = pkd->data_size;
            write_buffer = buffers[0];
            break;
        }
    }

    threads_wait_for_orderer_continue(ti);

    fwrite(write_buffer, bytes_write, 1, pkd->fp);

    if ( pkd->is_first_block ) {
        report_open_file_item(report, pkd->fi);
        pkd->fi->block_index = pkd->blocktable_idx;
        pkd->fi->offset = *pkd->total_size;
        pkd->fi->compressed_size = bytes_write;
    } else {
        pkd->fi->compressed_size += bytes_write;
    }

    *(pkd->total_size) += bytes_write;
    pkd->blocktable[pkd->blocktable_idx] = bytes_write;

    if ( pkd->is_last_block ) {
        report_close_file_item(report, pkd->fi->uncompressed_size, pkd->fi->compressed_size, NULL, pkd->is_not_last_file);
    }

    threads_completed(ti);

    return NULL;
}

/**
 * Compresses an entry based on the specified compression type.
 *
 * @param input_mem          Pointer to the input memory buffer (for in-memory data).
 * @param input_size         Size of the input data.
 * @param input_fp           File pointer to the input file (if reading from a file).
 * @param archive_file       File pointer to the archive output file.
 * @param fi                 Information about the compressed file (output).
 * @param total_size         Total size of compressed data processed so far (output).
 * @param blocktable         Table of compressed block sizes (output).
 * @param blocktable_idx     Index for the next entry in the block table (output).
 *
 * @return                   0 if successful, 1 on error.
 */

static int compress_entry_multi(FILE *input_fp, FILE *archive_file, FILEINFO *fi, size_t *total_size, uint32_t *blocktable, uint32_t *blocktable_idx, int is_not_last_file) {
    uint64_t to_read;
    size_t bytes_uncompressed = 0;

    if ( fi->uncompressed_size ) {
        uint32_t blocks = ( fi->uncompressed_size + _ArchiveInfo.block_size - 1 ) / _ArchiveInfo.block_size;
        uint32_t first_block = 1;

        while(blocks) {
            if ( blocks > 1 ) {
                to_read = _ArchiveInfo.block_size;
            } else {
                to_read = fi->uncompressed_size - bytes_uncompressed;
                if (to_read > _ArchiveInfo.block_size ) to_read = _ArchiveInfo.block_size;
            }

            PAKDATA *pkd;

            int slot = threads_get_free_slot( (void **) &pkd );

            uint8_t *buffers[2] = {
                    &pkd->buffers,
                    &pkd->buffers + _ArchiveInfo.block_size * 2
                };

            // Initialize the structure members
            pkd->is_first_block = first_block;
            pkd->is_last_block = blocks == 1;
            pkd->fp = archive_file;
            pkd->fi = fi;
            pkd->total_size = total_size;
            pkd->blocktable = blocktable;
            pkd->blocktable_idx = *blocktable_idx;
            pkd->is_not_last_file = is_not_last_file;

            pkd->data_size = fread(buffers[0], to_read, 1, input_fp) * to_read;

            threads_start_task( slot, compress_entry_thread, pkd );

            bytes_uncompressed += pkd->data_size;
            (*blocktable_idx)++;
            blocks--;
            first_block = 0;
        }
    } else {
        fi->compressed_size = 0;
    }

    return 0;
}

/**
 * Compresses an entry based on the specified compression type.
 *
 * @param input_mem          Pointer to the input memory buffer (for in-memory data).
 * @param input_size         Size of the input data.
 * @param input_fp           File pointer to the input file (if reading from a file).
 * @param archive_file       File pointer to the archive output file.
 * @param fi                 Information about the compressed file (output).
 * @param total_size         Total size of compressed data processed so far (output).
 * @param blocktable         Table of compressed block sizes (output).
 * @param blocktable_idx     Index for the next entry in the block table (output).
 *
 * @return                   0 if successful, 1 on error.
 */

static int compress_entry(const unsigned char *input_mem, size_t input_size, FILE *input_fp, FILE *archive_file, FILEINFO *fi, size_t *total_size, uint32_t *blocktable, uint32_t *blocktable_idx) {
    size_t bytes_read;
    size_t bytes_write;
    size_t bytes_compressed = 0;
    size_t bytes_uncompressed = 0;

    int64_t chunk_size = _ArchiveInfo.block_size;

    fi->offset = *total_size;
    fi->block_index = *blocktable_idx;

    uint32_t blocks = fi->num_blocks;
    uint64_t to_read;
    unsigned char *read_buffer;

    while (blocks) {
        uint8_t *write_buffer = target_buffer;

        if ( blocks > 1 ) {
            to_read = chunk_size;
        } else {
            to_read = fi->uncompressed_size - bytes_uncompressed;
            if (to_read > chunk_size ) to_read = chunk_size;
        }

        if ( input_fp ) {
            bytes_read = fread(source_buffer, to_read, 1, input_fp) * to_read;
            read_buffer = source_buffer;
        } else {
            bytes_read = to_read;
            read_buffer = &((unsigned char *)input_mem)[bytes_uncompressed];
        }

        // Compress based on the compression type
        switch (_ArchiveInfo.compression_type) {
            case PSARC_ZLIB: {
                uLongf dest_len = compressBound(bytes_read);

                compress2(write_buffer, &dest_len, read_buffer, bytes_read, _Config.compression_level);
                bytes_write = dest_len;

                // if compressed is > that plain data, then save plain data
                if (bytes_write >= bytes_read) {
                    bytes_write = bytes_read;
                    write_buffer = read_buffer;
                }
                break;
            }

            case PSARC_LZMA: {
                lzma_ret ret;

                uint64_t dest_len = chunk_size;

                // Structure for configuring compression options
                lzma_options_lzma lzma_options;
                lzma_lzma_preset(&lzma_options, _Config.compression_level | ( _Config.extreme_compression_flag ? LZMA_PRESET_EXTREME : 0 ));
    
                // Structure for configuring compression filter
                lzma_filter filters[] = {
//                    { .id = LZMA_FILTER_X86, .options = NULL },
//                    { .id = LZMA_FILTER_ARM, .options = NULL },
//                    { .id = LZMA_FILTER_ARM64, .options = NULL },
//                    { .id = LZMA_FILTER_ARMTHUMB, .options = NULL },
//                    { .id = LZMA_FILTER_POWERPC, .options = NULL },
//                    { .id = LZMA_FILTER_POWERPC, .options = NULL },
//                    { .id = LZMA_FILTER_IA64, .options = NULL },
//                    { .id = LZMA_FILTER_SPARC, .options = NULL },
//                    { .id = LZMA_FILTER_DELTA, .options = NULL },
                    { .id = LZMA_FILTER_LZMA2, .options = &lzma_options },
                    { .id = LZMA_VLI_UNKNOWN, .options = NULL },
                };

                // Structure for compression
                lzma_stream strm = LZMA_STREAM_INIT;

                // Initialize the compression object
                if ((ret = lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC64)) == LZMA_OK) {
                    strm.next_in = read_buffer;
                    strm.avail_in = bytes_read;
                    strm.next_out = write_buffer;
                    strm.avail_out = dest_len;

                    if ((ret = lzma_code(&strm, LZMA_FINISH)) == LZMA_STREAM_END || ret == LZMA_OK) {
                        bytes_write = dest_len - strm.avail_out;
                        // if compressed is >= that plain data, then save plain data
                        if (bytes_write >= bytes_read) {
                            bytes_write = bytes_read;
                            write_buffer = read_buffer;
                        }
                    } else {
                        // Error, save block without compression
                        bytes_write = bytes_read;
                        write_buffer = read_buffer;
                    }

                    lzma_end(&strm);
                } else {
                    // Error, save block without compression
                    bytes_write = bytes_read;
                    write_buffer = read_buffer;
                }
                lzma_end(&strm);
                break;
            }

            default:
            case PSARC_STORE: {
                // Store compression type, simply copy the input to the output
                bytes_write = bytes_read;
                write_buffer = read_buffer;
                break;
            }
        }

        fwrite(write_buffer, bytes_write, 1, archive_file);

        bytes_compressed += bytes_write;
        bytes_uncompressed += bytes_read;

        blocktable[*blocktable_idx] = bytes_write;
        (*blocktable_idx)++;

        blocks--;
    }

    *total_size += bytes_compressed;
    fi->compressed_size = bytes_compressed;

    return 0;
}

/**
 * Calculates the block table size for a list of files.
 *
 * This function takes an array of file paths and calculates the block table size
 * based on the file sizes and _ArchiveInfo.block_size.
 *
 * @param files             Array of file paths to process.
 * @param num_files         Number of files in the array.
 * @param fi                Array of TOC entries
 *
 * @return                  The block table size.
 */

static uint32_t get_blocktable_size(char **files, size_t num_files, FILEINFO * fi) {
    uint32_t block_off = 0;

    for (size_t i = 0; i < num_files; ++i) {
        struct stat file_stat;
        if ( stat(files[i], &file_stat) ) return 0;

        fi[i].uncompressed_size = (uint64_t) file_stat.st_size;
        fi[i].num_blocks = ( fi[i].uncompressed_size + _ArchiveInfo.block_size - 1 ) / _ArchiveInfo.block_size;

        // Calculate the next block_offset        
        block_off += fi[i].num_blocks;
    }

    return block_off;
}

/**
 * Writes the PSARC file header to the output file.
 *
 * @param output_file        File pointer to the PSARC output file.
 */

static void write_header(FILE *output_file) {
    // Create an instance of the PSARCHEADER structure and initialize its fields
    PSARCHEADER header;

    fseek(output_file, 0, SEEK_SET);

    strcpy(header.magic, "PSAR"); // 'PSAR'
    header.version = htonl(0x00010004); // v1.4
    if ( _ArchiveInfo.compression_type == PSARC_LZMA ) {
        strcpy(header.compression_type, "lzma"); // 'lzma'
    } else {
        strcpy(header.compression_type, "zlib"); // 'zlib'
    }
    header.toc_length = htonl(_ArchiveInfo.toc_length);
    header.toc_entry_size = htonl(0x1E);
    header.toc_entries = htonl(_ArchiveInfo.toc_entries);
    header.block_size = htonl(_ArchiveInfo.block_size);
    header.archive_flags = htonl(_ArchiveInfo.archive_flags);

    // Write the header uncompressed
    fwrite(&header, sizeof(header), 1, output_file);
}

/**
 * Writes the table of contents (TOC) entries to the output file.
 *
 * @param output_file        File pointer to the PSARC output file.
 * @param files_info_table         Array of FILEINFO structures containing TOC information.
 */

static void write_toc_table(FILE *output_file, FILEINFO *files_info_table) {
    PSARCTOC toc;

    fseek(output_file, sizeof(PSARCHEADER), SEEK_SET);

    for (int i = 0; i < _ArchiveInfo.toc_entries; i++) {
        // Write name_digest (16 bytes) uncompressed
        if (i) md5((uint8_t *)files_info_table[i].filename, strlen(files_info_table[i].filename), (uint8_t *)toc.name_digest);
        else   memset((uint8_t *)&toc.name_digest,'\0', sizeof(toc.name_digest));
        toc.block_offset = htonl(files_info_table[i].block_index);
        hton40((uint8_t *)&toc.uncompressed_size, files_info_table[i].uncompressed_size);
        hton40((uint8_t *)&toc.file_offset,(uint64_t) files_info_table[i].offset + _ArchiveInfo.toc_length);
        fwrite(&toc, sizeof(toc), 1, output_file);
    }
}

/**
 * Writes compressed block sizes to the output file.
 *
 * @param output_file        File pointer to the PSARC output file.
 * @param blocktable         Table of compressed block sizes.
 * @param blocktable_size    Number of entries in the block table.
 */

static void write_blocktable(FILE *output_file, uint32_t *blocktable, uint32_t blocktable_size) {
    int item_size = get_blocktable_item_size();

    for (int i = 0; i < blocktable_size; i++) {
        switch(item_size) {
            case 1: {
                uint8_t val = blocktable[i] & 0xff;
                fwrite(&val, sizeof(val), 1, output_file);
                break;
            }

            case 2: {
                uint16_t val = htons(blocktable[i] & 0xffff);
                fwrite(&val, sizeof(val), 1, output_file);
                break;
            }

            case 3: {
                uint8_t val[3];
                hton24(val, blocktable[i] & 0x0fffff);
                fwrite(&val, sizeof(val), 1, output_file);
                break;
            }

            case 4: {
                uint32_t val = htonl(blocktable[i]);
                fwrite(&val, sizeof(val), 1, output_file);
                break;
            }
        }
    }
}

/**
 * Creates a PSARC archive from specified files and stores it in the output file.
 *
 * @param output_path        Path to the PSARC output file.
 * @param files              Array of input file paths.
 * @param num_files          Number of input files.
 *
 * @return                   0 if successful
 *                           1 on error.
 *                           2 on error with output format
 */

int create_archive(char *output_path, char **files, size_t num_files ) {
    if (!_Config.overwrite_flag && access(output_path, F_OK) == 0) {
        fprintf( stderr, APPNAME": archive already exists (use -y for overwrite)\n" );
        return 1;
    }

    source_buffer = malloc(_ArchiveInfo.block_size * 2);
    if (!source_buffer) {
        fprintf( stderr, APPNAME": not enough memory\n" );
        return 1;
    }

    target_buffer = malloc(_ArchiveInfo.block_size * 2);
    if (!target_buffer) {
        fprintf( stderr, APPNAME": not enough memory\n" );

        free(source_buffer);
        return 1;
    }

    size_t total_size = 0;
    _ArchiveInfo.toc_entries = num_files;

    FILEINFO *files_info_table = (FILEINFO *)malloc((_ArchiveInfo.toc_entries + 1)* sizeof(FILEINFO));
    if (!files_info_table) {
        fprintf( stderr, APPNAME": not enough memory\n" );

        free(source_buffer);
        free(target_buffer);
        return 1;
    }

    char * filenames = NULL;
    size_t filenames_len = 0;

    for ( int i = 0; i < _ArchiveInfo.toc_entries; i++ ) {
        char *p = files[i], *px;
        size_t len;

        if ( _Config.trim_path_flag ) {
            px = strrchr( p, '/' );
            if ( px ) {
                p = ++px;
            }
        }

#ifdef __WIN32
        // Remove Drive:
        if (( px = strchr( p, ':' ))) {
            p = ++px;
#endif
            len = strlen(p);
            if ( _ArchiveInfo.archive_flags & AF_ABSPATH ) {
                // Add 1 to len for begin if not '/'
                if ( *p != '/' ) len++;
            } else {
                // Remove '/' from begin
                while ( *p == '/' ) {
                    p++;
                    len--;
                }
            }
#ifdef __WIN32
        } else {
            len = strlen(p);
        }
#endif
        filenames_len += len + ( ( i < _ArchiveInfo.toc_entries - 1 ) ? 1 : 0 );
    }

    filenames = malloc(filenames_len+1);
    if (!filenames) {
        fprintf( stderr, APPNAME": not enough memory\n" );

        free(files_info_table);
        free(source_buffer);
        free(target_buffer);
        return 1;
    }
    *filenames = '\0';
    
    for ( int i = 0; i < _ArchiveInfo.toc_entries; i++ ) {
        char *fname;
#ifdef __WIN32
        char *pfname;
        pfname = fname = path_to_unix(files[i], NULL);
        if ( !fname ) {
            fprintf( stderr, APPNAME": not enough memory\n" );

            free(filenames);
            free(files_info_table);
            free(target_buffer);
            free(source_buffer);
            return 1;
        }
#else
        fname = files[i];
#endif

        if ( _Config.trim_path_flag ) {
            char * px = strrchr( fname, '/' );
            if ( px ) fname = px;
        }

        if ( _ArchiveInfo.archive_flags & AF_ABSPATH ) {
            // Add / to begin if not exists
            if ( *fname != '/' ) strcat( filenames, "/" );
        } else {
            // Remove '/' from begin
            while ( *fname == '/' ) fname++;
        }

        strcat(filenames, fname);
#ifdef __WIN32
        free(pfname);
#endif
        if ( i < _ArchiveInfo.toc_entries - 1 ) strcat(filenames, "\x0a" );
    }

    // First block for files
    uint32_t blocktable_size = ( filenames_len + _ArchiveInfo.block_size - 1 ) / _ArchiveInfo.block_size;
    files_info_table[0].uncompressed_size = filenames_len;
    files_info_table[0].num_blocks = blocktable_size;
    uint32_t bsize = get_blocktable_size(files, _ArchiveInfo.toc_entries, &files_info_table[1]);
    if ( !bsize ) {
        fprintf( stderr, APPNAME": no files to add\n" );

        free(filenames);
        free(files_info_table);
        free(target_buffer);
        free(source_buffer);
        return 1;
    }
    blocktable_size += bsize;

    // Allocate the compressed sizes array
    uint32_t *blocktable = malloc((blocktable_size) * sizeof(uint32_t));
    if (!blocktable) {
        fprintf( stderr, APPNAME": not enough memory\n" );

        free(filenames);
        free(files_info_table);
        free(target_buffer);
        free(source_buffer);
        return 1;
    }

    _ArchiveInfo.toc_entries++;

    // Write the header and the entry table at the beginning of the uncompressed file
    _ArchiveInfo.toc_length = sizeof(PSARCHEADER) + _ArchiveInfo.toc_entries * sizeof(PSARCTOC) + blocktable_size * get_blocktable_item_size();

    FILE *archive_file = fopen(output_path, "wb");
    if (!archive_file) {
        fprintf( stderr, APPNAME": error creating archive\n" );

        free(filenames);
        free(files_info_table);
        free(target_buffer);
        free(source_buffer);
        free(blocktable);
        return 1;
    }

    write_header(archive_file);

    fseek(archive_file, _ArchiveInfo.toc_length, SEEK_SET);

    uint32_t blocktable_idx = 0;

    if (compress_entry((unsigned char *)filenames, filenames_len, NULL, archive_file, &files_info_table[0], &total_size, blocktable, &blocktable_idx) != 0) {
        fprintf( stderr, APPNAME": error getting filenames from archive\n" );

        fclose(archive_file);
        free(filenames);
        free(files_info_table);
        free(blocktable);
        free(target_buffer);
        free(source_buffer);
        unlink(output_path);
        return 1;
    }

    free(filenames);

    uint64_t files_compressed = 0LL;
    uint64_t files_uncompressed = 0LL;
    uint64_t manifest_compressed = files_info_table[0].compressed_size;
    uint64_t manifest_uncompressed = files_info_table[0].uncompressed_size;

    report = report_open(REPORT_TYPE_PAK, output_path);
    if (!report) {
        fprintf( stderr, APPNAME": fatal error\n" );

        fclose(archive_file);
        free(files_info_table);
        free(blocktable);
        free(target_buffer);
        free(source_buffer);
        unlink(output_path);
        return 1;
    }

    report_open_file_section(report);

    int user_data_size = sizeof(PAKDATA) + _ArchiveInfo.block_size * 4;

    if ( _Config.num_threads > 0 ) threads_init( _Config.num_threads, user_data_size);

    for (int i = 1; i < _ArchiveInfo.toc_entries; i++) {
        FILE *fp = NULL;
        files_info_table[i].filename = strdup(files[i-1]);
        if (!files_info_table[i].filename || !(fp = fopen(files_info_table[i].filename, "rb"))) {
            fprintf( stderr, APPNAME": error processing %s\n", files[i-1]);
            report_close(report, 1, files_compressed, files_uncompressed, manifest_compressed, manifest_uncompressed, i - 1, 1);

            for( ; i > 0; i--){
                free(files_info_table[i].filename);
                files_info_table[i].filename = NULL;
            }
            fclose(archive_file);
            free(files_info_table);
            free(blocktable);
            free(target_buffer);
            free(source_buffer);
            unlink(output_path);
            if ( _Config.num_threads > 0 ) threads_free();
            return 1;
        }

        if ( _Config.num_threads > 0 ) {
            compress_entry_multi(fp, archive_file, &files_info_table[i], &total_size, blocktable, &blocktable_idx, i < _ArchiveInfo.toc_entries - 1);
        } else {
            report_open_file_item(report, &files_info_table[i]);
            compress_entry(NULL, 0, fp, archive_file, &files_info_table[i], &total_size, blocktable, &blocktable_idx);
            report_close_file_item(report, files_info_table[i].uncompressed_size, files_info_table[i].compressed_size, NULL, i < _ArchiveInfo.toc_entries - 1);
            files_compressed += files_info_table[i].compressed_size;
        }

        fclose(fp);

        files_uncompressed += files_info_table[i].uncompressed_size;
    }

    if ( _Config.num_threads > 0 ) {
        threads_wait_for_completion();
        for (int i = 1; i < _ArchiveInfo.toc_entries; i++)files_compressed += files_info_table[i].compressed_size;
    }

    report_close_file_section(report);
    
    if ( _Config.num_threads > 0 ) threads_free();

    // Write the Toc table and block offsets
    write_toc_table(archive_file, files_info_table);
    write_blocktable(archive_file, blocktable, blocktable_size);

    fclose(archive_file);

    report_close(report, 1, files_compressed, files_uncompressed, manifest_compressed, manifest_uncompressed, _ArchiveInfo.toc_entries - 1, 0);

    free(target_buffer);
    free(source_buffer);
    free(blocktable);
    for (int i = 1; i < _ArchiveInfo.toc_entries; i++) free(files_info_table[i].filename); // Free filenames
    free(files_info_table);

    return 0;
}
