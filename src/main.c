/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file main.c
 * @brief Main program entry for the PSARc project.
 *
 * This file serves as the main program entry point for the PSARc project. Its primary
 * responsibility is to parse command-line arguments and invoke functions to handle PSARC
 * archive operations, such as extraction and manipulation.
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
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <lzma.h>

#include "common.h"
#include "pak.h"
#include "unpak.h"
#include "file_utils.h"
#include "threads.h"

// Define command-line options
static struct option long_options[] = {
    { "create", no_argument, 0, 'c' },
    { "extract", no_argument, 0, 'x' },
    { "list", no_argument, 0, 'l' },
    { "info", no_argument, 0, 'i' },
    { "file", required_argument, 0, 'f' },
    { "block-size", required_argument, 0, 'b' },
    { "recursive", no_argument, 0, 'r' },
    { "gzip", no_argument, 0, 'z' },
    { "lzma", no_argument, 0, 'j' },
    { "fast", no_argument, 0, '1' },
    { "best", no_argument, 0, '9' },
    { "extreme", no_argument, 0, 'e' },
    { "ignore-case", no_argument, 0, 'I' },
    { "absolute-paths", no_argument, 0, 'A' },
    { "source-dir", required_argument, 0, 's' },
    { "target-dir", required_argument, 0, 't' },
    { "trim-path", no_argument, 0, 'T' },
    { "overwrite", no_argument, 0, 'y' },
    { "skip-existing-files", no_argument, 0, 'S' },
    { "num-threads", required_argument, 0, 'n' },
    { "output-format", required_argument, 0, 'o' },
    { "verbose", no_argument, 0, 'v' },
    { "help", no_argument, 0, 'h' },
    { "version", no_argument, 0, 'V' },
    { 0, 0, 0, 0 }
};

// Table mapping format names to enum values
static struct {
    char *name;
    enum FORMAT_VALUE_ENUM value;
} format_mappings[] = {
    {"json", JSON_FORMAT},
    {"csv", CSV_FORMAT},
    {"xml", XML_FORMAT},
    // Add more formats if needed
    {NULL, UNKNOWN_FORMAT} // Marks the end of the table
};

int main( int argc, char *argv[] ) {
    int exit_value = EXIT_FAILURE;
    char *archive_file = NULL;
    int mode = 0; // 1 for create, 2 for extract, 3 for list, 4 for info
    int mode_count = 0;
    int compression_count = 0;
    int compression_level_count = 0;

    _Config.num_threads = threads_get_max(); // Default number of threads

    int option;
    while ( ( option = getopt_long( argc, argv, "cxlif:b:zj0123456789eIAs:t:rTySn:o:vhV", long_options, NULL ) ) != -1 ) {
        switch ( option ) {
            case 'c':
                if ( mode != 1 ) mode_count++;
                mode = 1; // Set mode to create
                break;

            case 'x':
                if ( mode != 2 ) mode_count++;
                mode = 2; // Set mode to extract
                break;

            case 'l':
                if ( mode != 3 ) mode_count++;
                mode = 3; // Set mode to list
                break;

            case 'i':
                if ( mode != 4 ) mode_count++;
                mode = 4; // Set mode to info
                break;

            case 'f':
                _Config.archive_file = archive_file = optarg; // Set the file path
                break;

            case 'b':
                _ArchiveInfo.block_size = atoi( optarg ); // Set block size
                break;

            case 'z':
                _ArchiveInfo.compression_type = PSARC_ZLIB; // Set compression type to zlib
                if ( _ArchiveInfo.compression_type != PSARC_ZLIB ) compression_count++;
                break;

            case 'j':
                _ArchiveInfo.compression_type = PSARC_LZMA; // Set compression type to lzma
                if ( _ArchiveInfo.compression_type != PSARC_LZMA ) compression_count++;
                break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                _Config.compression_level = option - '0'; // Set compression level
                compression_level_count++;
                break;

            case 'e':
                _Config.extreme_compression_flag = 1;
                break;

            case 'I': // --ignore-case
                _ArchiveInfo.archive_flags |= AF_ICASE; // Set the ignore-case flag
                break;

            case 'A': // --absolute-paths
                _ArchiveInfo.archive_flags |= AF_ABSPATH; // Set the absolute-paths flag
                break;

            case 's':
                _Config.source_dir = optarg; // Source directory
                break;

            case 't':
                _Config.target_dir = optarg; // Target directory
                break;

            case 'r':
                _Config.recursive_flag = 1; // Set recursive flag
                break;

            case 'T':
                _Config.trim_path_flag = 1; // Remove path from path file name flag
                break;

            case 'y':
                _Config.overwrite_flag = 1; // Set overwrite flag
                break;

            case 'S':
                _Config.skip_existing_files_flag = 1; // Skip existing files flag
                break;

            case 'n':
                _Config.num_threads = atoi( optarg ); // Number of threads
                printf("num_threads %d\n", _Config.num_threads);
                break;

            case 'o':
                _Config.output_format = UNKNOWN_FORMAT;
                // Search for the numeric value in the mapping table
                for (int i = 0; format_mappings[i].name; i++) {
                    if (strcmp(optarg, format_mappings[i].name) == 0) {
                        _Config.output_format = format_mappings[i].value;
                        break;
                    }
                }
                // If the format is not found, display an error
                if (_Config.output_format == UNKNOWN_FORMAT) {
                    fprintf( stderr, APPNAME": Invalid output format: %s\n", optarg );
                    fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );
                    return 1;
                }
                break;

            case 'v':
                _Config.verbose_flag = 1; // Set verbose flag
                break;

            case 'h': // --help
                printf( "PSARc v1.0 - (c) 2023 Juan José Ponteprino (SplinterGU)\n\n" );
                printf( "Usage: %s [options] [file]...\n", argv[0] );
                printf( "\nExamples:\n" );
                printf( "  %s -cf archive.pak foo bar  # Create (store) archive.pak from foo and bar.\n", argv[0] );
                printf( "  %s -czf archive.pak foo bar # Create (zlib) archive.pak from foo and bar.\n", argv[0] );
                printf( "  %s -lf archive.pak          # List files in archive.pak.\n", argv[0] );
                printf( "  %s -xf archive.pak          # Extract all files from archive.pak.\n", argv[0] );
                printf( "\nOptions:\n\n" );
                printf( " Operation mode:\n" );
                printf( "  -c, --create                 create an archive\n" );
                printf( "  -x, --extract                extract files\n" );
                printf( "  -l, --list                   list contents\n" );
                printf( "  -i, --info                   show archive information\n" );
                printf( "\n" );
                printf( " Operation modifiers:\n" );
                printf( "  -f, --file=FILE              specify file (mandatory)\n" );
                printf( "  -b, --block-size=BYTES       block size in bytes (default: 65536)\n" );
                printf( "\n" );
                printf( " Compression (default: no compression -store-):\n" );
                printf( "  -z, --zlib                   use zlib\n" );
                printf( "  -j, --lzma                   use lzma\n" );
                printf( "  -0                           compress faster (only for lzma)\n" );
                printf( "  -1, --fast                   compress faster\n" );
                printf( "  -9, --best                   compress better\n" );
                printf( "  -e, --extreme                extreme compress (only for lzma)\n" );
                printf( "\n" );
                printf( " Archive flags (default: relative paths and case-sensitive):\n" );
                printf( "  -I, --ignore-case            ignore case when matching file selection patterns\n" );
                printf( "                               during creation\n" );
                printf( "                               (ignored during extraction, uses creation setting)\n" );
                printf( "  -A, --absolute-paths         use absolute paths for file names\n" );
                printf( "\n" );
                printf( " File name selection:\n" );
                printf( "  -s, --source-dir=DIR         base directory for source files\n" );
                printf( "  -t, --target-dir=DIR         directory where extracted files will be placed\n" );
                printf( "  -r, --recursive              recurse into directories\n" );
                printf( "  -T, --trim-path              remove all file paths from/to the archive\n");
                printf( "\n" );
                printf( " Overwrite control:\n" );
                printf( "  -y, --overwrite              force overwrite of output file\n" );
                printf( "  -S, --skip-existing-files    don't replace existing files when extracting,\n" );
                printf( "                               silently skip over them\n" );
                printf( "\n" );
                printf( " Other options:\n" );
                printf( "  -n, --num-threads=NUM        specify the number of threads (default: auto, based on CPU cores)\n" );
                printf( "  -o, --output-format=FORMAT   specify the output format for information display\n" );
                printf( "                               available formats:\n" );
                printf( "                                   json\n" );
                printf( "                                   csv\n" );
                printf( "                                   xml\n" );
                printf( "  -v, --verbose                list processed files in detail\n" );
                printf( "  -h, --help                   show this help\n" );
                printf( "  -V, --version                show program version\n" );
                printf( "\n" );
                printf( "This software is provided under the terms of the MIT License.\n" );
                printf( "You may freely use, modify, and distribute this software, subject\n" );
                printf( "to the conditions and limitations of the MIT License.\n\n" );
                printf( "For more details, please see the LICENSE file included with this\n" );
                printf( "software.\n\n" );
                printf( "Report bugs to: splintergu@gmail.com\n" );
                printf( "Home page: <https://github.com/SplinterGU/PSARc>\n" );

                exit( 0 );

            case 'V': // --version
                // Display program version
                printf( "psar (PSARc) "VERSION"\n" );
                printf( "Copyright (C) 2023 Juan José Ponteprino (SplinterGU)\n" );
                printf( "License MIT: MIT License <https://opensource.org/licenses/MIT>.\n" );
                printf( "This is open-source software: you are free to change and redistribute it.\n" );
                printf( "There is NO WARRANTY, to the extent permitted by law.\n\n" );
                printf( "Written by Juan José Ponteprino (SplinterGU)\n" );
                printf( "Report bugs/contact to: splintergu@gmail.com\n" );
                printf( "Home page: <https://github.com/SplinterGU/PSARc>\n" );

                exit( 0 );

            default:
                // Handle invalid options
                fprintf( stderr, "Usage: %s [options] files...\n", argv[0] );
                fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

                return 1;
        }
    }

    // Check for valid mode selection
    if ( !mode || mode_count != 1 ) {
        fprintf( stderr, APPNAME": you must specify one operation mode\n" );
        fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

        return 1;
    }

    // Check for valid compression count
    if ( compression_count > 1 ) {
        fprintf( stderr, APPNAME": you must specify exactly one compression type\n" );
        fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

        return 1;
    }

    // Check for valid compression count and mode combination
    if ( compression_count && mode != 1 ) {
        fprintf( stderr, APPNAME": compression type is only for create mode\n" );
        fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

        return 1;
    }

    // Check for the presence of a file path
    if ( !archive_file ) {
        fprintf( stderr, APPNAME": you must specify an archive file\n" );
        fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

        return 1;
    }

    char *current_dir = NULL;
    char *new_archive = NULL;

    // Implement logic specific to each mode
    switch ( mode ) {
        case 1: { // Create mode ( -c )
            // Check for parameters specific to the create mode
            if ( _ArchiveInfo.block_size <= 0 ) { 
                fprintf( stderr, APPNAME": block size must be a positive integer for create mode\n" );
                fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

                return 1;
            }

            // Check for parameters specific to the create mode
            if ( optind >= argc ) { 
                fprintf( stderr, APPNAME": no files to add\n");
                fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

                return 1;
            }

            if ( _ArchiveInfo.compression_type == PSARC_LZMA ) {
                // Set default compression level for create mode with LZMA
                if ( compression_level_count == 0 ) _Config.compression_level = LZMA_PRESET_DEFAULT;
            } else {
                if ( _Config.compression_level == 0 ) {
                    fprintf( stderr, APPNAME": invalid compression level\n" );
                    fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

                    return 1;
                }
                if ( _Config.extreme_compression_flag ) {
                    fprintf( stderr, APPNAME": extreme compression isn't valid option for zlib\n" );
                    fprintf( stderr, "Try '%s --help' for more information.\n", argv[0] );

                    return 1;
                }
            }

            // Make full path for archive
            char * adir = strdup(archive_file);
            if ( !adir ) {
                fprintf( stderr, APPNAME": no enough memory\n" );

                return 1;
            }

            char * directory = dirname(adir);
            int mkr = mkpath(directory, 0777);
            free(adir);
            if ( mkr ) {
                fprintf( stderr, APPNAME": can't possible to create target directory\n" );

                return 1;
            }

            if ( _Config.source_dir ) {
                new_archive = fullpath(archive_file);
                if (!new_archive) {
                    fprintf( stderr, APPNAME": no enough memory\n" );

                    return 1;
                }

                archive_file = new_archive;

                current_dir = get_current_dir();
                if ( !current_dir ) {
                    fprintf( stderr, APPNAME": no enough memory\n" );

                    free( new_archive );

                    return 1;
                }
                int dummy = chdir(_Config.source_dir);
                dummy = dummy;
            }

            FILELIST filelist;
            filelist_init(&filelist, 100, 65536);

            for ( int i = optind; i < argc; i++ ) {
                if ( process_pattern(argv[i], &filelist, ( _Config.recursive_flag ? FLAG_RECURSIVE : 0 ) | ( ( _ArchiveInfo.archive_flags & AF_ICASE ) ? FLAG_ICASE : 0 ) ) ) {
                    // The search pattern refers to too many directories
                    if ( current_dir ) {
                        int dummy = chdir(current_dir);
                        dummy = dummy;
                        free(current_dir);
                    }

                    free( new_archive );

                    return 1;
                }
            }

            if ( !filelist.count ) {
                fprintf( stderr, APPNAME": no matching files found to create an archive\n" );

                filelist_free(&filelist);

                if ( current_dir ) {
                    int dummy = chdir(current_dir);
                    dummy = dummy;
                    free(current_dir);
                }

                free( new_archive );

                return 1;
            }

            exit_value = create_archive( archive_file, filelist.files, filelist.count );

            filelist_free(&filelist);

            break;
        }

        case 2: // Extract mode ( -x )
            if ( _Config.target_dir ) {
                if ( mkpath(_Config.target_dir, 0777) ) {
                    fprintf( stderr, APPNAME": can't possible to create target directory\n" );

                    return 1;
                }

                new_archive = fullpath(archive_file);
                if (!new_archive) {
                    fprintf( stderr, APPNAME": no enough memory\n" );

                    return 1;
                }
                archive_file = new_archive;

                current_dir = get_current_dir();
                if ( !current_dir ) {
                    fprintf( stderr, APPNAME": no enough memory\n" );

                    free(new_archive);

                    return 1;
                }
                int dummy = chdir(_Config.target_dir);
                dummy = dummy;
            }

        case 3: // List mode ( -l )
        case 4: // Info mode ( -i )
            exit_value = process_archive( archive_file, mode, &argv[optind], argc - optind );
            break;

        default:
            // This should not happen, as mode is ensured to be one of the valid options
            break;
    }

    if ( current_dir ) {
        int dummy = chdir(current_dir);
        dummy = dummy;
        free(current_dir);
    }

    free( new_archive );

    return exit_value; 
 }
