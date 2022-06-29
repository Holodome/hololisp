#include "formatter.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"

#ifndef HLL_FORMATTER_CLI
#define HLL_FORMATTER_CLI 1
#endif

hllf_settings
hllf_default_settings(void) {
    hllf_settings settings = { 0 };

    settings.tab_size = 2;

    return settings;
}

static size_t
decide_size_for_string_builder(size_t source_size) {
    return source_size * 3 / 2;
}

typedef struct {
    char *buffer;
    size_t buffer_size;
    size_t written;

    size_t grow_inc;
} string_builder;

static string_builder
create_string_builder(size_t size) {
    string_builder b = { 0 };

    b.buffer = calloc(size, 1);
    b.buffer_size = size;
    b.grow_inc = 4096;

    return b;
}

__attribute__((unused))
static void
string_builder_printf(string_builder *b, char const *fmt, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    size_t written = vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (b->written + written > b->buffer_size) {
        b->buffer_size += b->grow_inc;
        b->buffer = realloc(b->buffer, b->buffer_size);
    }

    strcpy(b->buffer + b->written, buffer);
}

char const *
hllf_format(char const *source, size_t source_length, hllf_settings *settings) {
    (void)settings;

    size_t sb_size = decide_size_for_string_builder(source_length);
    string_builder sb = create_string_builder(sb_size);

    char string_buf[4096];
    hll_lexer lexer = hll_lexer_create(source, string_buf, sizeof(string_buf));

    for (;;) {
        hll_lex_result result = hll_lexer_peek(&lexer);
        (void)result;

        if (lexer.token_kind == HLL_TOK_EOF) {
            break;
        }

        hll_lexer_eat(&lexer);
    }

    return sb.buffer;
}

void
hllf_format_free(char const *text) {
    free((void *)text);
}

#if HLL_FORMATTER_CLI
typedef struct {
    int is_valid;
    char const *in_filename;
    char const *out_filename;
} cli_options;

static cli_options
parse_cli_args(int argc, char const **argv) {
    cli_options opts = { 0 };

    if (argc == 2) {
        opts.in_filename = argv[0];
        opts.out_filename = argv[1];
        opts.is_valid = 1;
    }

    return opts;
}

int
main(int argc, char const **argv) {
    cli_options opts = parse_cli_args(argc - 1, argv + 1);
    if (!opts.is_valid) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif
