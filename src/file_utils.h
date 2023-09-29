/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file file_utils.c
 * @brief File utility functions for directory and file pattern handling in the PSARc project.
 *
 * This file contains utility functions dedicated to managing directory and file patterns in the
 * PSARc project. It expands patterns, verifies their existence, and creates a list of files to process.
 * Additionally, it filters out duplicate entries and normalizes file paths for consistency.
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

#ifndef FILE_UTILS_H
#define FILE_UTILS_H


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "hashset.h"

// Flags
#define FLAG_RECURSIVE  0x01
#define FLAG_ICASE      0x02

/**
 * @struct FILELIST
 * @brief A structure to represent a list of files.
 *
 * This structure holds an array of file paths, along with information about the number of files,
 * capacity, and a hashset to filter out duplicates.
 */
typedef struct {
    char **files;       /**< An array of file paths. */
    size_t count;       /**< The number of files in the list. */
    size_t capacity;    /**< The capacity of the list. */
    HASHSET *set;       /**< A hashset to filter out duplicate entries. */
} FILELIST;

/**
 * Get the current working directory.
 *
 * This function returns the path of the current working directory.
 *
 * @return                  A string containing the current working directory.
 */
char *get_current_dir();

#ifdef _WIN32
/**
 * Change the current working directory.
 *
 * This changes the current working directory to the specified path.
 *
 * @param x                 The path of the directory to change to.
 *
 * @return                  0 on success, 1 on error.
 */

#define chdir(x)    (!SetCurrentDirectory(x))
#endif


/**
 * Create a directory and all parent directories if they don't exist.
 *
 * This function creates a directory specified by the path 's' and all its parent directories
 * if they don't already exist. It sets the permissions for the created directories based on 'mode'.
 *
 * @param path              The path of the directory to create.
 * @param mode              The permissions to set for the created directories.
 *
 * @return                  0 if successful, -1 on error.
 */
int mkpath(const char *path, mode_t mode);

/**
 * Get the full path name of a file with directory validation.
 *
 * This function takes a file path as input, resolves its full path, and validates the existence
 * of all directories in the path. If any directory in the path does not exist, it returns NULL.
 *
 * The function then returns the full path as a dynamically allocated string, including the
 * filename. If the file itself does not exist, it does not create any directories or verify
 * the file's existence.
 *
 * @param path              The file path to resolve.
 *
 * @return                  A dynamically allocated string containing the full path,
 *                          or NULL if any directory in the path does not exist or if an error occurs
 *                          (memory allocation or path resolution).
 */

char *fullpath(const char *path);

#ifdef _WIN32
/**
 * Convert a Windows-style path to a Unix-style path.
 *
 * This function takes a Windows-style path and converts it to a Unix-style path
 * by replacing backslashes with forward slashes and removing the drive letter prefix.
 * If the `resolved_path` parameter is NULL, the function allocates memory for the
 * converted path and returns it.
 *
 * @param path              The Windows-style path to convert.
 * @param converted_path    A pointer to a character buffer to store the converted path,
 *                          or NULL to allocate memory dynamically.
 *
 * @return                  A pointer to the converted Unix-style path.
 *                          If `converted_path` is provided, it will be used; otherwise,
 *                          dynamically allocated memory will be used. Returns NULL on error.
 */
char *path_to_unix(const char* path, char *converted_path);
#endif

/**
 * Initialize a FILELIST structure.
 *
 * This function initializes a FILELIST structure with the given initial capacity and hashset size.
 *
 * @param list              A pointer to the FILELIST structure to initialize.
 * @param initialCapacity   The initial capacity for the file list.
 * @param hashSize          The size of the hashset used for duplicate filtering.
 */
void filelist_init(FILELIST *list, size_t initialCapacity, size_t hashSize);

/**
 * Add a file to the FILELIST.
 *
 * This function adds a file to the FILELIST structure, performing duplicate filtering.
 *
 * @param list              A pointer to the FILELIST structure.
 * @param filename          The file path to add.
 *
 * @return                  True if the file was added, false if it's a duplicate.
 */
int filelist_add(FILELIST *list, char *filename);

/**
 * Free resources associated with a FILELIST structure.
 *
 * This function releases memory and resources associated with a FILELIST structure.
 *
 * @param list              A pointer to the FILELIST structure.
 */
void filelist_free(FILELIST *list);

/**
 * List files recursively in a directory.
 *
 * This function recursively lists files in a directory and adds them to a FILELIST.
 *
 * @param path              The path of the directory to list.
 * @param filelist          A pointer to the FILELIST structure.
 * @param flags             Flags to control the listing, e.g., FLAG_RECURSIVE for recursive listing.
 */
void list_files_recursively(const char *path, FILELIST *filelist, uint16_t flags);

/**
 * Process a file pattern and add matching files to a FILELIST.
 *
 * This function processes a file pattern, expands it, and adds matching files to a FILELIST.
 *
 * @param pattern           The file pattern to process.
 * @param filelist          A pointer to the FILELIST structure.
 * @param flags             Flags to control the processing, e.g., FLAG_RECURSIVE for recursive matching.
 *
 * @return                  0 if successful, -1 on error.
 */

int process_pattern(char *pattern, FILELIST *filelist, uint16_t flags);

#endif /* FILE_UTILS_H */
