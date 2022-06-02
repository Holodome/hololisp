#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>
#include <stdio.h>

/**
 * Sets file for logger to write to. Logger does not close file.
 * @param file File to write to.
 */
void logger_set_file(FILE *file);

/**
 * Sets bool value that indicates whether should logs be written to
 * stdout/stderr.
 * @param is_writing non-zero value if needs to write, 0 otherwise. Default is
 * 0.
 */
void logger_set_write_to_console(int is_writing);

/**
 * Outputs error message using printf-format string and va_list.
 */
void log_errorv(char const *fmt, va_list args);

/**
 * Outputs error message using printf-format string and variadic arguments.
 */
void log_error(char const *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

/**
 * Outputs info message using printf-format string and va_list.
 */
void log_infov(char const *fmt, va_list args);

/**
 * Outputs info message using printf-format string and variadic arguments.
 */
void log_info(char const *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

#endif
