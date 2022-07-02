#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_reporter.h"
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
        "USAGE: hololisp file\n");
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

    hll_error_reporter reporter = { 0 };
    hll_init_error_reporter(&reporter);
    hll_ctx ctx = hll_create_ctx(&reporter);

    enum { BUFFER_SIZE = 4096 };
    char buffer[BUFFER_SIZE];
    hll_lexer lexer = hll_lexer_create(file_contents, buffer, BUFFER_SIZE);
    hll_reader reader = hll_reader_create(&lexer, &ctx);
    reader.filename = filename;

#if 0
    for (;;) {
        hll_lex_result res = hll_lexer_peek(&lexer);
        if (res || lexer.token_kind == HLL_TOK_EOF) {
            break;
        }

        printf("%u: %u %u\n", lexer.token_kind, lexer.token_line,
               lexer.token_column);
        hll_lexer_eat(&lexer);
    }
#else
    for (;;) {
        hll_obj *obj;
        hll_source_location current_location = { 0 };
        hll_reader_get_source_loc(&reader, &current_location);
        hll_read_result parse_result = hll_read(&reader, &obj);

        if (parse_result == HLL_READ_EOF) {
            break;
        } else if (parse_result != HLL_READ_OK) {
            break;
        } else {
            hll_eval(&ctx, obj);
            if (ctx.reporter->has_error) {
                hll_report_error_verbose(ctx.reporter, &current_location,
                                         "Eval failed");
                break;
            }
        }
    }
#endif

    return EXIT_SUCCESS;
close_file_error:
    if (hll_close_file(file) != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to close file '%s'\n", filename);
    }
error:
    return EXIT_FAILURE;
}

static int
execute_repl(void) {
    hll_error_reporter reporter = { 0 };
    hll_init_error_reporter(&reporter);
    hll_ctx ctx = hll_create_ctx(&reporter);

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
                hll_print(&ctx, stdout, hll_eval(&ctx, obj));
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
