#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "prog.h"

static void
help(uint32_t rc) {
    printf(
        "OVERVIEW: toy lisp interprenter\n"
        "\n"
        "USAGE: hololisp [opts] file...\n");
    exit(rc);
}

static void
version(void) {
    printf("hololisp version 0.0.1\n");
    exit(EXIT_SUCCESS);
}

static bool
cli_params(uint32_t argc, char **argv, options *opts) {
    uint32_t cursor = 1;
    while (cursor < argc) {
        char const *option = argv[cursor++];
        if (*option != '-') {
            opts->input_file = option;
            continue;
        }

        if (!strcmp(option, "--version") || !strcmp(option, "-v")) {
            version();
        } else if (!strcmp(option, "--help") || !strcmp(option, "-h")) {
            help(EXIT_SUCCESS);
        } else if (!strcmp(option, "--lex")) {
            opts->mode = PROGM_LEX;
        } else if (!strcmp(option, "--pp")) {
            opts->mode = PROGM_PP;
        } else if (!strcmp(option, "--interp")) {
            opts->mode = PROGM_INTERP;
        } else {
            fprintf(stderr, "Unknown option '%s'\n", option);
            help(EXIT_FAILURE);
        }
    }

    return true;
}


int
main(int argc, char **argv) {
    options opts = {0};
    if (!cli_params(argc, argv, &opts)) {
        return EXIT_FAILURE;
    }

    execute(&opts);

    return EXIT_SUCCESS;
}
