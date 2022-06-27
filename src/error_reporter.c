#include "error_reporter.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define ERROR_STRING "\033[31;1merror\033[0m"
#define NOTE_STRING "\033[90;1mnote\033[1m"

#define CHECK_INITIALIZED assert(reporter->is_initialized)

typedef struct file_info {
    struct file_info *next;

    char const *name;
    char const *full_name;

    char const *data;
    size_t data_size;
} file_info;

hll_error_reporter *
hll_get_empty_reporter(void) {
    static hll_error_reporter rep = { 0 };
    if (!rep.is_initialized) {
        rep.out = tmpfile();
        rep.is_initialized = 1;
    }
    return &rep;
}

void
hll_init_error_reporter(hll_error_reporter *reporter) {
    reporter->out = stderr;
    reporter->is_initialized = 1;
}

static file_info *
get_file_info(char const *filename) {
    char full_path_buffer[4096];
    size_t full_path_length;
    char *file_data;
    size_t file_data_size;
    if (hll_get_full_file_path(filename, full_path_buffer,
                               sizeof(full_path_buffer),
                               &full_path_length) == HLL_FS_IO_OK &&
        hll_read_entire_file(filename, &file_data, &file_data_size) ==
            HLL_FS_IO_OK) {
        size_t filename_length = strlen(filename);
        void *data = calloc(
            1, sizeof(file_info) + full_path_length + 1 + filename_length + 1);
        file_info *info = data;
        info->name = (char *)data + sizeof(file_info);
        info->full_name =
            (char *)data + sizeof(file_info) + 1 + filename_length;

        strcpy((char *)info->name, filename);
        strcpy((char *)info->full_name, full_path_buffer);
        info->data = file_data;
        info->data_size = file_data_size;

        return info;
    }

    return NULL;
}

static void
report_message(FILE *out, char const *message_kind, char const *msg,
               va_list args) {
    fprintf(out, "\033[1meval: %s: \033[1m", message_kind);
    vfprintf(out, msg, args);
    fprintf(out, "\033[0m\n");
}

static void
report_message_for_stdin(FILE *out, hll_source_location *loc,
                         char const *message_kind, char const *msg,
                         va_list args) {
    fprintf(out, "\033[1mstdin:%u: %s: \033[1m", loc->column, message_kind);
    vfprintf(out, msg, args);
    fprintf(out, "\033[0m\n");
}

typedef struct {
    char const *start;
    char const *end;
} find_line_result;

static find_line_result
find_line_in_file(char const *file_contents, char const *file_eof,
                  hll_source_location *loc) {
    char const *line_start = file_contents;
    uint32_t line_counter = 0;
    while (line_counter < loc->line && line_start < file_eof) {
        if (*line_start == '\n') {
            ++line_counter;
        }
        ++line_start;
    }

    char const *line_end = line_start + 1;
    while (*line_end != '\n' && line_end < file_eof) {
        ++line_end;
    }

    find_line_result result;
    result.start = line_start;
    result.end = line_end;

    return result;
}

static void
report_message_with_source(FILE *out, char const *file_contents,
                           char const *file_eof, hll_source_location *loc,
                           char const *message_kind, char const *msg,
                           va_list args) {
    find_line_result line = find_line_in_file(file_contents, file_eof, loc);

    fprintf(out, "\033[1m%s:%u:%u: %s: \033[1m", loc->filename, loc->line + 1,
            loc->column + 1, message_kind);
    vfprintf(out, msg, args);
    fprintf(out, "\033[0m\n%.*s\n", (int)(line.end - line.start), line.start);
    if (loc->column != 0) {
        fprintf(out, "%*c", loc->column, ' ');
    }
    fprintf(out, "\033[32;1m^\033[0m\n");
}

void
hll_report_errorv(hll_error_reporter *reporter, char const *format,
                  va_list args) {
    CHECK_INITIALIZED;
    report_message(reporter->out, ERROR_STRING, format, args);
}

void
hll_report_error(hll_error_reporter *reporter, char const *format, ...) {
    va_list args;
    va_start(args, format);
    hll_report_errorv(reporter, format, args);
}

void
hll_report_error_verbosev(hll_error_reporter *reporter,
                          hll_source_location *loc, char const *format,
                          va_list args) {
    CHECK_INITIALIZED;
    if (!loc->filename) {
        report_message_for_stdin(reporter->out, loc, ERROR_STRING, format,
                                 args);
    } else {
        file_info *file = get_file_info(loc->filename);
        if (file != NULL) {
            loc->filename = file->full_name;
            report_message_with_source(reporter->out, file->data,
                                       file->data + file->data_size, loc,
                                       ERROR_STRING, format, args);
        }
    }
}

void
hll_report_error_verbose(hll_error_reporter *reporter, hll_source_location *loc,
                         char const *format, ...) {
    va_list args;
    va_start(args, format);
    hll_report_error_verbosev(reporter, loc, format, args);
}

void
hll_report_notev(hll_error_reporter *reporter, char const *format,
                 va_list args) {
    CHECK_INITIALIZED;
    report_message(reporter->out, NOTE_STRING, format, args);
}

void
hll_report_note(hll_error_reporter *reporter, char const *format, ...) {
    va_list args;
    va_start(args, format);
    hll_report_notev(reporter, format, args);
}

void
hll_report_note_verbosev(hll_error_reporter *reporter, hll_source_location *loc,
                         char const *format, va_list args) {
    CHECK_INITIALIZED;
    if (!loc->filename) {
        report_message_for_stdin(reporter->out, loc, NOTE_STRING, format, args);
    } else {
        file_info *file = get_file_info(loc->filename);
        if (file != NULL) {
            loc->filename = file->full_name;
            report_message_with_source(reporter->out, file->data,
                                       file->data + file->data_size, loc,
                                       NOTE_STRING, format, args);
        }
    }
}

void
hll_report_note_verbose(hll_error_reporter *reporter, hll_source_location *loc,
                        char const *format, ...) {
    va_list args;
    va_start(args, format);
    hll_report_note_verbosev(reporter, loc, format, args);
}
