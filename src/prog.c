#include "prog.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static FILE *
open_file_read(char const *filename) {
    return strcmp(filename, "--") ? fopen(filename, "r") : stdin;
}

static void
pp(options const *opts) {
    (void)opts;
}

static void
lex(options const *opts) {
}

static void
interp(options const *opts) {
    (void)opts;
}

void
execute(options const *opts) {
    switch (opts->mode) {
    default:
        assert(0);
        break;
    case PROGM_INTERP:
        interp(opts);
        break;
    case PROGM_LEX:
        lex(opts);
        break;
    case PROGM_PP:
        pp(opts);
        break;
    }
}
