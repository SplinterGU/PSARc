/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file md5.h
 * @brief MD5 hashing algorithm for the PSARc project.
 *
 * This header file defines the MD5 hashing algorithm for use in the PSARc project.
 * It provides declarations and interfaces for generating MD5 hashes of data.
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

#ifndef __MD5_H
#define __MD5_H

#include <stdint.h>

/**
 * This function calculates the MD5 hash of a given message, represented by the `initial_msg` buffer
 * with a specified length `initial_len`. The resulting MD5 hash is stored in the `result` buffer.
 *
 * @param initial_msg    The input message for which to calculate the MD5 hash.
 * @param initial_len    The length of the input message.
 * @param result         A buffer to store the resulting MD5 hash (must have space for 16 bytes).
 *
 * @return               Returns 0 on success, or a non-zero value if an error occurs.
 */

extern int md5(uint8_t *initial_msg, size_t initial_len, uint8_t * result);

#endif
