/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file pak.h
 * @brief Header for PSARC file compression in the PSARc project.
 *
 * This header file contains declarations and function prototypes for performing file compression
 * into the PSARC format used in the PSARc project. It defines essential interfaces for creating
 * compressed PSARC archives.
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

#ifndef __PAK_H
#define __PAK_H

#include <stdint.h>
#include "common.h"
#include "psarc.h"

/**
 * Creates a PSARC archive from specified files and stores it in the output file.
 *
 * This function creates a PSARC archive from specified files and stores it in the output file.
 *
 * @param output_path       Path to the PSARC output file.
 * @param files             Array of input file paths.
 * @param num_files         Number of input files.
 *
 * @return                  0 if successful, 1 on error.
 */

int create_archive(char *output_path, char **files, size_t num_files);

#endif
