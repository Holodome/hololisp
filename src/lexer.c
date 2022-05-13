#include "lexer.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "general.h"
#include "unicode.h"

hll_lexer
hll_lexer_init(char const *cursor, char *buffer, uint32_t buffer_size) {
    hll_lexer lex = {0};

    lex.cursor      = cursor;
    lex.buffer      = buffer;
    lex.buffer_size = buffer_size;

    return lex;
}

static hll_bool32
is_codepoint_a_symbol(uint32_t codepoint) {
    static char const SYMB_CHARS[] = "~!@#$%^&*-_=+:/?<>";
    return isalnum(codepoint) || strchr(SYMB_CHARS, codepoint);
}

/* Function behaviourally similar to strtoll.
 * But, they are not present in c89 and do some stuff that we don't want, so
 * let's roll our own. Since this is function that is used only internally, we
 * can make assumption that 'start' is a valid c-string
 */
static hll_bool32
is_lisp_numberi(char const *start, int64_t *number) {
    hll_bool32 result     = FALSE;
    int64_t    test       = 0;
    int64_t    multiplier = 1;

    if (*start == '-') {
        ++start;
        multiplier = -1;
    }

    while (*start && isdigit(*start)) {
        test   = test * 10 + (*start++ - '0');
        result = TRUE;
    }

    if (result) {
        test *= multiplier;
        *number = test;
    }

    return result;
}

/*
 * Reads lisp symbol from given cursor and writes it to buffer.
 * If buffer size is less that symbol length, skip characters until symbol end.
 * Return number of bytes written, which can be used to detect buffer undeflow.
 * Number of bytes doesn't include terminating symbol.
 */
static uint32_t
eat_symbol(char *buffer, uint32_t buffer_size, char const **cursor) {
    uint32_t written = 0;

    char const *temp = *cursor;
    while (*temp && is_codepoint_a_symbol(*temp)) {
        if (written < buffer_size) {
            buffer[written++] = *temp;
        }
        ++temp;
    }

    if (buffer_size - written) {
        buffer[written] = 0;
    }

    *cursor = temp;

    return written;
}

hll_lex_result
hll_lexer_token(hll_lexer *lexer) {
    hll_lex_result result = HLL_LOK;

    /* This is short-circuiting branch for testing null buffer.
     This shouldn't be ever hit during normal execution so we don't care if this
     is ugly */
    hll_bool32 is_finished = lexer->cursor == NULL;
    if (is_finished) {
        lexer->token_kind = HLL_LTOK_EOF;
    }

    while (!is_finished) {
        uint8_t cp = *lexer->cursor;

        /* Non-ASCII */

        if (cp & 0x80) {
            uint32_t codepoint;

            /* We treat non ASCII symbol in unexpected place as string
             * Later it we can change to allow UTF8 symbols too */

            lexer->token_start = lexer->cursor;
            /* TODO: utf8 decoding errors */
            lexer->cursor     = utf8_decode(lexer->cursor, &codepoint);
            lexer->token_kind = HLL_LTOK_OTHER;

            if (lexer->cursor - lexer->token_start + 1 > lexer->buffer_size) {
                size_t length = lexer->cursor - lexer->token_start;

                memcpy(lexer->buffer, lexer->token_start, length);
                lexer->buffer[length] = 0;
            }

            break;
        }

        /* EOF */

        if (!cp) {
            lexer->token_kind = HLL_LTOK_EOF;
            break;
        }

        /* Spaces */

        if (isspace(*lexer->cursor)) {
            do {
                cp = *++lexer->cursor;
            } while (cp && isspace(cp));
            continue;
        }

        /* Comments */

        if (cp == ';') {
            do {
                cp = *++lexer->cursor;
            } while (cp && cp != '\n');

            if (cp == '\n') {
                cp = *++lexer->cursor;
            }
            continue;
        }

        /* Individual characters */

        if (cp == '\'') {
            ++lexer->cursor;
            lexer->token_kind = HLL_LTOK_QUOTE;
            continue;
        }

        if (cp == '.') {
            ++lexer->cursor;
            lexer->token_kind = HLL_LTOK_DOT;
            continue;
        }

        if (cp == '(') {
            ++lexer->cursor;
            lexer->token_kind = HLL_LTOK_LPAREN;
            continue;
        }

        if (cp == ')') {
            ++lexer->cursor;
            lexer->token_kind = HLL_LTOK_RPAREN;
            continue;
        }

        {
            lexer->token_strlen =
                eat_symbol(lexer->buffer, lexer->buffer_size, &lexer->cursor);
            if (lexer->token_strlen != lexer->cursor - lexer->token_start) {
                result = HLL_LBUF_OVERFLOW;
            }

            /* Check if it is a number */
            if (is_lisp_numberi(lexer->token_start, &lexer->token_i)) {
                lexer->token_kind = HLL_LTOK_NUMI;
            } else {
                lexer->token_kind = HLL_LTOK_SYMB;
            }
        }
    }

    return result;
}

