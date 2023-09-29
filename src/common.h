/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file common.h
 * @brief Common utility functions and declarations for the PSARc project.
 *
 * This header file contains common utility functions and declarations that are used throughout
 * the PSARc project. It provides essential interfaces and constants for other parts of the project.
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

#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>
#include <stddef.h>

#define APPNAME     "psarc"

// Define an enum for format values
enum FORMAT_VALUE_ENUM {
    STANDARD_FORMAT = 0,
    JSON_FORMAT = 1,
    CSV_FORMAT = 2,
    XML_FORMAT = 3,
    // Add more formats if needed
    UNKNOWN_FORMAT = -1
};

// Internals
typedef struct {
    struct {
        uint16_t high;
        uint16_t low;
    } version;
    int compression_type;           // Compression type:
                                    // 0 - store
                                    // 1 - zlib
                                    // 2 - lzma
    size_t toc_length;              // Total length of the Table of Contents (ToC), includes 32 bytes of header length and block length table following ToC
    uint32_t toc_entries;           // Number of entries, including Manifest and Files. The manifest (filenames table) is always the first file in the archive without an assigned ID.
    uint32_t block_size;            // Chunk size in bytes (default is 65536 bytes)
    uint32_t archive_flags;         // Flags:
                                    // 0 = relative paths (default)
                                    // 1 = ignore case in paths
                                    // 2 = absolute paths
} ARCHIVEINFO;

// Structure to store information about a file in the entry table
typedef struct {
    uint8_t name_digest[16];                // Digest of the file name
    char *filename;                         // File name
    uint64_t offset;                        // Offset within the archive
    uint32_t block_index;                   // Index of the first block
    uint32_t num_blocks;                    // Blocks used for this file
    size_t compressed_size;                 // Size of the file when compressed
    uint64_t uncompressed_size;             // Size of the file when uncompressed
} FILEINFO;

typedef struct {
    char *archive_file;                     // Archive name (input/output)
    int compression_level;                  // Compression level
    int extreme_compression_flag;           // Extreme compression flag for LZMA
    int overwrite_flag;                     // Overwrite flag
    int verbose_flag;                       // Verbose flag
    int recursive_flag;                     // Recursive flag
    char *source_dir;                       // Source directory
    char *target_dir;                       // Target directory
    int trim_path_flag;                     // Remove path from path file name flag
    int skip_existing_files_flag;           // Skip existing files flag
    int num_threads;                        // Number of threads
    enum FORMAT_VALUE_ENUM output_format;   // Output format for information
} CONFIG;

extern ARCHIVEINFO _ArchiveInfo;        // Archive information
extern CONFIG _Config;                  // config options

int get_blocktable_item_size();         // Retrieve the size of a single item in the block table based on the block size.
char *lcase(char *s);

/**
 * Calculates the compressed size of a file within the PSARC archive.
 *
 * This function calculates the compressed size of a file within the PSARC archive based on its
 * block size and block index.
 *
 * @param fi          Information about the file entry.
 * @param blocktable        The table containing block sizes for the PSARC archive.
 *
 * @return                  The compressed size of the file.
 */

uint64_t get_compressed_size(FILEINFO *fi, uint32_t *blocktable);

#endif
