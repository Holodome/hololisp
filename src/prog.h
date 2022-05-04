#ifndef __HLL_PROG_H__
#define __HLL_PROG_H__

#include <stdint.h>
#include <stdio.h>

typedef enum {
    PROGM_INTERP = 0x0,  // Typical interpretation, default mode
    PROGM_LEX    = 0x1,  // Only lexer, used for testing
    PROGM_PP     = 0x2,  // Run preprocessor for macro unwrapping
} program_mode;

typedef struct {
    char const *input_file;
    program_mode mode;
} options;

void execute(options const *opts);

#endif
