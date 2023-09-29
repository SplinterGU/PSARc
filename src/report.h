/**
 * Copyright (c) 2023 Juan José Ponteprino
 *
 * @file report.h
 * @brief Header for the unpak utility in the PSARc project.
 *
 * This header file contains declarations and constants for the unpak utility used in
 * the PSARc project. It defines interfaces for extracting files from PSARC archives.
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

#ifndef __REPORT_H
#define __REPORT_H

#include <stdint.h>
#include "common.h"
#include "psarc.h"

typedef enum {
    REPORT_TYPE_PAK,
    REPORT_TYPE_UNPAK,
    REPORT_TYPE_LIST, 
    REPORT_TYPE_INFO
} REPORT_TYPE;

typedef enum {
    REPORT_OPEN,
    REPORT_OPEN_FILE_SECTION,
    REPORT_OPEN_FILE_ITEM,
    REPORT_FILE_ITEM,
    REPORT_CLOSE_FILE_ITEM,
    REPORT_CLOSE_FILE_SECTION
} REPORT_OPERATION;

typedef struct {
    REPORT_TYPE type;
    REPORT_OPERATION last_operation;
} REPORT;

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

REPORT *report_open(REPORT_TYPE type, const char *archive_name);

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

void report_close(REPORT *report, int show_resume, uint64_t total_compressed, uint64_t total_uncompressed, uint64_t manifest_compressed, uint64_t manifest_uncompressed, uint32_t successful, uint32_t errors);

/**
 * Marks the open of a section in the report for listing files.
 *
 * This function marks the open of a section in the report for listing files. The behavior
 * depends on the report type and output format.
 *
 * @param report  The report structure to which the error is associated.
 */

void report_open_file_section(REPORT *report);

/**
 * Marks the end of the file section in the report.
 *
 * This function marks the end of the file section in the report. The behavior depends
 * on the report type and output format.
 *
 * @param report  The report structure to which the error is associated.
 */

void report_close_file_section(REPORT *report);

/**
 * Marks the open of describing an individual file within the file section of the report.
 *
 * This function marks the open of describing an individual file within the file section
 * of the report. The behavior depends on the report type, output format, and file information.
 *
 * @param report    The report structure to which the error is associated.
 * @param fi        Pointer to a FILEINFO structure containing file information.
 */

void report_open_file_item(REPORT *report, FILEINFO *fi);

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

void report_close_file_item(REPORT *report, uint64_t uncompressed_size, uint64_t compressed_size, const char *status, int is_not_last);

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

void report_file_item(REPORT *report, FILEINFO *fi, uint64_t uncompressed_size, uint64_t compressed_size, const char *status, int is_not_last);

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

void show_info(char *input_file, FILEINFO *files_info_table, uint32_t *blocktable);

/**
 * Report an error of the specified type for the provided archive.
 *
 * This function reports an error of the specified type for the provided archive.
 *
 * @param report  The report structure to which the error is associated.
 * @param message The error message format (can include format specifiers).
 * @param ...     Additional arguments to be formatted according to the message.
 */

void report_error(REPORT *report, const char *message, ...);

#endif
