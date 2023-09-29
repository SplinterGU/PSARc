/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file psarc.h
 * @brief PSARC archive format definitions for the PSARc project.
 *
 * This header file contains definitions and structures related to the PSARC archive format
 * used in the PSARc project. It defines the structure and organization of PSARC archives.
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

#ifndef __PSARC_H
#define __PSARC_H

#include <stdint.h>

// Compression types
#define PSARC_STORE 0
#define PSARC_ZLIB  1
#define PSARC_LZMA  2

// Archive flags
#define AF_ICASE    1
#define AF_ABSPATH  2

#pragma pack(1)
typedef struct {
    char magic[4];                  // Offset 0x00: "PSAR" - PlayStation Archive Magic
    uint32_t version;               // Offset 0x04: Version of the PSARC format
                                    // Format: First 2 bytes major version, next 2 bytes minor version
                                    // Default: 0x00010004 (v1.4)
    char compression_type[4];       // Offset 0x08: Compression type used (e.g., "zlib")
    uint32_t toc_length;            // Offset 0x0C: Length of the Table of Contents (ToC)
                                    // Includes 32 bytes of header length + block length table following ToC
    uint32_t toc_entry_size;        // Offset 0x10: Size of a single ToC entry
                                    // Default: 30 (0x1E) bytes
    uint32_t toc_entries;           // Offset 0x14: Total number of ToC entries
                                    // Includes both Manifest and Files entries
                                    // Manifest (filenames table) is always the first entry without an assigned ID
    uint32_t block_size;            // Offset 0x18: Size of data chunks (Default is 65536 bytes)
    uint32_t archive_flags;         // Offset 0x1C: Archive flags:
                                    // Bit 0: 0 = Relative paths (default), 1 = Ignore case paths
                                    // Bit 1: 0 = Relative paths (default), 1 = Absolute paths
} PSARCHEADER;

typedef struct {
    char name_digest[16];           // Offset 0x00: 128-bit MD5 hash of the entry's name
    uint32_t block_offset;          // Offset 0x10: Offset in the block list
    char uncompressed_size[5];      // Offset 0x14: Size of this entry once uncompressed (40 bits)
    char file_offset[5];            // Offset 0x19: File offset in PSARC for this entry
} PSARCTOC;
#pragma pack()

#endif
