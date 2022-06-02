#include "logger.h"

static FILE *output_file = NULL;
static int has_console_output = 0;

void
logger_set_file(FILE *file) {
    output_file = file;
}

void
logger_set_write_to_console(int is_writing_) {
    has_console_output = is_writing_;
}

void
log_errorv(char const *fmt, va_list args) {
    if (has_console_output) {
        vfprintf(stderr, fmt, args);
    }

    if (output_file != NULL) {
        vfprintf(output_file, fmt, args);
    }
}

void
log_error(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_errorv(fmt, args);
    va_end(args);
}

void
log_infov(char const *fmt, va_list args) {
    if (has_console_output) {
        vfprintf(stdout, fmt, args);
    }

    if (output_file != NULL) {
        vfprintf(output_file, fmt, args);
    }
}

void
log_info(char const *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_infov(fmt, args);
    va_end(args);
}
