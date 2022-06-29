#ifndef __HLLF_FORMATTER_H__
#define __HLLF_FORMATTER_H__

#include <stdint.h>

#include "hololisp/ext.h"

typedef struct {
    uint32_t tab_size;
} hllf_settings;

hllf_settings hllf_default_settings(void);

HLL_DECL char const *hllf_format(char const *source, size_t source_length,
                                 hllf_settings *settings);
HLL_DECL void hllf_format_free(char const *text);

#endif
