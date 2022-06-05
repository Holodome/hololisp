#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "lisp.h"
#include "lisp_gcs.h"
#include "parser.h"
#include "utils.h"

typedef struct {
    char const *filename;
} run_opts;

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
cli_params(uint32_t argc, char const **argv, run_opts *opts) {
    bool result = true;

    uint32_t cursor = 1;
    while (cursor < argc) {
        char const *option = argv[cursor++];
        if (*option != '-') {
            opts->filename = option;
            continue;
        }

        if (!strcmp(option, "--version") || !strcmp(option, "-v")) {
            version();
            break;
        } else if (!strcmp(option, "--help") || !strcmp(option, "-h")) {
            help();
            break;
        } else {
            fprintf(stderr, "Unknown option '%s'\n", option);
            result = false;
            break;
        }
    }

    return result;
}

int
main(int argc, char const **argv) {
    run_opts opts = { 0 };
    if (!cli_params(argc, argv, &opts)) {
        goto error;
    }

    if (!opts.filename) {
        fprintf(stderr, "No input files provided\n");
        goto error;
    }

    FILE *file;
    if (hll_open_file(&file, opts.filename, "r") != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to open file '%s'\n", opts.filename);
        goto error;
    }

    size_t fsize;
    if (hll_get_file_size(file, &fsize) != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to open file '%s'\n", opts.filename);
        goto close_file_error;
    }

    char *file_contents = calloc(fsize, 1);
    if (fread(file_contents, fsize, 1, file) != 1) {
        fprintf(stderr, "Failed to read file '%s'\n", opts.filename);
        goto close_file_error;
    }

    if (hll_close_file(file) != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to close file '%s'\n", opts.filename);
        goto error;
    }

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);
    hll_add_builtins(&ctx);

    enum { BUFFER_SIZE = 4096 };
    char buffer[BUFFER_SIZE];
    hll_lexer lexer = hll_lexer_create(file_contents, buffer, BUFFER_SIZE);
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    for (;;) {
        hll_lisp_obj_head *obj;
        hll_parse_result parse_result = hll_parse(&parser, &obj);

        if (parse_result == HLL_PARSE_EOF) {
            break;
        } else if (parse_result != HLL_PARSE_OK) {
            fprintf(stderr, "Invalid syntax\n");
            break;
        } else {
            hll_lisp_print(stdout, hll_eval(&ctx, obj));
            printf("\n");
        }
    }

    return EXIT_SUCCESS;
close_file_error:
    if (hll_close_file(file) != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to close file '%s'\n", opts.filename);
    }
error:
    return EXIT_SUCCESS;
}
