/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file hashset.h
 * @brief HASHSET data structure and declarations for the PSARc project.
 *
 * This header file defines the HASHSET data structure and associated declarations for use
 * in the PSARc project. The HASHSET provides efficient methods for managing unique elements.
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

#ifndef __HASHSET_H
#define __HASHSET_H

// Define the HASH_NODE structure for individual elements in the hash table
typedef struct HASH_NODE {
    char *value;
    struct HASH_NODE *next;
} HASH_NODE;

// Define the HASHSET structure for the hash table
typedef struct {
    size_t size;
    HASH_NODE **table;
} HASHSET;

// Function prototypes
HASHSET *hashset_init(size_t size);
int hashset_contains(HASHSET *set, const char *value);
int hashset_add(HASHSET *set, const char *value);
int hashset_del(HASHSET *set, const char *value);
void hashset_free(HASHSET *set);

#endif /* __HASHSET_H */
