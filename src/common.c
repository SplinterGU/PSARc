/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file common.c
 * @brief Common utility functions for the PSARc project.
 *
 * This file contains common utility functions that are used throughout the PSARc project.
 * These functions provide various helpful functionalities to other parts of the project.
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

#include <string.h>
#include <ctype.h>

#include "common.h"
#include "psarc.h"

// Global variables

CONFIG _Config = {
    .archive_file = NULL,                   // Archive name (input/output)
    .compression_level = 5,                 // Compression level (default for zlib)
    .extreme_compression_flag = 0,          // Extreme compression flag for LZMA
    .overwrite_flag = 0,                    // Overwrite flag
    .verbose_flag = 0,                      // Verbose flag
    .recursive_flag = 0,                    // Recursive flag
    .source_dir = NULL,                     // Source directory
    .target_dir = NULL,                     // Target directory
    .trim_path_flag = 0,                    // Remove path from path file name flag
    .skip_existing_files_flag = 0,          // Skip existing files flag
    .num_threads = 0,                       // Number of threads
    .output_format = STANDARD_FORMAT,       // Output format for information
};

ARCHIVEINFO _ArchiveInfo = {
    .version.high = 1,                      // Archive version (high)
    .version.low = 4,                       // Archive version (low)
    .compression_type = PSARC_STORE,        // Compression type (PSARC_STORE)
    .toc_length = 0,                        // Table of Contents (TOC) length
    .toc_entries = 0,                       // Number of TOC entries
    .block_size = 65536,                    // Block size (default is 65536 bytes)
    .archive_flags = 0,                     // Archive flags
};

/**
 * Convert a string to lowercase.
 *
 * This function takes a string as input and returns a new dynamically allocated string
 * where all characters are converted to lowercase. The original string remains unchanged.
 *
 * @param s     The input string to be converted to lowercase.
 *
 * @return      A dynamically allocated string with all characters converted to lowercase.
 *              The caller is responsible for freeing the memory allocated for the returned string.
 */

char *lcase(char *s) {
    char *so = strdup(s); // Create a copy of the input string
    for (int i = 0; i < strlen(so); i++) {
        so[i] = tolower(so[i]); // Convert each character to lowercase
    }
    return so; // Return the lowercase string (the caller is responsible for freeing its memory)
}

/**
 * Get the size of the block type based on the block size.
 *
 * @return              The size of the block type (1, 2, 3, or 4) or -1 for an invalid block size.
 */

int get_blocktable_item_size() {
    if (_ArchiveInfo.block_size <= 0x100) return 1;          // 1 byte block size
    if (_ArchiveInfo.block_size <= 0x10000) return 2;        // 2 bytes block size
    if (_ArchiveInfo.block_size <= 0x1000000) return 3;      // 3 bytes block size
    if (_ArchiveInfo.block_size <= 0x100000000ULL) return 4; // 4 bytes block size
    return -1; // Invalid block size
}

/**
 * Calculates the compressed size of a file within the PSARC archive.
 *
 * This function calculates the compressed size of a file within the PSARC archive based on its
 * block size and block index.
 *
 * @param fi                Information about the file entry.
 * @param blocktable        The table containing block sizes for the PSARC archive.
 *
 * @return                  The compressed size of the file.
 */

uint64_t get_compressed_size(FILEINFO *fi, uint32_t *blocktable) {
    uint64_t chunk_size = _ArchiveInfo.block_size;
    uint64_t length = 0;

    if (fi->uncompressed_size != 0) {
        uint32_t index = fi->block_index;
        uint32_t blocks = ( fi->uncompressed_size + chunk_size - 1 ) / chunk_size;
        while (blocks--) {
            uint32_t block_size = blocktable[index];
            // block_size == 0 => is chunk_size
            if (!block_size) block_size = chunk_size;
            length += block_size;
            index++;
        }
    }

    return length;
}
