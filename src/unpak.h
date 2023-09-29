/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file unpak.h
 * @brief Header for the unpak utility in the PSARc project.
 *
 * This header file contains declarations and constants for the unpak utility used in
 * the PSARc project. It defines interfaces for extracting files from PSARC archives.
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

#ifndef __UNPAK_H
#define __UNPAK_H

#include <stdint.h>
#include "common.h"
#include "psarc.h"

/**
 * Processes the PSARC archive based on the specified mode.
 *
 * This function processes the PSARC archive based on the specified mode.
 *
 * @param input_file         Path to the input PSARC archive.
 * @param mode               Processing mode.
 * @param files              Array of input file paths.
 * @param num_files          Number of input files.
 *
 * @return                   0 if successful, -1 on error.
 */
int process_archive(char *input_file, int mode, char **files, size_t num_files);

#endif
