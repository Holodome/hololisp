#include "prog.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* static FILE * */
/* open_file_read(char const *filename) { */
/*     return strcmp(filename, "--") ? fopen(filename, "r") : stdin; */
/* } */

static void
pp(hll_options const *opts) {
    (void)opts;
}

static void
lex(hll_options const *opts) {
    (void)opts;
}

static void
interp(hll_options const *opts) {
    (void)opts;
}

void
hll_execute(hll_options const *opts) {
    switch (opts->mode) {
    default:
        assert(0);
        break;
    case HLL_PROGM_INTERP:
        interp(opts);
        break;
    case HLL_PROGM_LEX:
        lex(opts);
        break;
    case HLL_PROGM_PP:
        pp(opts);
        break;
    }
}
