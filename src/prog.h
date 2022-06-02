#ifndef __HLL_PROG_H__
#define __HLL_PROG_H__

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HLL_PROGM_INTERP = 0x0, /* Typical interpretation, default mode */
    HLL_PROGM_LEX = 0x1,    /* Only lexer, used for testing */
    HLL_PROGM_PP = 0x2      /* Run preprocessor for macro unwrapping */
} hll_program_mode;

typedef struct {
    char const *input_file;
    hll_program_mode mode;
} hll_options;

void hll_execute(hll_options const *opts);

#ifdef __cplusplus
}
#endif

#endif
