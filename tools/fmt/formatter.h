#ifndef __HLLF_FORMATTER_H__
#define __HLLF_FORMATTER_H__

#include <stddef.h>
#include <stdint.h>

#include "hll_ext.h"

typedef struct {
    char *data;
    size_t data_size;
} hllf_format_result;

HLL_DECL hllf_format_result hllf_format(char const *source,
                                        size_t source_length);

HLL_DECL void hllf_format_free(char const *text);

#endif
