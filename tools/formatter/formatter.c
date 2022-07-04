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

typedef struct fmt_list {
    struct fmt_list *next;
    uint32_t element_count;
    uint32_t ident;
    special_form_kind special_form;
} fmt_list;

typedef struct {
    hll_memory_arena *arena;
    fmt_list *stack;
    fmt_list *freelist;
} fmt_state;

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
read_lisp_code_into_lexemes(char const *source, lexeme **lexemes,
                            size_t *lexeme_count, hll_memory_arena *arena) {
    size_t count = count_lexemes(source);
    lexeme *its = hll_memory_arena_alloc(arena, sizeof(lexeme) * count);
    read_lexemes(source, its, count, arena);

    *lexemes = its;
    *lexeme_count = count;
}

static void
push_list(fmt_state *state) {
    fmt_list *it = state->freelist;
    if (it) {
        state->freelist = it->next;
        memset(it, 0, sizeof(fmt_list));
    } else {
        it = hll_memory_arena_alloc(state->arena, sizeof(fmt_list));
    }

    it->next = state->stack;
    state->stack = it;
}

static void
pop_list(fmt_state *state) {
    fmt_list *it = state->stack;
    assert(it != NULL);
    state->stack = it->next;

    it->next = state->freelist;
    state->freelist = it;
}

static special_form_kind
special_form_from_str(char const *str) {
    special_form_kind kind = SPECIAL_FORM_NONE;

    if (strcmp(str, "if") == 0) {
        kind = SPECIAL_FORM_IF;
    } else if (strcmp(str, "when") == 0) {
        kind = SPECIAL_FORM_WHEN;
    } else if (strcmp(str, "unless") == 0) {
        kind = SPECIAL_FORM_UNLESS;
    } else if (strcmp(str, "defun") == 0) {
        kind = SPECIAL_FORM_DEFUN;
    } else if (strcmp(str, "let") == 0) {
        kind = SPECIAL_FORM_LET;
    }

    return kind;
}

static void
process_symb(fmt_state *state, lexeme *it, hll_string_builder *sb) {
    assert(it->kind == HLL_TOK_SYMB);

    fmt_list *stack = state->stack;
    if (!stack) {
        hll_string_builder_printf(sb, "\n%s\n", it->data);
    } else {
        if (stack->element_count == 0) {
            stack->special_form = special_form_from_str(it->data);
        } else {
            switch (stack->special_form) {
            case SPECIAL_FORM_NONE:
                break;
            case SPECIAL_FORM_IF:
                break;
            case SPECIAL_FORM_WHEN:
                break;
            case SPECIAL_FORM_UNLESS:
                break;
            case SPECIAL_FORM_DEFUN:
                break;
            case SPECIAL_FORM_LET:
                break;
            }
        }
    }
}

static void
process_lexeme(fmt_state *state, lexeme *it, hll_string_builder *sb) {
    switch (it->kind) {
    default:
        break;
    case HLL_TOK_LPAREN:
        push_list(state);
        break;
    case HLL_TOK_RPAREN:
        pop_list(state);
        break;
    case HLL_TOK_SYMB:
        process_symb(state, it, sb);
        break;
    }
}

static void
format_with_lexemes(lexeme *lexemes, size_t lexeme_count,
                    hll_string_builder *sb, hll_memory_arena *arena) {
    fmt_state state = { 0 };
    state.arena = arena;

    for (size_t idx = 0; idx < lexeme_count; ++idx) {
        lexeme *it = lexemes + idx;
        process_lexeme(&state, it, sb);
    }
}

hllf_format_result
hllf_format(char const *source, size_t source_length) {
    hll_memory_arena arena = { 0 };

    lexeme *lexemes = NULL;
    size_t lexeme_count = 0;
    read_lisp_code_into_lexemes(source, &lexemes, &lexeme_count, &arena);

    size_t sb_size = decide_size_for_string_builder(source_length);
    hll_string_builder sb = hll_create_string_builder(sb_size);

    format_with_lexemes(lexemes, lexeme_count, &sb, &arena);

    hllf_format_result result = { 0 };
    result.data = sb.buffer;
    result.data_size = sb.written;

    hll_memory_arena_clear(&arena);

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
