#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>

hll_lexer
hll_lexer_create(char const *cursor, char *buffer, uint32_t buffer_size) {
    hll_lexer lex = { 0 };

    lex.cursor = cursor;
    lex.buffer = buffer;
    lex.buffer_size = buffer_size;

    return lex;
}

static bool
is_codepoint_a_symbol(uint32_t codepoint) {
    static char const SYMB_CHARS[] = "+-*/@$%^&_=<>~.?![]{}";
    return isalnum(codepoint) || strchr(SYMB_CHARS, codepoint);
}

static bool
try_to_parse_number(char const *buffer, int64_t *number) {
    bool is_number = false;

    char const *cursor = buffer;
    if (cursor) {
        int64_t multiplier = 1;
        if (*cursor == '+') {
            ++cursor;
        } else if (*cursor == '-') {
            ++cursor;
            multiplier = -1;
        }

        // TODO: Report overflow
        int64_t result = 0;
        while (isdigit(*cursor)) {
            result = result * 10 + (*cursor++ - '0');
            is_number = true;
        }

        if (is_number && (is_number = (*cursor == 0))) {
            *number = result * multiplier;
        }
    }

    return is_number;
}

typedef struct {
    hll_lex_result result;
    size_t written;
    size_t length;
    char const *cursor;
} eat_symbol_result;

/*
 * Reads lisp symbol from given cursor and writes it to buffer.
 * If buffer size is less that symbol length, skip characters until symbol end.
 * Return number of bytes written, which can be used to detect buffer undeflow.
 * Number of bytes doesn't include terminating symbol.
 */
static eat_symbol_result
eat_symbol(char *buffer, size_t buffer_size, char const *cursor_) {
    eat_symbol_result result = { .result = HLL_LEX_OK, .cursor = cursor_ };

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
            result.result = HLL_LEX_BUF_OVERFLOW;
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

hll_lex_result
hll_lexer_peek(hll_lexer *lexer) {
    hll_lex_result result = HLL_LEX_OK;

    /* This is short-circuiting branch for testing null buffer.
     This shouldn't be ever hit during normal execution so we don't care if this
     is ugly */
    int is_finished = lexer->cursor == NULL ||
                      (lexer->already_met_eof && !lexer->should_return_old);
    if (is_finished) {
        lexer->token_kind = HLL_LTOK_EOF;
    } else {
        is_finished = lexer->should_return_old;
    }

    while (!is_finished) {
        uint8_t cp = *lexer->cursor;
        lexer->token_start = lexer->cursor;

        //
        // EOF
        //
        if (!cp) {
            lexer->token_kind = HLL_LTOK_EOF;
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
            do {
                cp = *++lexer->cursor;
            } while (cp != '\n' && cp);
        }
        //
        // Individual characters
        //
        else if (cp == '(') {
            ++lexer->cursor;
            lexer->token_kind = HLL_LTOK_LPAREN;
            is_finished = 1;
        } else if (cp == ')') {
            ++lexer->cursor;
            lexer->token_kind = HLL_LTOK_RPAREN;
            is_finished = 1;
        } else if (cp == '\'') {
            ++lexer->cursor;
            lexer->token_kind = HLL_LTOK_QUOTE;
            is_finished = 1;
        }
        //
        // Symbol
        //
        else {
            eat_symbol_result eat_symb_res =
                eat_symbol(lexer->buffer, lexer->buffer_size, lexer->cursor);

            // TODO: Don't like that error code is taken from eat_symb_res
            if ((result = eat_symb_res.result) != HLL_LEX_OK) {
                lexer->token_kind = HLL_LTOK_SYMB;
            } else {
                // Now we perform checks
                // TODO: don't like that we have to parse multpile times
                if (try_to_parse_number(lexer->buffer, &lexer->token_int)) {
                    lexer->token_kind = HLL_LTOK_NUMI;
                } else {
                    int is_all_dots = 1;
                    for (size_t i = 0; i < eat_symb_res.length && is_all_dots;
                         ++i) {
                        if (lexer->cursor[i] != '.') {
                            is_all_dots = 0;
                        }
                    }

                    if (is_all_dots && eat_symb_res.length == 1) {
                        lexer->token_kind = HLL_LTOK_DOT;
                    } else if (is_all_dots) {
                        lexer->token_kind = HLL_LTOK_DOT;
                        result = HLL_LEX_ALL_DOT_SYMB;
                    } else {
                        lexer->token_kind = HLL_LTOK_SYMB;
                    }
                }
            }

            lexer->cursor = eat_symb_res.cursor;
            lexer->token_length = eat_symb_res.length;
            is_finished = 1;
        }
    }

    lexer->should_return_old = 1;

    return result;
}

void
hll_lexer_eat(hll_lexer *lexer) {
    /* Forbid eating EOF. This should not be done anyway (and has no sense). */
    if (lexer->token_kind != HLL_LTOK_EOF) {
        lexer->should_return_old = 0;
    }
}

hll_lex_result
hll_lexer_eat_peek(hll_lexer *lexer) {
    hll_lexer_eat(lexer);
    return hll_lexer_peek(lexer);
}
