#ifndef __HLL_ERROR_REPORTER_H__
#define __HLL_ERROR_REPORTER_H__

#include <stdarg.h>
#include <stdint.h>

typedef struct hll_source_location {
    char const *filename;
    uint32_t line;
    uint32_t column;
} hll_source_location;

typedef struct hll_error_reporter {
    int has_error;
} hll_error_reporter;

int hll_is_error_reported(hll_error_reporter *reporter);

void hll_report_errorv(hll_error_reporter *reporter, char const *format,
                       va_list args);

void hll_report_error(hll_error_reporter *reporter, char const *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

void hll_report_error_verbosev(hll_error_reporter *reporter,
                               hll_source_location loc, char const *format,
                               va_list args);

void hll_report_error_verbose(hll_error_reporter *reporter,
                              hll_source_location loc, char const *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 3, 4)))
#endif
    ;

void hll_report_notev(hll_error_reporter *reporter, char const *format,
                      va_list args);

void hll_report_note(hll_error_reporter *reporter, char const *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 1, 2)))
#endif
    ;

void hll_report_note_verbosev(hll_error_reporter *reporter,
                              hll_source_location loc, char const *format,
                              va_list args);

void hll_report_note_verbose(hll_error_reporter *reporter,
                             hll_source_location loc, char const *format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

#endif
