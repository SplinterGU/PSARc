/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file md5.c
 * @brief Implementation of the MD5 hashing algorithm for the PSARc project.
 *
 * This file implements the MD5 hashing algorithm used in the PSARc project.
 * It contains functions for computing MD5 hashes of data.
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

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
 
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

int md5(uint8_t *initial_msg, size_t initial_len, uint8_t * result) {
 
    // Message (to prepare)
    uint8_t *msg = NULL;
 
    // Note: All variables are unsigned 32 bit and wrap modulo 2^32 when calculating
 
    // r specifies the per-round shift amounts
 
    uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                    5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

    // Use binary integer part of the sines of integers (in radians) as constants// Initialize variables:
    uint32_t k[] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
        0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
        0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
        0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
        0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
        0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
        0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
        0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
        0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

    // These vars will contain the hash
//    uint32_t h[] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 };
    uint32_t *h = ( uint32_t * ) result;
    h[0] = 0x67452301;
    h[1] = 0xefcdab89;
    h[2] = 0x98badcfe;
    h[3] = 0x10325476;

    // Pre-processing: adding a single 1 bit
    //append "1" bit to message    
    /* Notice: the input bytes are considered as bits strings,
       where the first bit is the most significant bit of the byte.[37] */
 
    // Pre-processing: padding with zeros
    //append "0" bit until message length in bit ≡ 448 (mod 512)
    //append length mod (2 pow 64) to message
 
    int new_len = ((((initial_len + 8) / 64) + 1) * 64) - 8;
 
    msg = calloc(new_len + 64, 1); // also appends "0" bits 
                                   // (we alloc also 64 extra bytes...)

    if ( !msg ) return 1;

    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 128; // write the "1" bit
 
    uint32_t bits_len = 8*initial_len; // note, we append the len
    memcpy(msg + new_len, &bits_len, 4);           // in bits at the end of the buffer

    uint32_t i;
 
    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
    int offset;
    for(offset=0; offset<new_len; offset += (512/8)) { 
        // break chunk into sixteen 32-bit words w[j], 0 ≤ j ≤ 15
        uint32_t *w = (uint32_t *) (msg + offset);
 
        // Initialize hash value for this chunk:
        uint32_t a = h[0];
        uint32_t b = h[1];
        uint32_t c = h[2];
        uint32_t d = h[3];
 
        // Main loop:
        for(i = 0; i<64; i++) {
            uint32_t f, g;
 
            if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;          
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }

            uint32_t temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;
        }
 
        // Add this chunk's hash to result so far:
 
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
 
    }
 
    // cleanup
    free(msg);
 
    return 0;
}
