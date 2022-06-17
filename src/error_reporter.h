#ifndef __HLL_ERROR_REPORTER_H__
#define __HLL_ERROR_REPORTER_H__

#include <stdarg.h>
#include <stdint.h>

typedef struct {
    char const *filename;
    uint32_t line;
    uint32_t column;
} source_location;

void hll_init_error_reporter();
void hll_free_error_reporter();

void hll_report_errorv(char const *format, va_list args);

void hll_report_error(char const *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

void hll_report_error_verbosev(source_location loc, char const *format,
                               va_list args);

void hll_report_error_verbose(source_location loc, char const *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

#endif
