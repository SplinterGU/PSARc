/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file file_utils.h
 * @brief Header for file and directory pattern handling in the PSARc project.
 *
 * This header file contains declarations and function prototypes for handling file and directory
 * patterns in the PSARc project. It defines essential interfaces for expanding patterns, verifying
 * existence, creating a list of files to process, filtering duplicates, and normalizing file paths
 * to ensure consistency.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define stat _stat
#define mkdir(path, mode) _mkdir(path)
#define snprintf _snprintf
#else
#include <glob.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#endif

#include "file_utils.h"

#ifdef _WIN32
typedef struct {
    char pattern[PATH_MAX];
    int is_dir;
} FRAGMENT_PATH;

static int match(const char *pattern, const char *string, uint16_t flags);
static void process_directory_pattern(FILELIST *filelist, FRAGMENT_PATH *fragment_paths, int num_fragment_paths, char *base_dir, uint16_t flags);

/**
 * Create a directory and all of its parent directories if they do not exist.
 *
 * This function recursively creates directories in a path if they do not already exist.
 *
 * @param s     The path of the directory to create.
 * @param mode  The permissions to set for the created directories.
 *
 * @return      0 on success, -1 on error.
 */

char *realpath(const char* path, char *resolved_path) {
    size_t pathsz;
    if ( !resolved_path ) {
        pathsz = GetFullPathName(path, 0, NULL, NULL);
        if ( !pathsz ) return NULL;
        resolved_path = malloc(pathsz);
    }
    GetFullPathName(path, pathsz, resolved_path, NULL);

    char *p = resolved_path;
    while (( p = strchr( p, '\\' ))) {
        *p = '/';
        p++;
    }

    return resolved_path;
}

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

char *path_to_unix(const char* path, char *converted_path) {
    if (!path && !converted_path) return NULL;

    if (!converted_path) {
        converted_path = strdup(path);
        if (!converted_path) {
            return NULL; // Memory allocation error
        }
    } else {
        if ( path && path != converted_path ) strcpy(converted_path, path);
    }

    // Replace backslashes with forward slashes
    char *p = converted_path;
    while ((p = strchr(p, '\\'))) {
        *p = '/';
        p++;
    }

    return converted_path;
}

#endif

/**
 * Get the current working directory.
 *
 * This function returns the path of the current working directory.
 *
 * @return  A string containing the current working directory.
 */

char *get_current_dir() {
#ifdef _WIN32
    DWORD size = GetCurrentDirectory(0, NULL);
    if ( !size ) return NULL;
    char *path = malloc(size);
    GetCurrentDirectory(size, path);
#else
    char *path = getcwd(NULL, 0);
#endif
    return path;
}

/**
 * Create a directory and all of its parent directories if they do not exist.
 *
 * This function recursively creates directories in a path if they do not already exist.
 *
 * @param path  The path of the directory to create.
 * @param mode  The permissions to set for the created directories.
 *
 * @return      0 on success, -1 on error.
 */

int mkpath(const char *path, mode_t mode) {
    char *q, *r;
    int rv = -1;

    if ( !path[0]
        || strcmp(path, ".") == 0
        || strcmp(path, "/") == 0 
#ifdef _WIN32
        || ( strlen(path) == 2 && path[1] == ':' ) 
        || ( strlen(path) > 2 && strcmp(path + strlen(path) - 2, ":.") == 0 ) 
        || ( strlen(path) > 2 && ( strcmp(path + strlen(path) - 2, ":/") == 0 ) || strcmp(path + strlen(path) - 2, ":\\") == 0 ) 
#endif
        ) return 0;

    if (!(q = strdup(path))) return -1;

    if (!(r = dirname(q))) {
        free(q);
        return -1;
    }

    if ((mkpath(r, mode) == -1) && (errno != EEXIST)) {
        free(q);
        return -1;
    }

    if ((mkdir(path, mode) == -1) && (errno != EEXIST)) {
        rv = -1;
    } else {
        rv = 0;
    }

    free(q);

    return (rv);
}

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
 * @param path      The file path to resolve.
 *
 * @return          A dynamically allocated string containing the full path,
 *                  or NULL if any directory in the path does not exist or if an error occurs
 *                  (memory allocation or path resolution).
 */

char *fullpath(const char *path) {
    // Duplicate the input path
    char *p = strdup(path);
    if (!p) return NULL;

    // Get the directory of the input path
    char *directory = dirname(p);

    // Resolve the real path of the directory
    char *real_dir = realpath(directory, NULL);
    free(p); // Free the duplicated input path
    if (!real_dir) return NULL;

    // Duplicate the archive file name
    char *filename = strdup(path);
    if (!filename) {
        free(real_dir); // Free the resolved directory path
        return NULL;
    }

    // Get the base name of the archive file
    char *name = basename(filename);

    // Allocate memory for the full path
    char *fpath = malloc(strlen(real_dir) + 1 + strlen(name) + 1);
    if (!fpath) {
        free(filename); // Free the duplicated archive file name
        free(real_dir); // Free the resolved directory path
        return NULL;
    }

    // Create the full path by concatenating the directory and file name
    sprintf(fpath, "%s/%s", real_dir, name);

    free(filename); // Free the duplicated archive file name
    free(real_dir); // Free the resolved directory path

    return fpath;
}

#ifndef _WIN32
/**
 * Create a case-insensitive pattern from the given pattern.
 *
 * This function takes a pattern as input and creates a new pattern that is case-insensitive.
 * It surrounds each alphabet character in square brackets to match both lowercase and uppercase versions.
 *
 * @param pattern The input pattern to be made case-insensitive.
 *
 * @return A dynamically allocated string containing the case-insensitive pattern,
 *         or NULL if memory allocation fails. The caller is responsible for freeing the returned string.
 */

char *icase_pattern(char *pattern) {
    // Calculate the maximum possible length for the new pattern (4 times the original length)
    char *newPattern = malloc(strlen(pattern) * 4 + 1);
    if (!newPattern) {
        return NULL; // Memory allocation failed
    }
    
    // Create a new case-insensitive pattern
    int j = 0;
    for (int i = 0; i < strlen(pattern); i++) {
        char c = pattern[i];
        if (isalpha(c)) {
            newPattern[j++] = '[';
            newPattern[j++] = tolower(c);
            newPattern[j++] = toupper(c);
            newPattern[j++] = ']';
        } else {
            newPattern[j++] = c;
        }
    }
    newPattern[j] = '\0';
    return newPattern;
}
#endif

/**
 * Removes occurrences of "." and "./" from a path string.
 *
 * @param path The input path string.
 */

void rm_dot_dir_from_path(char *path) {
    int len = strlen(path);

    if (len >= 2) {
        // Handle "./"
        for (int i = 0; i < len - 1; i++) {
            if (path[i] == '.' && path[i + 1] == '/' && (i == 0 || (i > 0 && path[i - 1] == '/'))) {
                memmove(path + i, path + i + 2, len - i - 1);
                len -= 2;
                i--; // Check the same index again for multiple occurrences
            }
        }
    }
}

/**
 * Initialize the file list, including the associated HASHSET.
 *
 * This function initializes a FILELIST structure, allocates memory for the list of files,
 * and sets the initial capacity for both the list and the HASHSET.
 *
 * @param list           Pointer to the FILELIST structure to initialize.
 * @param initialCapacity The initial capacity for the list of files.
 * @param hashSize       The initial capacity for the associated HASHSET.
 */

void filelist_init(FILELIST *list, size_t initialCapacity, size_t hashSize) {
    list->files = (char **)malloc(initialCapacity * sizeof(char *));
    list->count = 0;
    list->capacity = initialCapacity;

    // Initialize the HASHSET with the specified size
    list->set = hashset_init(hashSize);
}

/**
 * Add a file to the file list while ensuring uniqueness based on canonical paths.
 *
 * This function adds a file to the FILELIST structure while checking for duplicate entries
 * based on their canonical paths. If the file's canonical path already exists in the HASHSET,
 * the file is not added again to maintain uniqueness.
 *
 * @param list      Pointer to the FILELIST structure.
 * @param filename  The filename (path) to add to the list.
 *
 * @return          1 if the file was successfully added (or already exists), 0 on error.
 */

int filelist_add(FILELIST *list, char *filename) {
    char *canonicalPath = realpath(filename, NULL);
    if (canonicalPath == NULL) {
        // Error in obtaining the canonical path, return 0
        return 0;
    }

    // Check if the canonical path already exists in the set
    if (!hashset_add(list->set, canonicalPath)) {
        // The file with the same canonical path is already in the set, do not add it
        free(canonicalPath);
        return 0;
    }

    // If the list is full, increase its capacity
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->files = (char **)realloc(list->files, list->capacity * sizeof(char *));
    }

    rm_dot_dir_from_path(filename);

    // Add the canonical path to the list
    const char *parentDir = strstr(filename, "../");
    if (parentDir == filename || (parentDir > filename && parentDir[-1] == '/')) {
        list->files[list->count] = strdup(canonicalPath);
    } else {
        list->files[list->count] = strdup(filename);
        free(canonicalPath);
    }
    list->count++;
    return 1; // Indicates that the file was added
}

/**
 * Frees the memory used by the file list.
 *
 * @param list              The FILELIST structure to be freed.
 */

void filelist_free(FILELIST *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->files[i]);
    }
    free(list->files);
    list->count = 0;
    list->capacity = 0;

    // Free the HASHSET and its resources
    if (list->set != NULL) {
        hashset_free(list->set);
        list->set = NULL;
    }
}

/**
 * Recursively lists files in a directory and its subdirectories.
 *
 * @param path              The path of the directory to open listing from.
 * @param filelist          The FILELIST structure to store the file list.
 */

void list_files_recursively(const char *path, FILELIST *filelist, uint16_t flags) {
#ifdef _WIN32
    WIN32_FIND_DATA find_file_data;
    HANDLE hFind;
    char search_path[PATH_MAX];

    snprintf(search_path, sizeof(search_path), "%s*", path);

    hFind = FindFirstFileEx(search_path, FindExInfoStandard, &find_file_data, FindExSearchNameMatch, NULL, 0);

    if (hFind == INVALID_HANDLE_VALUE) return; // Error opening directory

    do {
        if (strcmp(find_file_data.cFileName, ".") != 0 && strcmp(find_file_data.cFileName, "..") != 0) {
            char fpath[PATH_MAX];
            snprintf(fpath, sizeof(fpath), "%s%s", path, find_file_data.cFileName);
            if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                strcat(fpath, "/");
                list_files_recursively(fpath, filelist, flags);
            } else {
                filelist_add(filelist, fpath);
            }
        }
    } while (FindNextFile(hFind, &find_file_data) != 0);

    FindClose(hFind);
#else
    DIR *dir;
    struct dirent *entry;
    struct stat info;

    if (!(dir = opendir(path))) return; // Error opening directory

    while ((entry = readdir(dir)) != NULL) {
        char fpath[PATH_MAX];
        snprintf(fpath, sizeof(fpath), "%s%s", path, entry->d_name);
        if (stat(fpath, &info) == 0) {
            if (S_ISDIR(info.st_mode)) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    strcat(fpath, "/");
                    list_files_recursively(fpath, filelist, flags);
                }
            } else if (S_ISREG(info.st_mode)) {
                filelist_add(filelist, fpath);
            }
        }
    }

    closedir(dir);
#endif
}

#ifdef _WIN32

/**
 * Matches a pattern against a string.
 *
 * @param pattern           The pattern to match.
 * @param string            The string to match against.
 * @param flags             Flags
 *
 * @return                  1 if the pattern matches, 0 otherwise.
 */

static int match(const char *pattern, const char *string, uint16_t flags) {
    while (*pattern && *string) {
        if (*pattern == '*') {
            pattern++;
            while (*string) {
                if (match(pattern, string, flags)) return 1;
                string++;
            }
            return *pattern == '\0';
        } else if ( (
                      ( ( flags & FLAG_ICASE ) && tolower(*pattern) != tolower(*string) ) ||
                      (!( flags & FLAG_ICASE ) &&          *pattern != *string          )
                    ) && *pattern != '?') {
            return 0;
        }
        pattern++;
        string++;
    }
    while (*pattern == '*') pattern++;

    return  ( ( flags & FLAG_ICASE ) && tolower(*pattern) == tolower(*string) ) ||
            (!( flags & FLAG_ICASE ) &&          *pattern == *string          );
}

/**
 * Processes a directory pattern.
 *
 * @param filelist              The FILELIST structure to store the file list.
 * @param fragment_paths        An array of FRAGMENT_PATH structures.
 * @param num_fragment_paths    The number of fragment paths.
 * @param base_dir              The base directory.
 * @param flags                 Flags
 */

static void process_directory_pattern(FILELIST *filelist, FRAGMENT_PATH *fragment_paths, int num_fragment_paths, char *base_dir, uint16_t flags) {
    char result[PATH_MAX];
    WIN32_FIND_DATA find_file_data;
    HANDLE hFind;

    if (!num_fragment_paths) return;

    char current_dir[PATH_MAX];
    char *search = NULL;
    char *pattern = NULL;

    snprintf(current_dir, sizeof(current_dir), "%s%s%s", base_dir ? base_dir : "", base_dir && *base_dir ? "" : "", fragment_paths[0].pattern);

    char *path_sep1 = strrchr(current_dir, '/');
    char *path_sep2 = strrchr(current_dir, '\\');
    char *path_sep = (path_sep1 > path_sep2) ? path_sep1 : path_sep2;
    if (!path_sep) path_sep = current_dir;
    if (*path_sep == '\\' || *path_sep == '/') path_sep++;

    pattern = strdup(path_sep);

    if (path_sep) *path_sep = '\0';

    if (strchr(pattern, '*') || strchr(pattern, '?')) {
        int l = strlen(current_dir) + 2;
        search = malloc(l);
        snprintf(search, l, "%s*", current_dir);
    } else {
        if (base_dir && *base_dir) {
            int l = strlen(base_dir) + strlen(fragment_paths[0].pattern) + 2;
            search = malloc(l);
            snprintf(search, l, "%s%s", base_dir, fragment_paths[0].pattern);
        } else {
            search = strdup(fragment_paths[0].pattern);
        }
    }

    int is_parent = !strcmp(pattern, "..");
    int is_current = !strcmp(pattern, ".");

    hFind = FindFirstFile(search, &find_file_data);
    if (hFind == INVALID_HANDLE_VALUE && !fragment_paths[0].is_dir) {
        if (is_parent) {
            DWORD attr = GetFileAttributes(search);
            if (attr & FILE_ATTRIBUTE_DIRECTORY) {
                snprintf(result, sizeof(result), "%s/", search);
                filelist_add(filelist, result);
                free(pattern);
                free(search);
                return;
            }
        }
    }

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(find_file_data.cFileName, ".") != 0 && strcmp(find_file_data.cFileName, "..") != 0) {
                int is_match = match(pattern, find_file_data.cFileName, flags);
                if (is_parent || is_current || is_match) {
                    int is_real_dir = find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
                    if (fragment_paths[0].is_dir || is_real_dir) {
                        if (fragment_paths[0].is_dir) {
                            if (is_real_dir) {
                                if (is_parent) {
                                    strcpy(path_sep, "..");
                                } else if (is_current) {
                                    strcpy(path_sep, ".");
                                } else {
                                    strcpy(path_sep, find_file_data.cFileName);
                                }
                                process_directory_pattern(filelist, &fragment_paths[1], num_fragment_paths - 1, current_dir, flags);
                            }
                        } else {
                            if ( flags & FLAG_RECURSIVE ) {
                                if (is_parent) {
                                    snprintf(result, sizeof(result), "%s../", current_dir);
                                } else if (is_current) {
                                    snprintf(result, sizeof(result), "%s./", current_dir);
                                } else {
                                    snprintf(result, sizeof(result), "%s%s/", current_dir, find_file_data.cFileName);
                                }
                                list_files_recursively(result, filelist, flags);
                            }
                        }
                    } else {
                        snprintf(result, sizeof(result), "%s%s", current_dir, find_file_data.cFileName);
                        filelist_add(filelist, result);
                    }
                }
            }
        } while (FindNextFile(hFind, &find_file_data) != 0);
        FindClose(hFind);
    }
    free(pattern);
    free(search);
}

#endif

/**
 * Processes a pattern and lists files accordingly.
 *
 * @param pattern           The pattern to process.
 * @param filelist          The FILELIST structure to store the file list.
 * @param flags             Flags
 *
 * @return                  0 if successful, -1 on error.
 */

int process_pattern(char *pattern, FILELIST *filelist, uint16_t flags) {
#ifdef _WIN32
    int include_first_slash = ( *pattern == '/' || *pattern == '\\' )
                              && !( strlen(pattern) > 2 && ( !strncmp(&pattern[1],":/",2) || !strncmp(&pattern[1],":\\",2) ) ) ;
    char *fragment = strtok((char *)pattern, "/\\");

    int num_fragment_paths = 0;
    FRAGMENT_PATH fragment_paths[256];

    memset(fragment_paths, '\0', sizeof(fragment_paths));

    int error = 0;
    int part = 0;
    while (fragment) {
        int is_drive = 0;

        if (part || include_first_slash) strcat(fragment_paths[num_fragment_paths].pattern, "/");
        include_first_slash = 0;
        strcat(fragment_paths[num_fragment_paths].pattern, fragment);

        if (!part && strrchr(fragment, ':')) is_drive = 1;

        part++;

        fragment = strtok(NULL, "/\\");
        if (!is_drive || !fragment) {
            if (fragment) fragment_paths[num_fragment_paths].is_dir = 1;
            num_fragment_paths++;
            if (fragment && num_fragment_paths == sizeof(fragment_paths) / sizeof(fragment_paths[0])) {
                error = 1;
                break;
            }
        }
    }

    if (error) {
//        fprintf(stderr, "The search pattern refers to too many directories.\n");
        return -1;
    }

    process_directory_pattern(filelist, fragment_paths, num_fragment_paths, NULL, flags);

#else
    glob_t glob_result;

    if ( flags & FLAG_ICASE ) pattern = icase_pattern(pattern);

    int pattern_length = strlen(pattern);
    if ( pattern_length > 1 && pattern[pattern_length-1] == '/') {
        pattern[pattern_length-1] = '\0';
        pattern_length--;
    } 
    int is_parent = ( pattern_length >= 2 && !strcmp(pattern + pattern_length - 2, ".." ) ) ? 1 : 0;
    int is_current = ( !is_parent && pattern_length >= 1 && !strcmp(pattern + pattern_length - 1, "." ) ) ? 1 : 0;
    int oflags = GLOB_TILDE | GLOB_BRACE | GLOB_MARK | GLOB_PERIOD;
    if (!glob(pattern, oflags, NULL, &glob_result)) {
        for (size_t i = 0; i < glob_result.gl_pathc; i++) {
            char *current_entry = glob_result.gl_pathv[i];
            int entry_length = strlen(current_entry);
            int entry_is_parent = ( entry_length >= 3 && !strcmp(current_entry + entry_length - 3, "../" ) ) ? 1 : 0;
            int entry_is_current = ( !entry_is_parent && entry_length >= 2 && !strcmp(current_entry + entry_length - 2, "./" ) ) ? 1 : 0;

            if ( ( !entry_is_parent && !entry_is_current ) || 
                  ( is_parent && entry_is_parent ) ||
                  ( is_current && entry_is_current ) ) {

                struct stat info;
                if (stat(current_entry, &info) == 0) {
                    if (S_ISDIR(info.st_mode)) {
                        if ( flags & FLAG_RECURSIVE ) list_files_recursively(current_entry, filelist, flags);
                    } else if (S_ISREG(info.st_mode)) {
                        filelist_add(filelist, current_entry);
                    }
                }
            }
        }
        globfree(&glob_result);
    }

    if ( flags & FLAG_ICASE ) free( pattern );
#endif
    return 0;
}
