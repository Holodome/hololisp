#include "hll_lexer.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

#define MAX_ALLOWED_INT_VALUE (INT64_MAX / 10LL)

char const *
hll_lex_result_str(hll_lex_result result) {
    char const *str = NULL;

    switch (result) {
    case HLL_LEX_OK:
        str = "ok";
        break;
    case HLL_LEX_BUF_OVERFLOW:
        str = "buffer overflow";
        break;
    case HLL_LEX_ALL_DOT_SYMB:
        str = "symbol consists of all dots";
        break;
    case HLL_LEX_INT_TOO_BIG:
        str = "integer is too big";
        break;
    }

    return str;
}

hll_lexer
hll_lexer_create(char const *cursor, char *buffer, uint32_t buffer_size) {
    hll_lexer lex = { 0 };

    lex.cursor = cursor;
    lex.buffer = buffer;
    lex.buffer_size = buffer_size;
    lex.line_start = cursor;

    return lex;
}

static int
is_codepoint_a_symbol(uint32_t codepoint) {
    static char const SYMB_CHARS[] = "+-*/@$%^&_=<>~.?![]{}";
    return isalnum(codepoint) || strchr(SYMB_CHARS, codepoint);
}

typedef struct {
    int is_valid;
    int overflow;
} parse_number_result;

static parse_number_result
try_to_parse_number(char const *start, char const *end, int64_t *number) {
    parse_number_result result = { 0 };
    int is_number = 0;

    char const *cursor = start;
    if (cursor) {
        int64_t multiplier = 1;
        if (*cursor == '+') {
            ++cursor;
        } else if (*cursor == '-') {
            ++cursor;
            multiplier = -1;
        }

        int64_t value = 0;
        while (isdigit(*cursor)) {
            if (value >= MAX_ALLOWED_INT_VALUE) {
                result.overflow = 1;
            }

            value = value * 10 + (*cursor++ - '0');
            is_number = 1;
        }

        if (is_number && cursor == end) {
            *number = value * multiplier;
            result.is_valid = 1;
        }
    }

    return result;
}

typedef struct {
    int overflow;
    size_t written;
    size_t length;
    char const *cursor;
} eat_symbol_result;

/*
 * Reads lisp symbol from given cursor and writes it to buffer.
 * If buffer size is less that symbol length, skip characters until symbol end.
 * Return number of bytes written, which can be used to detect buffer overflow.
 * Number of bytes doesn't include terminating symbol.
 */
static eat_symbol_result
eat_symbol(char *buffer, size_t buffer_size, char const *cursor_) {
    eat_symbol_result result = { .cursor = cursor_ };

    char *write = buffer;
    char *buffer_eof = buffer + buffer_size;

    if (write) {
        while (*result.cursor && is_codepoint_a_symbol(*result.cursor)) {
            char cp = *result.cursor++;
            ++result.length;
            if (write < buffer_eof) {
                *write++ = cp;
            }
        }

        if (buffer_eof != write) {
            result.written = result.length = write - buffer;
            *write = 0;
        } else {
            result.overflow = 1;
        }
    } else {
        result.written = 0;
        while (*result.cursor && is_codepoint_a_symbol(*result.cursor)) {
            ++result.length;
            ++result.cursor;
        }
    }

    return result;
}

typedef struct {
    int should_generate_token;
    int is_overflow;
} handle_comment_result;

static handle_comment_result
handle_comment(hll_lexer *lexer) {
    assert(*lexer->cursor == ';');
    handle_comment_result result = { 0 };

    if (!lexer->is_ext) {
        uint8_t cp;
        do {
            cp = *++lexer->cursor;
        } while (cp != '\n' && cp);
    } else if (lexer->buffer != NULL) {
        char *write = lexer->buffer;
        uint8_t cp;
        do {
            cp = *++lexer->cursor;
            // TODO: This is incorrect in cases of loop end due to post
            // condition
            if (write < lexer->buffer + lexer->buffer_size && cp != '\n') {
                *write++ = cp;
            }
        } while (cp != '\n' && cp);

        if (lexer->buffer + lexer->buffer_size != write) {
            lexer->token_length = write - lexer->buffer;
            *write = 0;
        } else {
            result.is_overflow = 1;
        }
        result.should_generate_token = 1;
    } else {
        uint8_t cp;
        do {
            cp = *++lexer->cursor;
        } while (cp != '\n' && cp);
        result.should_generate_token = 1;
    }

    return result;
}

hll_lex_result
hll_lexer_peek(hll_lexer *lexer) {
    hll_lex_result result = HLL_LEX_OK;

    /* This is short-circuiting branch for testing null buffer.
     This shouldn't be ever hit during normal execution, so we don't care if
     this is ugly */
    int is_finished = lexer->cursor == NULL ||
                      (lexer->already_met_eof && !lexer->should_return_old);
    if (is_finished) {
        lexer->token_kind = HLL_TOK_EOF;
    } else {
        is_finished = lexer->should_return_old;
        if (lexer->should_return_old) {
            result = lexer->old_result;
        }
    }

    while (!is_finished) {
        uint8_t cp = *(uint8_t *)lexer->cursor;
        lexer->token_line = lexer->line;
        lexer->token_column = lexer->cursor - lexer->line_start;

        //
        // EOF
        //
        if (!cp) {
            lexer->token_kind = HLL_TOK_EOF;
            lexer->already_met_eof = 1;
            is_finished = 1;
        }
        //
        // Spaces (skip)
        //
        else if (isspace(cp)) {
            do {
                if (cp == '\n') {
                    ++lexer->line;
                    lexer->line_start = lexer->cursor + 1;
                }
                cp = *++lexer->cursor;
            } while (isspace(cp) && cp);
        }
        //
        // Comments
        //
        else if (cp == ';') {
            handle_comment_result res = handle_comment(lexer);
            if (res.should_generate_token) {
                if (res.is_overflow) {
                    result = HLL_LEX_BUF_OVERFLOW;
                }
                is_finished = 1;
                lexer->token_kind = HLL_TOK_EXT_COMMENT;
            }
        }
        //
        // Individual characters
        //
        else if (cp == '(') {
            ++lexer->cursor;
            lexer->token_kind = HLL_TOK_LPAREN;
            is_finished = 1;
        } else if (cp == ')') {
            ++lexer->cursor;
            lexer->token_kind = HLL_TOK_RPAREN;
            is_finished = 1;
        } else if (cp == '\'') {
            ++lexer->cursor;
            lexer->token_kind = HLL_TOK_QUOTE;
            is_finished = 1;
        }
        //
        // Symbol
        //
        else {
            eat_symbol_result eat_symb_res =
                eat_symbol(lexer->buffer, lexer->buffer_size, lexer->cursor);

            lexer->token_kind = HLL_TOK_SYMB;
            if (eat_symb_res.overflow) {
                result = HLL_LEX_BUF_OVERFLOW;
            } else {
                lexer->token_kind = HLL_TOK_SYMB;
                parse_number_result parse_number_res = try_to_parse_number(
                    lexer->cursor, eat_symb_res.cursor, &lexer->token_int);
                if (parse_number_res.is_valid) {
                    lexer->token_kind = HLL_TOK_NUMI;
                    if (parse_number_res.overflow) {
                        result = HLL_LEX_INT_TOO_BIG;
                    }
                } else {
                    int is_all_dots = 1;
                    for (size_t i = 0; i < eat_symb_res.length && is_all_dots;
                         ++i) {
                        if (lexer->cursor[i] != '.') {
                            is_all_dots = 0;
                        }
                    }

                    if (is_all_dots && eat_symb_res.length == 1) {
                        lexer->token_kind = HLL_TOK_DOT;
                    } else if (is_all_dots) {
                        lexer->token_kind = HLL_TOK_DOT;
                        result = HLL_LEX_ALL_DOT_SYMB;
                    } else {
                        lexer->token_kind = HLL_TOK_SYMB;
                    }
                }
            }

            lexer->cursor = eat_symb_res.cursor;
            lexer->token_length = eat_symb_res.length;
            is_finished = 1;
        }
    }

    if (!lexer->should_return_old) {
        lexer->should_return_old = 1;
        lexer->old_result = result;
    }

    return result;
}

void
hll_lexer_eat(hll_lexer *lexer) {
    /* Forbid eating EOF. This should not be done anyway (and has no sense). */
    if (lexer->token_kind != HLL_TOK_EOF) {
        lexer->should_return_old = 0;
    }
}

hll_lex_result
hll_lexer_eat_peek(hll_lexer *lexer) {
    hll_lexer_eat(lexer);
    return hll_lexer_peek(lexer);
}
