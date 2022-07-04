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
    SPECIAL_FORM_FUNC_CALL = 0x1,
    SPECIAL_FORM_IF = 0x2,
    SPECIAL_FORM_WHEN = 0x3,
    SPECIAL_FORM_UNLESS = 0x4,
    SPECIAL_FORM_DEFUN = 0x5,
    SPECIAL_FORM_LET = 0x6,
} special_form_kind;

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

typedef struct {
    hll_token_kind kind;
    char const *data;
    size_t data_length;
    size_t line;
    size_t column;
} lexeme;

typedef struct fmt_ast {
    struct fmt_ast *up;
    struct fmt_ast *next;

    bool is_list;
    lexeme *lex;
    special_form_kind special_form;
    size_t element_count;
    struct fmt_ast *first_child;
    struct fmt_ast *last_child;
} fmt_ast;

typedef struct fmt_state_ident {
    struct fmt_state_ident *next;
    uint32_t ident;
} fmt_state_ident;

typedef struct {
    uint32_t ident;
    fmt_state_ident *stack;
    fmt_state_ident *freelist;
    hll_memory_arena *arena;
    hll_string_builder *sb;
} fmt_state;

static void
push_ident(fmt_state *state, uint32_t ident) {
    fmt_state_ident *it = state->freelist;
    if (it) {
        state->freelist = it->next;
        memset(it, 0, sizeof(fmt_state_ident));
    } else {
        it = hll_memory_arena_alloc(state->arena, sizeof(fmt_state_ident));
    }

    it->ident = ident;
    state->ident += ident;

    it->next = state->stack;
    state->stack = it;
}

static void
pop_ident(fmt_state *state) {
    assert(state->stack);
    fmt_state_ident *it = state->stack;
    state->stack = it->next;

    assert(state->ident >= it->ident);
    state->ident -= it->ident;
    it->next = state->freelist;
    state->freelist = it;
}

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
append_child(fmt_ast *ast, fmt_ast *new) {
    ++ast->element_count;
    if (!ast->first_child) {
        ast->first_child = ast->last_child = new;
    } else {
        ast->last_child->next = new;
        ast->last_child = new;
    }
}

static fmt_ast *
build_ast(lexeme *lexemes, size_t lexeme_count, hll_memory_arena *arena) {
    fmt_ast *ast = hll_memory_arena_alloc(arena, sizeof(fmt_ast));
    ast->is_list = true;

    for (size_t i = 0; i < lexeme_count; ++i) {
        lexeme *it = lexemes + i;

        switch (it->kind) {
        default:
            HLL_UNREACHABLE;
            break;
        case HLL_TOK_LPAREN: {
            fmt_ast *new = hll_memory_arena_alloc(arena, sizeof(fmt_ast));
            new->lex = it;
            new->is_list = true;
            new->up = ast;
            append_child(ast, new);
            ast = new;
        } break;
        case HLL_TOK_RPAREN:
            if (ast->first_child && !ast->first_child->is_list) {
                ast->special_form =
                    special_form_from_str(ast->first_child->lex->data);
                if (!ast->special_form) {
                    ast->special_form = SPECIAL_FORM_FUNC_CALL;
                }
            }
            if (ast->up) {
                ast = ast->up;
            }
            break;
        case HLL_TOK_DOT:
        case HLL_TOK_NUMI:
        case HLL_TOK_SYMB:
        case HLL_TOK_EXT_COMMENT:
        case HLL_TOK_QUOTE: {
            fmt_ast *new = hll_memory_arena_alloc(arena, sizeof(fmt_ast));
            new->lex = it;
            append_child(ast, new);
        } break;
        }
    }

    return ast;
}

static void
fmt_newline(fmt_state *state) {
    hll_string_builder_printf(state->sb, "\n");
    if (state->ident) {
        hll_string_builder_printf(state->sb, "%*s", state->ident, "");
    }
}

static void
separate_list_its(fmt_ast *ast, fmt_state *state, bool allow_newline) {
    if (ast->next) {
        if (ast->next->lex->line == ast->lex->line &&
            ast->lex->kind != HLL_TOK_QUOTE) {
            hll_string_builder_printf(state->sb, " ");
        } else if (ast->next->lex->line != ast->lex->line && allow_newline) {
            fmt_newline(state);
        }
    }
}

static void
fmt_ast_recursive(fmt_ast *ast, fmt_state *state) {
    if (ast->is_list) {
        if (ast->first_child->lex->line == ast->last_child->lex->line &&
            (ast->special_form == SPECIAL_FORM_NONE ||
             ast->special_form == SPECIAL_FORM_FUNC_CALL)) {
            hll_string_builder_printf(state->sb, "(");
            for (fmt_ast *child = ast->first_child; child;
                 child = child->next) {
                fmt_ast_recursive(child, state);
                separate_list_its(child, state, false);
            }
            hll_string_builder_printf(state->sb, ")");
        } else {
            hll_string_builder_printf(state->sb, "(");
            switch (ast->special_form) {
            case SPECIAL_FORM_NONE:
                push_ident(state, 1);
                for (fmt_ast *child = ast->first_child; child;
                     child = child->next) {
                    fmt_ast_recursive(child, state);
                    separate_list_its(ast, state, true);
                }
                pop_ident(state);
                break;
            case SPECIAL_FORM_DEFUN: {
                push_ident(state, 2);
                // First 3 elements on same line
                size_t idx = 0;
                for (fmt_ast *child = ast->first_child; child;
                     child = child->next) {
                    fmt_ast_recursive(child, state);
                    if (idx < 2) {
                        hll_string_builder_printf(state->sb, " ");
                    } else if (idx == 2) {
                        fmt_newline(state);
                    } else {
                        separate_list_its(child, state, true);
                    }
                    ++idx;
                }
                pop_ident(state);
            } break;
            case SPECIAL_FORM_LET: {
                // First 2 on same line
                size_t idx = 0;
                bool has_ident = false;
                for (fmt_ast *child = ast->first_child; child;
                     child = child->next) {
                    if (idx < 1) {
                        fmt_ast_recursive(child, state);
                        hll_string_builder_printf(state->sb, " ");
                    } else if (idx == 1) {
                        // (let (<first>
                        push_ident(state, strlen("(let "));
                        fmt_ast_recursive(child, state);
                        pop_ident(state);
                        push_ident(state, 2);
                        fmt_newline(state);
                        has_ident = true;
                    } else {
                        fmt_ast_recursive(child, state);
                        separate_list_its(child, state, true);
                    }
                    ++idx;
                }
                if (has_ident) {
                    pop_ident(state);
                }
            } break;
            case SPECIAL_FORM_WHEN:
            case SPECIAL_FORM_UNLESS: {
                push_ident(state, 2);
                // First 2 on same line
                size_t idx = 0;
                for (fmt_ast *child = ast->first_child; child;
                     child = child->next) {
                    fmt_ast_recursive(child, state);
                    if (idx < 1) {
                        hll_string_builder_printf(state->sb, " ");
                    } else if (idx == 1) {
                        fmt_newline(state);
                    } else {
                        separate_list_its(child, state, true);
                    }
                    ++idx;
                }
                pop_ident(state);
            } break;
            case SPECIAL_FORM_IF: {
                // First 2 on same line
                size_t idx = 0;
                bool has_ident = false;
                for (fmt_ast *child = ast->first_child; child;
                     child = child->next) {
                    fmt_ast_recursive(child, state);
                    if (idx < 1) {
                        hll_string_builder_printf(state->sb, " ");
                    } else if (idx == 1) {
                        // (if <first>...
                        push_ident(state, strlen("(if "));
                        fmt_newline(state);
                        has_ident = true;
                    } else {
                        separate_list_its(child, state, true);
                    }
                    ++idx;
                }
                if (has_ident) {
                    pop_ident(state);
                }
            } break;
            case SPECIAL_FORM_FUNC_CALL: {
                // First 2 on same line
                bool has_ident = false;
                size_t idx = 0;
                for (fmt_ast *child = ast->first_child; child;
                     child = child->next) {
                    fmt_ast_recursive(child, state);
                    if (idx < 1) {
                        hll_string_builder_printf(state->sb, " ");
                    } else if (idx == 1) {
                        // (<name> <first>...
                        has_ident = true;
                        push_ident(state,
                                   1 + ast->first_child->lex->data_length + 1);
                        fmt_newline(state);
                    } else {
                        separate_list_its(child, state, true);
                    }
                    ++idx;
                }
                if (has_ident) {
                    pop_ident(state);
                }
            } break;
            }
            hll_string_builder_printf(state->sb, ")");
        }
    } else {
        switch (ast->lex->kind) {
        default:
            HLL_UNREACHABLE;
            break;
        case HLL_TOK_EXT_COMMENT:
            break;
        case HLL_TOK_NUMI:
        case HLL_TOK_SYMB:
            hll_string_builder_printf(state->sb, "%s", ast->lex->data);
            break;
        case HLL_TOK_DOT:
            hll_string_builder_printf(state->sb, ".");
            break;
        case HLL_TOK_LPAREN:
            hll_string_builder_printf(state->sb, "(");
            break;
        case HLL_TOK_RPAREN:
            hll_string_builder_printf(state->sb, ")");
            break;
        case HLL_TOK_QUOTE:
            hll_string_builder_printf(state->sb, "'");
            break;
        }
    }
}

static void
format_with_ast(fmt_ast *ast, hll_string_builder *sb, hll_memory_arena *arena) {
    fmt_state state = { 0 };
    state.arena = arena;
    state.sb = sb;
    for (fmt_ast *child = ast->first_child; child; child = child->next) {
        fmt_ast_recursive(child, &state);
        hll_string_builder_printf(sb, "\n");
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

    fmt_ast *ast = build_ast(lexemes, lexeme_count, &arena);
    format_with_ast(ast, &sb, &arena);

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
