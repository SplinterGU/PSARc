/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file hashset.c
 * @brief HASHSET data structure and operations for the PSARc project.
 *
 * This file implements a HASHSET data structure and associated operations used in the PSARc project.
 * The HASHSET is designed for efficient storage and retrieval of unique elements.
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

#include "hashset.h"

/**
 * Initializes the hash set with the specified size.
 *
 * @param size The size of the hash set.
 *
 * @return     A pointer to the initialized hash set, or NULL if memory allocation fails.
 */

HASHSET *hashset_init(size_t size) {
    HASHSET *set = (HASHSET *)malloc(sizeof(HASHSET));
    if (!set) {
        return NULL;
    }

    set->size = size;
    set->table = (HASH_NODE **)calloc(size, sizeof(HASH_NODE *));
    if (!set->table) {
        free(set);
        return NULL;
    }

    return set;
}

/**
 * Simple hash function to calculate the index for a given string in the hash table.
 *
 * @param str  The input string.
 * @param size The size of the hash table.
 *
 * @return     The calculated hash index.
 */

size_t hash(const char *str, size_t size) {
    size_t hash = 0;
    for (size_t i = 0; str[i] != '\0'; i++) {
        hash = (hash * 31) + str[i];
    }
    return hash % size;
}

/**
 * Checks if a string value exists in the hash set.
 *
 * @param set           Pointer to the hash set.
 * @param value         Value to check for existence.
 * @param index         Pre-calculated hash index for the value.
 *
 * @return              1 if the value exists in the set, 0 otherwise.
 */

static int hashset_containsAtIndex(HASHSET *set, const char *value, size_t index) {
    HASH_NODE *current = set->table[index];
    while (current) {
        if (strcmp(current->value, value) == 0) {
            return 1; // Value exists in the table
        }
        current = current->next;
    }
    return 0; // Value does not exist in the set
}

/**
 * Checks if a string value exists in the hash set.
 *
 * @param set           Pointer to the hash set.
 * @param value         Value to check for existence.
 *
 * @return              1 if the value exists in the set, 0 otherwise.
 */

int hashset_contains(HASHSET *set, const char *value) {
    if (!set || !value) {
        return 0;
    }

    return hashset_containsAtIndex(set, value, hash(value, set->size));
}

/**
 * Inserts a string value into the hash set.
 *
 * @param set           Pointer to the hash set.
 * @param value         Value to insert.
 *
 * @return              1 if insertion is successful, 0 otherwise (e.g., if it already exists or memory allocation fails).
 */

int hashset_add(HASHSET *set, const char *value) {
    if (!set || !value) {
        return 0;
    }

    size_t index = hash(value, set->size);

    // Check if the value already exists in the set
    if (hashset_containsAtIndex(set, value, index)) {
        return 0; // Value already exists in the set
    }

    // Insert the new string value into the table
    HASH_NODE *new_node = (HASH_NODE *)malloc(sizeof(HASH_NODE));
    if (!new_node) {
        return 0; // Memory allocation failed
    }

    new_node->value = strdup(value);
    new_node->next = set->table[index];
    set->table[index] = new_node;

    return 1; // Insertion successful
}

/**
 * Removes a string value from the hash set.
 *
 * @param set           Pointer to the hash set.
 * @param value         Value to remove.
 *
 * @return              1 if removal is successful, 0 if the value doesn't exist or memory allocation fails.
 */

int hashset_del(HASHSET *set, const char *value) {
    if (!set || !value) {
        return 0;
    }

    size_t index = hash(value, set->size);

    // Check if the value exists in the set
    HASH_NODE *current = set->table[index];
    HASH_NODE *previous = NULL;

    while (current) {
        if (strcmp(current->value, value) == 0) {
            // Found the value to remove
            if (previous) {
                previous->next = current->next;
            } else {
                set->table[index] = current->next;
            }

            free(current->value);
            free(current);
            return 1; // Removal successful
        }
        previous = current;
        current = current->next;
    }

    return 0; // Value not found in the set
}

/**
 * Frees the memory used by the hash set and its nodes.
 *
 * @param set   Pointer to the hash set to free.
 */

void hashset_free(HASHSET *set) {
    if (!set) {
        return;
    }

    for (size_t i = 0; i < set->size; i++) {
        HASH_NODE *current = set->table[i];
        while (current) {
            HASH_NODE *next = current->next;
            free(current->value);
            free(current);
            current = next;
        }
    }

    free(set->table);
    free(set);
}
