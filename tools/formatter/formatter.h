#ifndef __HLLF_FORMATTER_H__
#define __HLLF_FORMATTER_H__

#include <stddef.h>
#include <stdint.h>

#include "hololisp/ext.h"

typedef struct {
    uint32_t tab_size;
} hllf_settings;

hllf_settings hllf_default_settings(void);

typedef struct {
    char *data;
    size_t data_size;
} hllf_format_result;

HLL_DECL hllf_format_result hllf_format(char const *source,
                                        size_t source_length,
                                        hllf_settings *settings);
HLL_DECL void hllf_format_free(char const *text);

#endif
