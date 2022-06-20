#include "error_reporter.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define ERROR_STRING "\033[31;1merror\033[0m"
#define NOTE_STRING "\033[90;1mnote\033[1m"
#define CHECK_INITIALIZED     \
    if (!rep->is_initialized) \
        return;

typedef struct file_info {
    struct file_info *next;

    char const *name;
    char const *full_name;

    char const *data;
    size_t data_size;
} file_info;

typedef struct {
    FILE *out;

    char const *current_filename;
    size_t error_count;
    int is_initialized;

    file_info *files;
} error_reporter;

static error_reporter rep_;
static error_reporter *rep = &rep_;

void
hll_init_error_reporter() {
    rep->out = stderr;
    rep->is_initialized = 1;
}

void
hll_free_error_reporter() {
    assert(!"TODO");
}

static file_info *
get_file_info(char const *filename) {
    file_info **info_ptr = &rep->files;
    while (*info_ptr && strcmp((*info_ptr)->name, filename) != 0) {
        info_ptr = &(*info_ptr)->next;
    }

    if (!*info_ptr) {
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
            void *data = calloc(1, sizeof(file_info) + full_path_length + 1 +
                                       filename_length + 1);
            file_info *info = data;
            info->name = (char *)data + sizeof(file_info);
            info->full_name =
                (char *)data + sizeof(file_info) + 1 + filename_length;

            strcpy((char *)info->name, filename);
            strcpy((char *)info->full_name, full_path_buffer);
            info->data = file_data;
            info->data_size = file_data_size;

            *info_ptr = info;
        } else {
            hll_report_error("INTERNAL: Failed to read file '%s'", filename);
        }
    }

    return *info_ptr;
}

static void
report_message(FILE *out, char const *message_kind, char const *msg,
               va_list args) {
    fprintf(out, "\033[1meval: %s: \033[1m", message_kind);
    vfprintf(out, msg, args);
    fprintf(out, "\033[0m\n");
}

static void
report_message_for_stdin(FILE *out, source_location loc,
                         char const *message_kind, char const *msg,
                         va_list args) {
    fprintf(out, "\033[1mstdin:%u: %s: \033[1m", loc.column, message_kind);
    vfprintf(out, msg, args);
    fprintf(out, "\033[0m\n");
}

static void
report_message_with_source(FILE *out, char const *file_contents,
                           char const *file_eof, source_location loc,
                           char const *message_kind, char const *msg,
                           va_list args) {
    char const *line_start = file_contents;
    uint32_t line_counter = 0;
    while (line_counter < loc.line && line_start < file_eof) {
        if (*line_start == '\n') {
            ++line_counter;
        }
        ++line_start;
    }

    char const *line_end = line_start + 1;
    while (*line_end != '\n' && line_end != file_eof) {
        ++line_end;
    }

    // The reason this exists is because originally we were supposed to decode
    // UTF8 here.
    uint32_t col_counter = 0;
    char const *col_cursor = line_start;
    while (col_cursor < line_start + loc.column && col_cursor < line_end) {
        ++col_cursor;
        ++col_counter;
    }

    fprintf(out, "\033[1m%s:%u:%u: %s: \033[1m", loc.filename, loc.line + 1,
            col_counter + 1, message_kind);
    vfprintf(out, msg, args);
    fprintf(out, "\033[0m\n%.*s\n", (int)(line_end - line_start), line_start);
    if (col_counter != 0) {
        fprintf(out, "%*c", col_counter, ' ');
    }
    fprintf(out, "\033[32;1m^\033[0m\n");
}

void
hll_report_errorv(char const *format, va_list args) {
    CHECK_INITIALIZED;
    report_message(rep->out, ERROR_STRING, format, args);
}

void
hll_report_error(char const *format, ...) {
    va_list args;
    va_start(args, format);
    hll_report_errorv(format, args);
}

void
hll_report_error_verbosev(source_location loc, char const *format,
                          va_list args) {
    CHECK_INITIALIZED;
    if (!loc.filename) {
        report_message_for_stdin(rep->out, loc, ERROR_STRING, format, args);
    } else {
        file_info *file = get_file_info(loc.filename);
        if (file != NULL) {
            loc.filename = file->full_name;
            report_message_with_source(rep->out, file->data,
                                       file->data + file->data_size, loc,
                                       ERROR_STRING, format, args);
        }
    }
}

void
hll_report_error_verbose(source_location loc, char const *format, ...) {
    va_list args;
    va_start(args, format);
    hll_report_error_verbosev(loc, format, args);
}

void
hll_report_notev(char const *format, va_list args) {
    CHECK_INITIALIZED;
    report_message(rep->out, NOTE_STRING, format, args);
}

void
hll_report_note(char const *format, ...) {
    va_list args;
    va_start(args, format);
    hll_report_notev(format, args);
}

void
hll_report_note_verbosev(source_location loc, char const *format,
                         va_list args) {
    CHECK_INITIALIZED;
    if (!loc.filename) {
        report_message_for_stdin(rep->out, loc, NOTE_STRING, format, args);
    } else {
        file_info *file = get_file_info(loc.filename);
        if (file != NULL) {
            loc.filename = file->full_name;
            report_message_with_source(rep->out, file->data,
                                       file->data + file->data_size, loc,
                                       NOTE_STRING, format, args);
        }
    }
}

void
hll_report_note_verbose(source_location loc, char const *format, ...) {
    va_list args;
    va_start(args, format);
    hll_report_note_verbosev(loc, format, args);
}
