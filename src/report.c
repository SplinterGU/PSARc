/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file report.c
 * @brief Implementation of the unpak utility for the PSARc project.
 *
 * This file implements the unpak utility, which is used to extract files from PSARC archives
 * in the PSARc project. It provides functionality for unpacking archive contents.
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
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <locale.h>
#include <libgen.h>
#include <stdarg.h>
#include <math.h>

#include "common.h"
#include "psarc.h"
#include "report.h"

/**
 * Prints formatted output with custom placeholders.
 *
 * This function allows for custom formatting by specifying placeholders in the format string.
 * The supported placeholders are:
 *   - '%H':  Prints md5hash
 *   - '%T':  Prints the compression type based on the integer argument (e.g., PSARC_STORE, PSARC_ZLIB, PSARC_LZMA).
 *   - '%M':  Prints "stored" or "deflated" based on two uint64_t arguments (partial and total)(for compression).
 *   - '%m':  Prints "extracting" or "inflating" based on two uint64_t arguments (partial and total)(for decompression).
 *   - '%R':  Prints the compression ratio as a percentage based on two uint64_t arguments (partial and total).
 *   - '%L':  Prints a uint64_t value.
 *   - '%d':  Prints a int value.
 *   - '%s':  Prints a char * value.
 *
 * NOTE: All placeholders (except '%H') support an optional width specified as a number (e.g., %13L) to control the field width.
 *
 * @param mask      The format string containing placeholders.
 * @param ...       Additional arguments to replace the placeholders.
 */

static void printc(const char *mask, ...) {
    va_list args;
    va_start(args, mask);

    int width;

    char c;
    while ((c = *mask)) {
        width = 0;
        if (c == '%') {
            c = *(++mask);
            if (*mask >= '0' && *mask <= '9') {
                while (*mask >= '0' && *mask <= '9') {
                    width = width * 10 + (*mask - '0');
                    mask++;
                }
                c = *(mask);
            }

            switch (c) {
                case 'H': {
                    unsigned char *hash = va_arg(args, uint8_t *);
                    for (int i = 0; i < 16; i++) printf("%02x", (unsigned char)hash[i]);
                    break;
                }

                case 'T': {
                    int type = va_arg(args, int);
                    switch (type) {
                        case PSARC_STORE:
                            printf("%*s", width, "store");
                            break;

                        case PSARC_ZLIB:
                            printf("%*s", width, "zlib");
                            break;

                        case PSARC_LZMA:
                            printf("%*s", width, "lzma");
                            break;

                        default:
                            break;
                    }
                    break;
                }

                case 'M': {
                    uint64_t partial = va_arg(args, uint64_t);
                    uint64_t total = va_arg(args, uint64_t);
                    printf("%*s", width, total == partial ? "stored" : "deflated");
                    break;
                }
    
                case 'm': {
                    uint64_t partial = va_arg(args, uint64_t);
                    uint64_t total = va_arg(args, uint64_t);
                    printf("%*s", width, total == partial ? "extracting" : "inflating");
                    break;
                }
    
                case 'R': {
                    double value = va_arg(args, double);
                    printf("%*.02f", width, isnan(value) ? 0 : 100.0f - value * 100.0f);
                    break;
                }

                case 'L': {
                    printf("%*"PRId64, width, va_arg(args, uint64_t));
                    break;
                }

                case 'd': {
                    printf("%*d", width, va_arg(args, int));
                    break;
                }

                case 's': {
                    printf("%*s", width, va_arg(args, char *));
                    break;
                }

                default: {
                    putchar(c);
                    break;
                }
            }
        } else {
            putchar(c);
        }
        mask++;
    }

    va_end(args);
}

/**
 * Opens a report of the specified type for the given archive.
 *
 * This function opens a report of the specified type for the provided archive name.
 *
 * @param type           The type of report (e.g., PAK, UNPAK, LIST).
 * @param archive_name   The name of the archive.
 *
 * @return               A pointer to the REPORT structure representing the opened report,
 *                       or NULL in case of an error.
 */

static const char *report_open_mask[] = {
//    "archive: %s\n", // STANDARD_FORMAT
    "%s:\n", // STANDARD_FORMAT

    "{\"archive\":\"%s\"", // JSON_FORMAT

    "type_record,archive_name,files_name,files_name_digest,files_compression_method,files_uncompressed,files_compressed,files_savings,files_status,total_files,total_uncompressed,total_compressed,total_savings,total_errors,error_message\n"
    "archive,%s\n", // CSV_FORMAT

    "<?xml version=\"1.0\" encoding=\"UTF-8\"?><archive><archive>%s</archive>" // XML_FORMAT
};

REPORT * report_open(REPORT_TYPE type, const char *archive_name) {
    REPORT *report = (REPORT *)malloc(sizeof(REPORT));
    if (!report) return NULL;

    int idx = _Config.output_format;
    if ( idx > XML_FORMAT || idx < 0 ) idx = 0;
    printc(report_open_mask[idx], archive_name);

    report->type = type;
    report->last_operation = REPORT_OPEN;

    return report;
}

/**
 * Closes a report of the specified type, including compression statistics and totals.
 *
 * This function closes a report of the specified type and prints the closing information,
 * including compression statistics and totals, depending on the report type and output format.
 *
 * @param report                    The report structure to which the error is associated.
 * @param show_totals               Whether to display the totals
 * @param total_compressed          The total size of compressed data.
 * @param total_uncompressed        The total size of uncompressed data.
 * @param manifest_compressed       The size of compressed manifest data.
 * @param manifest_uncompressed     The size of uncompressed manifest data.
 * @param successful                The number of successful operations.
 * @param errors                    The number of errors encountered.
 */

static const char *report_close_pak_mask[] = {                        
    "%d files\ntotal uncompressed=%L -> compressed=%L (%R%% savings)\n", // STANDARD_FORMAT
    ",\"totals\": {\"files\":%d,\"uncompressed\":%L,\"compressed\":%L,\"savings\":%R}", // JSON_FORMAT
    "totals,,,,,,,,,%d,%L,%L,%R\n", // CSV_FORMAT
    "<totals><files>%d</files><uncompressed>%L</uncompressed><compressed>%L</compressed><savings>%R</savings></totals>" // XML_FORMAT
};

static const char *report_close_unpak_mask[] = {
    "%d files\nbytes=%L errors=%d\n", // STANDARD_FORMAT
    ",\"totals\":{\"files\":%d,\"uncompressed\":%L,\"errors\":%d}", // JSON_FORMAT
    "totals,,,,,,,,,%d,%L,,,%d\n", // CSV_FORMAT
    "<totals><files>%d</files><uncompressed>%L</uncompressed><errors>%d</errors></totals>" // XML_FORMAT
};

static const char *report_close_list_mask[] = {
    "%d files\n", // STANDARD_FORMAT
    ",\"total_files\":%d", // JSON_FORMAT
    "totals,,,,,,,,,%d\n", // CSV_FORMAT
    "<total_files>%d</total_files>" // XML_FORMAT
};

void report_close(REPORT *report, int show_totals, uint64_t total_compressed, uint64_t total_uncompressed, uint64_t manifest_compressed, uint64_t manifest_uncompressed, uint32_t successful, uint32_t errors) {
    if ( report && show_totals ) {
        int idx = _Config.output_format;
        if (idx > XML_FORMAT || idx < 0) idx = 0;

        switch ( report->type ) {
            case    REPORT_TYPE_PAK:
                printc(report_close_pak_mask[idx],
                    successful,
                    total_uncompressed,
                    total_compressed,
                    (double) total_compressed / total_uncompressed
                    );
                break;

            case    REPORT_TYPE_UNPAK:
                printc(report_close_unpak_mask[idx],
                    successful,
                    total_uncompressed,
                    errors
                    );
                break;

            case    REPORT_TYPE_LIST:
                printc(report_close_list_mask[idx],
                    successful
                    );
                break;

            case    REPORT_TYPE_INFO:
                break;

            default:
                break;
        }
    }

    switch ( _Config.output_format ) {
        default:
            break;

        case JSON_FORMAT:
            printf("}");
            break;

        case XML_FORMAT:
            printf("</archive>");
            break;
    }

    free(report);
}


/**
 * Marks the open of a section in the report for listing files.
 *
 * This function marks the open of a section in the report for listing files. The behavior
 * depends on the report type and output format.
 *
 * @param report  The report structure to which the error is associated.
 */

void report_open_file_section(REPORT *report) {
    if (!report) return;

    switch ( _Config.output_format ) {
        default:
            break;

        case STANDARD_FORMAT:
            if (report->type == REPORT_TYPE_LIST ) {
                if ( _Config.verbose_flag )
                    printf("   Compressed  Uncompressed   Method Saving Name digest                      Name\n" \
                           "------------- ------------- -------- ------ -------------------------------- ------------------------\n");
                else
                    printf(" Uncompressed Name\n" \
                           "------------- ------------------------\n");
            }
            break;

        case JSON_FORMAT:
            // In JSON format, open a list of files.
            printf(",\"files\":[");
            break;

        case XML_FORMAT:
            // In XML format, open a container for files.
            printf("<files>");
            break;
    }

    report->last_operation = REPORT_OPEN_FILE_SECTION;
}

/**
 * Marks the end of the file section in the report.
 *
 * This function marks the end of the file section in the report. The behavior depends
 * on the report type and output format.
 *
 * @param report  The report structure to which the error is associated.
 */

void report_close_file_section(REPORT *report) {
    if (!report) return;

    switch ( _Config.output_format ) {
        default:
            break;

        case STANDARD_FORMAT:
            if (report->type == REPORT_TYPE_LIST ) {
                if ( _Config.verbose_flag )
                    printf("------------- ------------- -------- ------ -------------------------------- ------------------------\n");
                else
                    printf("------------- ------------------------\n");
            }
            break;

        case JSON_FORMAT:
            // In JSON format, close the list of files.
            printf("]");
            break;

        case XML_FORMAT:
            // In XML format, close the container for files.
            printf("</files>");
            break;
    }

    report->last_operation = REPORT_CLOSE_FILE_SECTION;
}

/**
 * Marks the open of describing an individual file within the file section of the report.
 *
 * This function marks the open of describing an individual file within the file section
 * of the report. The behavior depends on the report type, output format, and file information.
 *
 * @param report    The report structure to which the error is associated.
 * @param fi        Pointer to a FILEINFO structure containing file information.
 */

static const char *report_open_file_item_pak_mask[] = {
    "adding: %s", // STANDARD_FORMAT
    "{\"name\":\"%s\",", // JSON_FORMAT
    "files,,%s,", // CSV_FORMAT
    "<file><name>%s</name>" // XML_FORMAT
};

static const char *report_open_file_item_unpak_mask[] = {
    "%m: %s...", // STANDARD_FORMAT
    "{\"name\":\"%s\",\"compression_method\":\"%m\"", // JSON_FORMAT
    "files,,%s,,%m,", // CSV_FORMAT
    "<file><name>%s</name><compression_method>%m</compression_method>" // XML_FORMAT
};

static const char *report_open_file_item_unpak_verbose_mask[] = {
    "%m: %s (%L bytes)...", // STANDARD_FORMAT
    "{\"name\":\"%s\",\"compression_method\":\"%m\",\"uncompressed\":%L", // JSON_FORMAT
    "files,,%s,,%m,%L", // CSV_FORMAT
    "<file><name>%s</name><compression_method>%m</compression_method><uncompressed>%L</uncompressed>" // XML_FORMAT
};

static const char *report_file_item_list_mask[] = {
    "%13L %s\n", // STANDARD_FORMAT
    "{\"name\":\"%s\",\"uncompressed\":%L}", // JSON_FORMAT
    "files,,%s,,,%L\n", // CSV_FORMAT
    "<file><name>%s</name><uncompressed>%L</uncompressed></file>" // XML_FORMAT
};

static const char *report_file_item_list_verbose_mask[] = {
    "%13L %13L %8M %5R%% %H %s\n", // STANDARD_FORMAT
    "{\"name\":\"%s\",\"name_digest\":\"%H\",\"compression_method\":\"%M\",\"uncompressed\":%L,\"compressed\":%L,\"savings\":%R}", // JSON_FORMAT
    "files,,%s,%H,%M,%L,%L,%R\n", // CSV_FORMAT
    "<file><name>%s</name><name_digest>%H</name_digest><compression_method>%M</compression_method><uncompressed>%L</uncompressed><compressed>%L</compressed><savings>%R</savings></file>" // XML_FORMAT
};

void report_open_file_item(REPORT *report, FILEINFO *fi) {
    if (!report) return;

    int idx = _Config.output_format;
    if ( idx > XML_FORMAT || idx < 0 ) idx = 0;

    switch ( _Config.output_format ) {
        case STANDARD_FORMAT:
            switch ( report->type ) {
                case    REPORT_TYPE_PAK:
                    printc(report_open_file_item_pak_mask[idx],
                            fi->filename
                        );
                    break;

                case    REPORT_TYPE_UNPAK:
                    if ( _Config.verbose_flag ) {
                        printc(report_open_file_item_unpak_verbose_mask[idx],
                                fi->uncompressed_size, fi->compressed_size,
                                fi->filename,
                                fi->uncompressed_size
                            );
                    } else {
                        printc(report_open_file_item_unpak_mask[idx],
                                fi->uncompressed_size, fi->compressed_size,
                                fi->filename
                            );
                    }
                    break;

                case    REPORT_TYPE_LIST:
                    if ( _Config.verbose_flag ) {
                        printc(report_file_item_list_verbose_mask[idx],
                                fi->compressed_size,
                                fi->uncompressed_size,
                                fi->uncompressed_size, fi->compressed_size,
                                (double) fi->compressed_size / fi->uncompressed_size,
                                fi->name_digest,
                                fi->filename
                            );
                    } else {
                        printc(report_file_item_list_mask[idx],
                                fi->uncompressed_size,
                                fi->filename
                            );
                    }
                    break;

                default:
                    break;
            }
            break;
     
        default: // non STANDARD_FORMAT
          switch ( report->type ) {
                case    REPORT_TYPE_PAK:
                    printc(report_open_file_item_pak_mask[idx],
                            fi->filename
                        );
                    break;

                case    REPORT_TYPE_UNPAK:
                    if ( _Config.verbose_flag ) {
                        printc(report_open_file_item_unpak_verbose_mask[idx],
                                fi->filename,
                                fi->uncompressed_size, fi->compressed_size,
                                fi->uncompressed_size
                            );
                    } else {
                        printc(report_open_file_item_unpak_mask[idx],
                                fi->filename,
                                fi->uncompressed_size, fi->compressed_size
                            );
                    }
                    break;

                case    REPORT_TYPE_LIST:
                    if ( _Config.verbose_flag ) {
                        printc(report_file_item_list_verbose_mask[idx],
                                fi->filename,
                                fi->name_digest,
                                fi->uncompressed_size, fi->compressed_size,
                                fi->uncompressed_size,
                                fi->compressed_size,
                                (double) fi->compressed_size / fi->uncompressed_size
                            );
                    } else {
                        printc(report_file_item_list_mask[idx],
                                fi->filename,
                                fi->uncompressed_size
                            );
                    }
                    break;

                default:
                    break;
            }
            break;
    }

    report->last_operation = REPORT_OPEN_FILE_ITEM;
}

/**
 * Marks the end of describing an individual file within the file section of the report.
 *
 * This function marks the end of describing an individual file within the file section
 * of the report. The behavior depends on the report type, output format, and file information.
 *
 * @param report                The report structure to which the error is associated.
 * @param uncompressed_size     The uncompressed size of the file.
 * @param compressed_size       The compressed size of the file.
 * @param status                The status of the file operation.
 * @param is_not_last           Flag indicating whether this is the last file in the section.
 */

static const char *report_close_file_item_pak_mask[] = {
    " (%M %R%%)\n", // STANDARD_FORMAT
    "\"compression_method\":\"%M\",\"savings\":%R}", // JSON_FORMAT
    ",%M,,,%R\n", // CSV_FORMAT
    "<compression_method>%M</compression_method><savings>%R</savings></file>"  // XML_FORMAT
};

static const char *report_close_file_item_pak_verbose_mask[] = {
    " (in=%L) (out=%L) (%M %R%%)\n", // STANDARD_FORMAT
    "\"compression_method\":\"%M\",\"uncompressed\":%L,\"compressed\":%L,\"savings\":%R}", // JSON_FORMAT
    ",%M,%L,%L,%R\n", // CSV_FORMAT
    "<compression_method>%M</compression_method><uncompressed>%L</uncompressed>""<compressed>%L</compressed><savings>%R</savings></file>" // XML_FORMAT
};

static const char *report_close_file_item_unpak_mask[] = {
    " %s\n", // STANDARD_FORMAT
    ",\"status\":\"%s\"}", // JSON_FORMAT
    ",,,%s\n", // CSV_FORMAT
    "<status>%s</status></file>" // XML_FORMAT
};

void report_close_file_item(REPORT *report, uint64_t uncompressed_size, uint64_t compressed_size, const char *status, int is_not_last) {
    if (!report) return;

    int idx = _Config.output_format;
    if ( idx > XML_FORMAT || idx < 0 ) idx = 0;
    
    switch ( report->type ) {
        case    REPORT_TYPE_PAK:
            if ( _Config.verbose_flag) {
                if ( idx == STANDARD_FORMAT ) {
                    printc(report_close_file_item_pak_verbose_mask[idx],
                            uncompressed_size,
                            compressed_size,
                            uncompressed_size, compressed_size,
                            (double) compressed_size / uncompressed_size
                        );
                } else {
                    printc(report_close_file_item_pak_verbose_mask[idx],
                            uncompressed_size, compressed_size,
                            uncompressed_size,
                            compressed_size,
                            (double) compressed_size / uncompressed_size
                        );
                }
            } else {
                printc(report_close_file_item_pak_mask[idx],
                        uncompressed_size, compressed_size,
                        (double) compressed_size / uncompressed_size
                    );
            }
            break;

        case    REPORT_TYPE_UNPAK:
            printc(report_close_file_item_unpak_mask[idx],
                    status
                );
            break;

        default:
            break;
    }

    if (idx == JSON_FORMAT && is_not_last) printf(",");

    report->last_operation = REPORT_CLOSE_FILE_ITEM;
}

/**
 * Marks the describing an individual file within the file section of the report.
 *
 * This function describing an individual file within the file section
 * of the report. The behavior depends on the report type, output format, and file information.
 *
 * @param report                The report structure to which the error is associated.
 * @param fi                    Pointer to a FILEINFO structure containing file information.
 * @param uncompressed_size     The uncompressed size of the file.
 * @param compressed_size       The compressed size of the file.
 * @param status                The status of the file operation.
 * @param is_not_last           Flag indicating whether this is the last file in the section.
 */

static const char *report_file_item_pak_mask[] = {
    "adding: %s (%M %R%%)\n", // STANDARD_FORMAT
    "{\"name\":\"%s\",\"compression_method\":\"%M\",\"savings\":%R}", // JSON_FORMAT
    "files,,%s,,%M,,,%R\n", // CSV_FORMAT
    "<file><name>%s</name><compression_method>%M</compression_method><savings>%R</savings></file>"  // XML_FORMAT
};

static const char *report_file_item_pak_verbose_mask[] = {
    "adding: %s (in=%L) (out=%L) (%M %R%%)\n", // STANDARD_FORMAT
    "{\"name\":\"%s\",\"compression_method\":\"%M\",\"uncompressed\":%L,\"compressed\":%L,\"savings\":%R}", // JSON_FORMAT
    "files,,%s,,%M,%L,%L,%R\n", // CSV_FORMAT
    "<file><name>%s</name><compression_method>%M</compression_method><uncompressed>%L</uncompressed>""<compressed>%L</compressed><savings>%R</savings></file>" // XML_FORMAT
};

static const char *report_file_item_unpak_mask[] = {
    "%m: %s... %s\n", // STANDARD_FORMAT
    "{\"name\":\"%s\",\"compression_method\":\"%m\",\"status\":\"%s\"}", // JSON_FORMAT
    "files,,%s,,%m,,,,%s\n", // CSV_FORMAT
    "<file><name>%s</name><compression_method>%m</compression_method><status>%s</status></file>" // XML_FORMAT
};

static const char *report_file_item_unpak_verbose_mask[] = {
    "%m: %s (%L bytes)... %s\n" // STANDARD_FORMAT
    "{\"name\":\"%s\",\"compression_method\":\"%m\",\"uncompressed\":%L,\"status\":\"%s\"}" // JSON_FORMAT
    "files,,%s,,%m,%L,,,%s\n" // CSV_FORMAT
    "<file><name>%s</name><compression_method>%m</compression_method><uncompressed>%L</uncompressed><status>%s</status></file>" // XML_FORMAT
};

void report_file_item(REPORT *report, FILEINFO *fi, uint64_t uncompressed_size, uint64_t compressed_size, const char *status, int is_not_last) {
    if (!report) return;

    int idx = _Config.output_format;
    if ( idx > XML_FORMAT || idx < 0 ) idx = 0;

    switch ( _Config.output_format ) {
        case STANDARD_FORMAT:
            switch ( report->type ) {
                case    REPORT_TYPE_PAK:
                    if ( _Config.verbose_flag) {
                        printc(report_file_item_pak_verbose_mask[idx],
                                fi->filename,
                                uncompressed_size,
                                compressed_size,
                                uncompressed_size, compressed_size,
                                (double) compressed_size / uncompressed_size
                            );
                    } else {
                        printc(report_file_item_pak_mask[idx],
                                fi->filename,
                                uncompressed_size, compressed_size,
                                (double) compressed_size / uncompressed_size
                            );
                    }
                    break;

                case    REPORT_TYPE_UNPAK:
                    if ( _Config.verbose_flag ) {
                        printc(report_file_item_unpak_verbose_mask[idx],
                                fi->uncompressed_size, fi->compressed_size,
                                fi->filename,
                                fi->uncompressed_size,
                                status
                            );
                    } else {
                        printc(report_file_item_unpak_mask[idx],
                                fi->uncompressed_size, fi->compressed_size,
                                fi->filename,
                                status
                            );
                    }
                    break;

                case    REPORT_TYPE_LIST:
                    if ( _Config.verbose_flag ) {
                        printc(report_file_item_list_verbose_mask[idx],
                                fi->compressed_size,
                                fi->uncompressed_size,
                                fi->uncompressed_size, fi->compressed_size,
                                (double) fi->compressed_size / fi->uncompressed_size,
                                fi->name_digest,
                                fi->filename
                            );
                    } else {
                        printc(report_file_item_list_mask[idx],
                                fi->uncompressed_size,
                                fi->filename
                            );
                    }
                    break;

                default:
                    break;
            }
            break;
     
        default: // non STANDARD_FORMAT
          switch ( report->type ) {
                case    REPORT_TYPE_PAK:
                    if ( _Config.verbose_flag ) {
                        printc(report_file_item_pak_mask[idx],
                                fi->filename,
                                uncompressed_size, compressed_size,
                                uncompressed_size,
                                compressed_size,
                                (double) compressed_size / uncompressed_size
                            );
                    } else {
                        printc(report_file_item_pak_mask[idx],
                                fi->filename,
                                uncompressed_size, compressed_size,
                                (double) compressed_size / uncompressed_size
                            );
                    }
                    break;

                case    REPORT_TYPE_UNPAK:
                    if ( _Config.verbose_flag ) {
                        printc(report_file_item_unpak_verbose_mask[idx],
                                fi->filename,
                                fi->uncompressed_size, fi->compressed_size,
                                fi->uncompressed_size,
                                status
                            );
                    } else {
                        printc(report_file_item_unpak_mask[idx],
                                fi->filename,
                                fi->uncompressed_size, fi->compressed_size,
                                status
                            );
                    }
                    break;

                case    REPORT_TYPE_LIST:
                    if ( _Config.verbose_flag ) {
                        printc(report_file_item_list_verbose_mask[idx],
                                fi->filename,
                                fi->name_digest,
                                fi->uncompressed_size, fi->compressed_size,
                                fi->uncompressed_size,
                                fi->compressed_size,
                                (double) fi->compressed_size / fi->uncompressed_size
                            );
                    } else {
                        printc(report_file_item_list_mask[idx],
                                fi->filename,
                                fi->uncompressed_size
                            );
                    }
                    break;

                default:
                    break;
            }
            break;
    }

    if (idx == JSON_FORMAT && is_not_last) printf(",");

    report->last_operation = REPORT_FILE_ITEM;
}

/**
 * Displays information about the PSARC archive.
 *
 * This function displays information about the PSARC archive, including its filename, version,
 * compression type, sizes, and archive flags. The level of detail in the information can be
 * controlled using the 'detailed' parameter.
 *
 * @param input_file            The input PSARC archive file.
 * @param files_info_table      An array of FILEINFO structures.
 * @param blocktable            The table containing block sizes for the PSARC archive.
 */

static const char *info_mask[] = {
                        // STANDARD_FORMAT
                        "archive         : %s\n"
                        "version         : %d.%d\n"
                        "total files     : %d\n"
                        "block size      : %d bytes\n"
                        "archive flags   : %s\n"
                        "manifest        : %L -> %L bytes (%T - %M %R%%)\n"
                        "files           : %L -> %L bytes (%T - %M %R%%)\n"
                        "total           : %L -> %L bytes (%M %R%%)\n"
                        "physical size   : %L bytes\n",

                        // JSON_FORMAT
                        "{"
                          "\"archive\":\"%s\","
                          "\"version\":%d.%d,"
                          "\"total_files\":%d,"
                          "\"block_size\":%d,"
                          "\"archive_flags\":[%s],"
                          "\"manifest\":{"
                            "\"uncompressed\":%L,"
                            "\"compressed\":%L,"
                            "\"compression_type\":\"%T\","
                            "\"compression_method\":\"%M\","
                            "\"savings\":%R"
                          "},"
                          "\"files\":{"
                            "\"uncompressed\":%L,"
                            "\"compressed\":%L,"
                            "\"compression_type\":\"%T\","
                            "\"compression_method\":\"%M\","
                            "\"savings\":%R"
                          "},"
                          "\"totals\":{"
                            "\"uncompressed\":%L,"
                            "\"compressed\":%L,"
                            "\"compression_method\":\"%M\","
                            "\"savings\":%R"
                          "},"
                          "\"physical_size\":%L"
                        "}",

                        // CSV_FORMAT
                        "type,archive,version,total_files,block_size,archive_flags,manifest_uncompressed,manifest_compressed,manifest_compression_type,manifest_compression_method,manifest_savings,files_uncompressed,files_compressed,files_compression_type,files_compression_method,files_savings,totals_uncompressed,totals_compressed,totals_compression_method,totals_savings,physical_size\n"
                        "totals,%s,%d.%d,%d,%d,\"%s\",%L,%L,\"%T\",\"%M\",%R,%L,%L,\"%T\",\"%M\",%R,%L,%L,\"%M\",%R,%L\n",

                        // XML_FORMAT
                        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                          "<archive>"
                          "<archive>%s</archive>"
                          "<version>%d.%d</version>"
                          "<total_files>%d</total_files>"
                          "<block_size>%d</block_size>"
                          "<archive_flags>"
                            "%s"
                          "</archive_flags>"
                          "<manifest>"
                            "<uncompressed>%L</uncompressed>"
                            "<compressed>%L</compressed>"
                            "<compression_type>%T</compression_type>"
                            "<compression_method>%M</compression_method>"
                            "<savings>%R</savings>"
                          "</manifest>"
                          "<files>"
                            "<uncompressed>%L</uncompressed>"
                            "<compressed>%L</compressed>"
                            "<compression_type>%T</compression_type>"
                            "<compression_method>%M</compression_method>"
                            "<savings>%R</savings>"
                          "</files>"
                          "<totals>"
                            "<uncompressed>%L</uncompressed>"
                            "<compressed>%L</compressed>"
                            "<compression_method>%M</compression_method>"
                            "<savings>%R</savings>"
                          "</totals>"
                          "<physical_size>%L</physical_size>"
                        "</archive>"
                    };

void show_info(char *input_file, FILEINFO *files_info_table, uint32_t *blocktable) {
    int compression_type = PSARC_STORE;
    int manifest_compression_type = PSARC_STORE;

    uint64_t total_compressed = 0LL;
    uint64_t total_uncompressed = 0LL;
    uint64_t manifest_compressed = 0LL;
    uint64_t manifest_uncompressed = files_info_table[0].uncompressed_size;

    uint64_t csz;
    if ((csz = get_compressed_size(&files_info_table[0], blocktable)) != manifest_uncompressed) {
        manifest_compression_type = _ArchiveInfo.compression_type;
    }
    manifest_compressed = csz;
    total_compressed += csz;
    total_uncompressed += manifest_uncompressed;

    for (uint32_t i = 1; i < _ArchiveInfo.toc_entries; i++) {
        if ((csz = get_compressed_size(&files_info_table[i], blocktable)) != files_info_table[i].uncompressed_size) {
            compression_type = _ArchiveInfo.compression_type;
        }
        total_compressed += csz;
        total_uncompressed += files_info_table[i].uncompressed_size;
    }

#if 0
#ifdef _WIN32
    setlocale(LC_NUMERIC, "en_US");
#else
    setlocale(LC_NUMERIC, "en_US.UTF-8");
#endif
#endif

    int idx = _Config.output_format;
    if ( idx > XML_FORMAT || idx < 0 ) idx = 0;

    char archive_flags_str[128];

    switch ( _Config.output_format ) {
        default:
        case CSV_FORMAT:
            sprintf(archive_flags_str, "%s%s", (_ArchiveInfo.archive_flags & AF_ABSPATH) ? "Absolute Paths" : "Relative Paths",
                                               (_ArchiveInfo.archive_flags & AF_ICASE) ? " | Case-Insensitive Path" : "" );
            break;

        case JSON_FORMAT:
            sprintf(archive_flags_str, "%s%s", (_ArchiveInfo.archive_flags & AF_ABSPATH) ? "\"Absolute Paths\"" : "\"Relative Paths\"",
                                               (_ArchiveInfo.archive_flags & AF_ICASE) ? ",\"Case-Insensitive Path\"" : "" );
            break;

        case XML_FORMAT:
            sprintf(archive_flags_str, "<flag>%s</flag>%s", (_ArchiveInfo.archive_flags & AF_ABSPATH) ? "Absolute Paths" : "Relative Paths",
                                                            (_ArchiveInfo.archive_flags & AF_ICASE) ? "<flag>Case-Insensitive Path</flag>" : "" );
            break;

        }

  /*
                        "archive         : %s\n"
                        "version         : %d.%d\n"
                        "total files     : %d\n"
                        "block size      : %d bytes\n"
                        "archive flags   : %s\n"
                        "manifest        : %L -> %L bytes (%T - %M %R%%)\n"
                        "files           : %L -> %L bytes (%T - %M %R%%)\n"
                        "total           : %L -> %L bytes (%M %R%%)\n"
                        "physical size   : %L bytes\n",
*/
    printc(info_mask[idx],
            input_file,
            _ArchiveInfo.version.high, _ArchiveInfo.version.low,
            _ArchiveInfo.toc_entries - 1,
            _ArchiveInfo.block_size,
            archive_flags_str,
            (uint64_t) manifest_uncompressed,
            (uint64_t) manifest_compressed,
            manifest_compression_type,
            (uint64_t) manifest_compressed, (uint64_t) manifest_uncompressed,
            (double) manifest_compressed / manifest_uncompressed,
            (uint64_t) total_uncompressed - manifest_uncompressed,
            (uint64_t) total_compressed - manifest_compressed,
            compression_type,
            (uint64_t) total_compressed - manifest_compressed, (uint64_t) total_uncompressed - manifest_uncompressed,
            (double) (total_compressed - manifest_compressed) / (total_uncompressed - manifest_uncompressed),
            (uint64_t) total_uncompressed,
            (uint64_t) total_compressed,
            total_compressed, total_uncompressed,
            (double) total_compressed / total_uncompressed,
            total_compressed + _ArchiveInfo.toc_length
        );
}

/**
 * Report an error of the specified type for the provided archive.
 *
 * This function reports an error of the specified type for the provided archive.
 *
 * @param report  The report structure to which the error is associated.
 * @param message The error message format (can include format specifiers).
 * @param ...     Additional arguments to be formatted according to the message.
 */

void report_error(REPORT *report, const char *message, ...) {
    va_list args;
    va_start(args, message);

    if ( report ) {
        switch (report->last_operation) {
            case    REPORT_OPEN:
                break;

            case    REPORT_OPEN_FILE_SECTION:
                report_close_file_section(report);
                break;

            case    REPORT_OPEN_FILE_ITEM:
                report_close_file_item(report, 0, 0, "fail", 0);
                report_close_file_section(report);
                break;

            case    REPORT_FILE_ITEM:
            case    REPORT_CLOSE_FILE_ITEM:
                report_close_file_section(report);
                break;

            case    REPORT_CLOSE_FILE_SECTION:
                break;
        }
    }

    switch (_Config.output_format) {
        case STANDARD_FORMAT:
            printf(APPNAME ": ");
            vprintf(message, args);
            printf("\n");
            break;

        case JSON_FORMAT:
            if ( !report ) printf("{");
            else           printf(",");
            printf("\"error\":\"");
            vprintf(message, args);
            printf("\"");
            if ( !report ) printf("}");
            break;

        case CSV_FORMAT:
            if ( !report ) printf("type_record,archive_name,files_name,files_name_digest,files_compression_method,files_uncompressed,files_compressed,files_savings,files_status,total_files,total_uncompressed,total_compressed,total_savings,total_errors,error_message\n");
            printf("error,,,,,,,,,,,,,,\"");
            vprintf(message, args);
            printf("\"\n");
            break;

        case XML_FORMAT:
            if ( !report ) printf("<archive>");
            printf("<error>");
            vprintf(message, args);
            printf("</error>");
            if ( !report ) printf("</archive>");
            break;

        default:
            break;
    }

    va_end(args);

    if ( report ) {
        switch (report->last_operation) {
            case    REPORT_OPEN:
                report_close(report, 0, 0, 0, 0, 0, 0, 0);
                break;

            case    REPORT_OPEN_FILE_SECTION:
                break;

            case    REPORT_OPEN_FILE_ITEM:
                break;

            case    REPORT_FILE_ITEM:
            case    REPORT_CLOSE_FILE_ITEM:
                break;

            case    REPORT_CLOSE_FILE_SECTION:
                break;

        }
    }
}
