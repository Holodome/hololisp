#include "lexer.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Characters that are considered symbols in lisp
#define SYMB_CHARS "~!@#$%^&*-_=+:/?<>"

void
lisp_lexer_init(lisp_lexer *lex, char *data, char *tok_buf,
                uint32_t tok_buf_capacity) {
    lex->cursor = data;
    lex->tok_buf = tok_buf;
    lex->tok_buf_capacity = tok_buf_capacity;
}

bool
lisp_lexer_peek(lisp_lexer *lex) {
    if (lex->use_old) {
        goto out;
    }

    lex->use_old = true;
    lex->kind = TOK_NONE;
    while (lex->kind == TOK_NONE) {
        char cp = *lex->cursor;
        if (!cp) {
            lex->kind = TOK_EOF;
            continue;
        }

        if (isspace(cp)) {
            do {
                cp = *++lex->cursor;
            } while (cp && isspace(cp));
            continue;
        }

        if (cp == ';') {
            do {
                cp = *++lex->cursor;
            } while (cp && cp != '\n');

            if (cp == '\n') {
                cp = *++lex->cursor;
            }
            continue;
        }

        if (cp == '\'') {
            ++lex->cursor;
            lex->kind = TOK_QUOTE;
            continue;
        }

        if (cp == '.') {
            ++lex->cursor;
            lex->kind = TOK_DOT;
            continue;
        }

        if (cp == '(') {
            ++lex->cursor;
            lex->kind = TOK_LPAREN;
            continue;
        }

        if (cp == ')') {
            ++lex->cursor;
            lex->kind = TOK_RPAREN;
            continue;
        }

        char *write_cursor = lex->tok_buf;
        char *write_eof = lex->tok_buf + lex->tok_buf_capacity;

        *write_cursor++ = cp;
        cp = *++lex->cursor;

        while (cp && (isalnum(cp) || strchr(SYMB_CHARS, cp))) {
            if (write_cursor < write_eof) {
                *write_cursor++ = cp;
            }
            cp = *++lex->cursor;
        }

        *write_cursor = 0;

        char *test = lex->tok_buf;
        if (*test == '-') {
            ++test;
        }
        while (*test && isdigit(*test)) {
            ++test;
        }
        if (*test == 0) {
            lex->kind = TOK_NUMI;
            lex->numi = atoi(lex->tok_buf);
        } else {
            lex->symb_len = write_cursor - lex->tok_buf;
            lex->kind = TOK_SYMB;
        }
        continue;
    }

out:
    return lex->kind != TOK_EOF;
}

void
lisp_lexer_eat(lisp_lexer *lex) {
    lex->use_old = false;
}

void
lisp_lexer_eat_peek(lisp_lexer *lex) {
    lisp_lexer_eat(lex);
    lisp_lexer_peek(lex);
}

