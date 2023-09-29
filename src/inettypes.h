/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file inettypes.h
 * @brief Network data type conversion functions and declarations for the PSARc project.
 *
 * This header file contains declarations and function prototypes for network data type conversion
 * functions used in the PSARc project. These functions enable the conversion of data between
 * network byte order (big-endian) and host byte order (platform-dependent) for 40-bit, 24-bit, and
 * 64-bit integers, ensuring proper network communication and data consistency.
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

#ifndef __INETTYPES_H
#define __INETTYPES_H

#include <stdint.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

/**
 * Convert a 64-bit value to network byte order (big-endian).
 *
 * This function converts a 64-bit value to network byte order (big-endian).
 *
 * @param ptr    Pointer to the memory where the result will be stored.
 * @param value  The 64-bit value to convert.
 */
extern void hton40(uint8_t *ptr, uint64_t value);

/**
 * Convert a 40-bit value from network byte order (big-endian) to host byte order.
 *
 * This function converts a 40-bit value from network byte order (big-endian) to host byte order.
 *
 * @param ptr  Pointer to the memory where the 40-bit value is stored.
 * @return     The 40-bit value in host byte order.
 */
extern uint64_t ntoh40(const uint8_t *ptr);

/**
 * Convert a 24-bit value to network byte order (big-endian).
 *
 * This function converts a 24-bit value to network byte order (big-endian).
 *
 * @param ptr    Pointer to the memory where the result will be stored.
 * @param value  The 24-bit value to convert.
 */
extern void hton24(uint8_t *ptr, uint32_t value);

/**
 * Convert a 24-bit value from network byte order (big-endian) to host byte order.
 *
 * This function converts a 24-bit value from network byte order (big-endian) to host byte order.
 *
 * @param ptr  Pointer to the memory where the 24-bit value is stored.
 * @return     The 24-bit value in host byte order.
 */
extern uint32_t ntoh24(const uint8_t *ptr);

/**
 * Convert a 64-bit value to network byte order (big-endian) or host byte order, depending on the machine's endianness.
 *
 * This function converts a 64-bit value to network byte order (big-endian) if the machine is little-endian,
 * or it leaves the value unchanged if the machine is big-endian.
 *
 * @param value  The 64-bit value to convert.
 * @return       The 64-bit value in the appropriate byte order.
 */
extern uint64_t htonll(uint64_t value);

#endif
