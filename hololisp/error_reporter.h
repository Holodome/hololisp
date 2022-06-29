#ifndef __HLL_ERROR_REPORTER_H__
#define __HLL_ERROR_REPORTER_H__

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include "ext.h"

typedef struct hll_source_location {
    /// If 0, consider this be from stdin
    char const *filename;
    /// Starts from 0
    uint32_t line;
    /// Starts from 0
    uint32_t column;
} hll_source_location;

typedef struct hll_error_reporter {
    int is_initialized;
    int has_error;
    void *out;
} hll_error_reporter;

hll_error_reporter *hll_get_empty_reporter(void);

HLL_DECL
void hll_init_error_reporter(hll_error_reporter *reporter);

HLL_DECL
void hll_report_errorv(hll_error_reporter *reporter, char const *format,
                       va_list args);

HLL_DECL
void hll_report_error_verbosev(hll_error_reporter *reporter,
                               hll_source_location *loc, char const *format,
                               va_list args);

HLL_DECL
void hll_report_notev(hll_error_reporter *reporter, char const *format,
                      va_list args);

HLL_DECL
void hll_report_note_verbosev(hll_error_reporter *reporter,
                              hll_source_location *loc, char const *format,
                              va_list args);

HLL_DECL
HLL_ATTR(format(printf, 2, 3))
void hll_report_error(hll_error_reporter *reporter, char const *format, ...);

HLL_DECL
HLL_ATTR(format(printf, 3, 4))
void hll_report_error_verbose(hll_error_reporter *reporter,
                              hll_source_location *loc, char const *format,
                              ...);

HLL_DECL
HLL_ATTR(format(printf, 2, 3))
void hll_report_note(hll_error_reporter *reporter, char const *format, ...);

HLL_DECL
HLL_ATTR(format(printf, 3, 4))
void hll_report_note_verbose(hll_error_reporter *reporter,
                             hll_source_location *loc, char const *format, ...);

#endif
