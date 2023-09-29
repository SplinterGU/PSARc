/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file inettypes.c
 * @brief Implementation of network data type conversion functions for the PSARc project.
 *
 * This file provides the implementation of functions responsible for converting data between
 * network byte order (big-endian) and host byte order (platform-dependent) for various data sizes,
 * including 40-bit, 24-bit, and 64-bit integers. These functions are essential for network
 * communication and data consistency in the PSARc project.
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

#ifndef _WIN32
#include <arpa/inet.h>
#endif
#include "inettypes.h"

/**
 * Convert a 64-bit value to network byte order (big-endian).
 *
 * This function converts a 64-bit value to network byte order (big-endian).
 *
 * @param ptr    Pointer to the memory where the result will be stored.
 * @param value  The 64-bit value to convert.
 */
void hton40(uint8_t *ptr, uint64_t value) {
    ptr[0] = (uint8_t)((value >> 32) & 0xFF);
    ptr[1] = (uint8_t)((value >> 24) & 0xFF);
    ptr[2] = (uint8_t)((value >> 16) & 0xFF);
    ptr[3] = (uint8_t)((value >> 8) & 0xFF);
    ptr[4] = (uint8_t)(value & 0xFF);
}

/**
 * Convert a 40-bit value from network byte order (big-endian) to host byte order.
 *
 * This function converts a 40-bit value from network byte order (big-endian) to host byte order.
 *
 * @param ptr  Pointer to the memory where the 40-bit value is stored.
 * @return     The 40-bit value in host byte order.
 */
uint64_t ntoh40(const uint8_t *ptr) {
    return (((uint64_t) ptr[0]) << 32) | (((uint64_t) ptr[1]) << 24) | (((uint64_t) ptr[2]) << 16) | (((uint64_t) ptr[3]) << 8) | ((uint64_t) ptr[4]);
}

/**
 * Convert a 24-bit value to network byte order (big-endian).
 *
 * This function converts a 24-bit value to network byte order (big-endian).
 *
 * @param ptr    Pointer to the memory where the result will be stored.
 * @param value  The 24-bit value to convert.
 */
void hton24(uint8_t *ptr, uint32_t value) {
    ptr[0] = (uint8_t)((value >> 16) & 0xFF);
    ptr[1] = (uint8_t)((value >> 8) & 0xFF);
    ptr[2] = (uint8_t)(value & 0xFF);
}

/**
 * Convert a 24-bit value from network byte order (big-endian) to host byte order.
 *
 * This function converts a 24-bit value from network byte order (big-endian) to host byte order.
 *
 * @param ptr  Pointer to the memory where the 24-bit value is stored.
 * @return     The 24-bit value in host byte order.
 */
uint32_t ntoh24(const uint8_t *ptr) {
    return (((uint64_t) ptr[0]) << 16) | (((uint64_t) ptr[1]) << 8) | ((uint64_t) ptr[2]);
}

/**
 * Convert a 64-bit value to network byte order (big-endian) or host byte order, depending on the machine's endianness.
 *
 * This function converts a 64-bit value to network byte order (big-endian) if the machine is little-endian,
 * or it leaves the value unchanged if the machine is big-endian.
 *
 * @param value  The 64-bit value to convert.
 * @return       The 64-bit value in the appropriate byte order.
 */
uint64_t htonll(uint64_t value) {
    // Check the endianness of the machine
    static const int num = 42;
    if (*(char *)&num == 42) { // Little-endian
        uint32_t high_part = htonl((uint32_t)(value >> 32));
        uint32_t low_part = htonl((uint32_t)(value & 0xFFFFFFFFLL));
        return ((uint64_t)low_part << 32) | high_part;
    } else { // Big-endian
        return value;
    }
}
