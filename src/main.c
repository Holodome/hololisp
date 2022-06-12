#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _POSIX_SOURCE
#include <unistd.h>

#include "lexer.h"
#include "lisp.h"
#include "lisp_std.h"
#include "reader.h"
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

static int
is_stdin_interactive(void) {
    return isatty(fileno(stdin));
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

static int
execute_file(char const *filename) {
    FILE *file;
    if (hll_open_file(&file, filename, "r") != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to open file '%s'\n", filename);
        goto error;
    }

    size_t fsize;
    if (hll_get_file_size(file, &fsize) != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to open file '%s'\n", filename);
        goto close_file_error;
    }

    char *file_contents = calloc(fsize, 1);
    if (fread(file_contents, fsize, 1, file) != 1) {
        fprintf(stderr, "Failed to read file '%s'\n", filename);
        goto close_file_error;
    }

    if (hll_close_file(file) != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to close file '%s'\n", filename);
        goto error;
    }

    hll_ctx ctx = hll_create_ctx();

    enum { BUFFER_SIZE = 4096 };
    char buffer[BUFFER_SIZE];
    hll_lexer lexer = hll_lexer_create(file_contents, buffer, BUFFER_SIZE);
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    for (;;) {
        hll_obj *obj;
        hll_read_result parse_result = hll_read(&reader, &obj);

        if (parse_result == HLL_READ_EOF) {
            break;
        } else if (parse_result != HLL_READ_OK) {
            break;
        } else {
            hll_eval(&ctx, obj);
        }
    }

    return EXIT_SUCCESS;
close_file_error:
    if (hll_close_file(file) != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to close file '%s'\n", filename);
    }
error:
    return EXIT_SUCCESS;
}

static int
execute_repl(void) {
    hll_ctx ctx = hll_create_ctx();

    enum { BUFFER_SIZE = 4096 };
    char buffer[BUFFER_SIZE];
    char line_buffer[BUFFER_SIZE];

    int is_interactive = is_stdin_interactive();

    for (;;) {
        if (is_interactive) {
            printf("hololisp> ");
        }
        // TODO: Overflow
        if (fgets(line_buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        hll_lexer lexer = hll_lexer_create(line_buffer, buffer, BUFFER_SIZE);
        hll_reader reader = hll_reader_create(&lexer, &ctx);

        for (;;) {
            hll_obj *obj;
            hll_read_result parse_result = hll_read(&reader, &obj);

            if (parse_result == HLL_READ_EOF) {
                break;
            } else if (parse_result != HLL_READ_OK) {
                break;
            } else {
                hll_print(stdout, hll_eval(&ctx, obj));
                printf("\n");
            }
        }
    }

    return EXIT_SUCCESS;
}

int
main(int argc, char const **argv) {
    run_opts opts = { 0 };
    if (!cli_params(argc, argv, &opts)) {
        return EXIT_FAILURE;
    }

    if (!opts.filename) {
        return execute_repl();
    } else {
        return execute_file(opts.filename);
    }

    return EXIT_SUCCESS;
}
