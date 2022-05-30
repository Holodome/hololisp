#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "prog.h"

static void
help(void) {
    printf(
        "OVERVIEW: toy lisp interprenter\n"
        "\n"
        "USAGE: hololisp [opts] file...\n");
}

static void
version(void) {
    printf("hololisp version 0.0.1\n");
}

static bool
cli_params(uint32_t argc, char **argv, hll_options *opts) {
    bool result = true;

    uint32_t cursor = 1;
    while (cursor < argc) {
        char const *option = argv[cursor++];
        if (*option != '-') {
            opts->input_file = option;
            continue;
        }

        if (!strcmp(option, "--version") || !strcmp(option, "-v")) {
            version();
            break;
        } else if (!strcmp(option, "--help") || !strcmp(option, "-h")) {
            help();
            break;
        } else if (!strcmp(option, "--lex")) {
            opts->mode = HLL_PROGM_LEX;
        } else if (!strcmp(option, "--pp")) {
            opts->mode = HLL_PROGM_PP;
        } else if (!strcmp(option, "--interp")) {
            opts->mode = HLL_PROGM_INTERP;
        } else {
            fprintf(stderr, "Unknown option '%s'\n", option);
            result = false;
            break;
        }
    }

    return result;
}

int
main(int argc, char **argv) {
    hll_options opts = {0};
    if (!cli_params(argc, argv, &opts)) {
        return EXIT_FAILURE;
    }

    hll_execute(&opts);

    return EXIT_SUCCESS;
}
