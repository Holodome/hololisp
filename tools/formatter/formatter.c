#include "formatter.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_lexer.h"
#include "hll_utils.h"

#ifndef HLL_FORMATTER_CLI
#define HLL_FORMATTER_CLI 1
#endif

typedef enum {
    SPECIAL_FORM_NONE = 0x0,
    SPECIAL_FORM_IF = 0x1,
    SPECIAL_FORM_WHEN = 0x2,
    SPECIAL_FORM_UNLESS = 0x3,
    SPECIAL_FORM_DEFUN = 0x4,
    SPECIAL_FORM_LET = 0x5,
} special_form_kind;

typedef struct fmt_list_stack {
    struct fmt_list_stack *next;
    uint32_t element_count;
    uint32_t ident;
    special_form_kind special_form;
} fmt_list_stack;

typedef struct {
    hll_memory_arena arena;
    uint32_t ident;
    fmt_list_stack *ident_stack;
    fmt_list_stack *ident_freelist;
    uint32_t parens_depth;
} formatter_state;

typedef struct {
    hll_token_kind kind;
    char const *data;
    size_t data_length;
    size_t line;
    size_t column;
} lexeme;

static size_t
decide_size_for_string_builder(size_t source_size) {
    return source_size * 3 / 2;
}

static size_t
count_lexemes(char const *source) {
    hll_lexer lexer = hll_lexer_create(source, NULL, 0);
    lexer.is_ext = 1;

    size_t token_count = 0;
    for (;;) {
        hll_lex_result result = hll_lexer_peek(&lexer);
        (void)result;

        if (lexer.token_kind == HLL_TOK_EOF) {
            break;
        }

        ++token_count;
        hll_lexer_eat(&lexer);
    }

    return token_count;
}

static void
read_lexemes(char const *source, lexeme *lexemes, size_t lexeme_count,
             hll_memory_arena *arena) {
    size_t cursor = 0;

    char buffer[4096];
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    lexer.is_ext = 1;

    for (;;) {
        hll_lex_result result = hll_lexer_peek(&lexer);
        if (result == HLL_LEX_BUF_OVERFLOW) {
            fprintf(stderr, "BUFFER OVERFLOW!!!\n");
            exit(1);
        }

        if (lexer.token_kind == HLL_TOK_EOF) {
            break;
        }

        assert(cursor < lexeme_count);
        lexeme *it = lexemes + cursor;
        it->kind = lexer.token_kind;
        it->line = lexer.token_line;
        it->column = lexer.token_column;
        switch (lexer.token_kind) {
        default:
            HLL_UNREACHABLE;
            break;
        case HLL_TOK_NUMI:
        case HLL_TOK_SYMB:
        case HLL_TOK_EXT_COMMENT:
            it->data = hll_memory_arena_alloc(arena, lexer.token_length + 1);
            it->data_length = lexer.token_length;
            memcpy((void *)it->data, lexer.buffer, lexer.token_length);
            break;
        case HLL_TOK_DOT:
        case HLL_TOK_LPAREN:
        case HLL_TOK_RPAREN:
        case HLL_TOK_QUOTE:
            break;
        }

        ++cursor;
        hll_lexer_eat(&lexer);
    }

    assert(cursor == lexeme_count);
}

static void
read_lisp_code_into_tokens(char const *source, lexeme **lexemes,
                           size_t *lexeme_count, hll_memory_arena *arena) {
    size_t count = count_lexemes(source);
    lexeme *its = hll_memory_arena_alloc(arena, sizeof(lexeme) * count);
    read_lexemes(source, its, count, arena);

    *lexemes = its;
    *lexeme_count = count;
}

static void
push_ident(formatter_ctx *ctx, uint32_t ident) {
    fmt_list_stack *it = ctx->ident_freelist;
    if (!it) {
        it = hll_memory_arena_alloc(&ctx->arena, sizeof(fmt_list_stack));
    } else {
        memset(it, 0, sizeof(fmt_list_stack));
    }
    it->ident = ident;

    it->next = ctx->ident_stack;
    ctx->ident_stack = it;
    ctx->ident += ident;
}

static void
pop_ident(formatter_ctx *ctx) {
    assert(ctx->ident_stack);
    fmt_list_stack *it = ctx->ident_stack;
    ctx->ident_stack = it->next;

    assert(ctx->ident >= it->ident);
    ctx->ident -= it->ident;

    it->next = ctx->ident_freelist;
    ctx->ident_freelist = it;
}

static void
ident(formatter_ctx *ctx, hll_string_builder *sb) {
    if (ctx->ident) {
        hll_string_builder_printf(sb, "%*s", ctx->ident, "");
    }
}

static void
format_with_tokens(token_array *array, hll_string_builder *sb) {
    formatter_ctx ctx = { 0 };

    /* token _last_token = { 0 }; */
    /* token *last_token = &_last_token; */
    for (size_t cursor = 0; cursor < array->token_count; ++cursor) {
        token *tok = array->tokens + cursor;
        /* { */
        /*     size_t newline_count = tok->line - last_token->line; */
        /*     while (newline_count--) { */
        /*         string_builder_printf(sb, "\n"); */
        /*     } */
        /* } */

        switch (tok->kind) {
        default:
            HLL_UNREACHABLE;
            break;
        case HLL_TOK_NUMI:
        case HLL_TOK_SYMB:
            if (ctx.ident_stack && ctx.ident_stack->element_count) {
                hll_string_builder_printf(sb, "\n");
                ident(&ctx, sb);
            }
            hll_string_builder_printf(sb, "%s", tok->data);
            if (ctx.ident_stack) {
                ++ctx.ident_stack->element_count;
            }
            break;
        case HLL_TOK_EXT_COMMENT:
            break;
        case HLL_TOK_DOT:
            hll_string_builder_printf(sb, ".");
            break;
        case HLL_TOK_LPAREN:
            ident(&ctx, sb);
            if (ctx.ident_stack && ctx.ident_stack->element_count) {
                ++ctx.ident_stack->element_count;
                hll_string_builder_printf(sb, "\n");
                ident(&ctx, sb);
            }
            if (ctx.ident_stack) {
                ++ctx.ident_stack->element_count;
            }
            hll_string_builder_printf(sb, "(");
            push_ident(&ctx, 2);
            break;
        case HLL_TOK_RPAREN:
            hll_string_builder_printf(sb, ")");
            pop_ident(&ctx);
            if (!ctx.ident_stack) {
                hll_string_builder_printf(sb, "\n");
            }
            break;
        case HLL_TOK_QUOTE:
            hll_string_builder_printf(sb, "'");
            break;
        }

        /* last_token = tok; */
    }

    hll_memory_arena_clear(&ctx.arena);
}

hllf_format_result
hllf_format(char const *source, size_t source_length) {
    token_array tokens = read_lisp_code_into_tokens(source);
#if 0
    for (size_t i = 0; i < tokens.token_count; ++i) {
        printf("Token %zu:%zu:%zu: kind: %u, data; %s\n", i,
               tokens.tokens[i].line, tokens.tokens[i].column,
               tokens.tokens[i].kind, tokens.tokens[i].data);
    }
#endif

    size_t sb_size = decide_size_for_string_builder(source_length);
    hll_string_builder sb = hll_create_string_builder(sb_size);

    format_with_tokens(&tokens, &sb);
    hll_memory_arena_clear(&tokens.arena);

    hllf_format_result result = { 0 };
    result.data = sb.buffer;
    result.data_size = sb.written;

    return result;
}

void
hllf_format_free(char const *text) {
    free((void *)text);
}

#if HLL_FORMATTER_CLI
typedef struct {
    bool is_valid;
    char const *in_filename;
    char const *out_filename;
} cli_options;

static cli_options
parse_cli_args(int argc, char const **argv) {
    cli_options opts = { 0 };

    if (argc == 2) {
        opts.in_filename = argv[0];
        opts.out_filename = argv[1];
        opts.is_valid = true;
    }

    return opts;
}

int
main(int argc, char const **argv) {
    cli_options opts = parse_cli_args(argc - 1, argv + 1);
    if (!opts.is_valid) {
        fprintf(stderr, "hololisp-format <in> <out>\n");
        return EXIT_FAILURE;
    }

    char *file_contents = NULL;
    size_t file_size = 0;
    hll_fs_io_result read_result =
        hll_read_entire_file(opts.in_filename, &file_contents, &file_size);
    if (read_result != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to load code to format from file '%s': %s\n",
                opts.in_filename, hll_get_fs_io_result_string(read_result));
        return EXIT_FAILURE;
    }

    hllf_format_result formatted = hllf_format(file_contents, file_size);

    hll_fs_io_result write_result = hll_write_to_file(
        opts.out_filename, formatted.data, formatted.data_size);
    if (read_result != HLL_FS_IO_OK) {
        fprintf(stderr, "Failed to write formatted code to file '%s': %s\n",
                opts.out_filename, hll_get_fs_io_result_string(write_result));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#endif
