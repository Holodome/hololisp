#include "reader.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "lexer.h"
#include "lisp.h"

static hll_read_result
report_error(hll_read_result result, hll_reader *reader, char const *format,
             ...) {
    va_list args;
    va_start(args, format);
    fprintf(reader->ctx->file_outerr, "Reader error at %u:%u: %s:\n",
            reader->lexer->token_line, reader->lexer->token_column,
            hll_read_result_str(result));
    vfprintf(reader->ctx->file_outerr, format, args);
    fprintf(reader->ctx->file_outerr, "\n");

    return result;
}

char const *
hll_read_result_str(hll_read_result res) {
    char const *str = NULL;

    switch (res) {
    case HLL_READ_OK:
        str = "ok";
        break;
    case HLL_READ_EOF:
        str = "eof";
        break;
    case HLL_READ_LEX_FAILED:
        str = "lex failed";
        break;
    case HLL_READ_UNEXPECTED_TOKEN:
        str = "unexpected token";
        break;
    case HLL_READ_MISSING_RPAREN:
        str = "missing rparen";
        break;
    case HLL_READ_STRAY_DOT:
        str = "stray dot";
        break;
    }

    return str;
}

hll_reader
hll_reader_create(hll_lexer *lexer, hll_ctx *ctx) {
    hll_reader reader = { 0 };

    reader.lexer = lexer;
    reader.ctx = ctx;

    return reader;
}

static hll_read_result
read_cons(hll_reader *reader, hll_obj **head) {
    hll_read_result result = HLL_READ_OK;

    // Pull the pointers
    hll_lexer *lexer = reader->lexer;
    hll_ctx *ctx = reader->ctx;

    // Test initial conditions
    // We expect cons to begin with opening paren
    hll_lex_result lex_result = hll_lexer_peek(lexer);
    assert(lex_result == HLL_LEX_OK);
    assert(lexer->token_kind == HLL_TOK_LPAREN);
    lex_result = hll_lexer_eat_peek(lexer);
    // Now we epxect at least one list element followed by others ending either
    // with right paren or dot, element and right paren Now check if we don't
    // have first element
    if (lex_result != HLL_LEX_OK) {
        return report_error(
            HLL_READ_LEX_FAILED, reader,
            "Lexing failed when parsing first element of list: %s",
            hll_lex_result_str(lex_result));
    } else if (lexer->token_kind == HLL_TOK_DOT) {
        return report_error(HLL_READ_STRAY_DOT, reader,
                            "Stray dot as first element of list");
    } else if (lexer->token_kind == HLL_TOK_EOF) {
        return report_error(HLL_READ_MISSING_RPAREN, reader,
                            "Missing closing paren (eof ecnountered)");
    } else if (lexer->token_kind == HLL_TOK_RPAREN) {
        // If we encounter right paren right away, do early return and skip
        // unneded stuff.
        *head = hll_nil;
        hll_lexer_eat(lexer);
        return HLL_READ_OK;
    }

    hll_obj *list_head;
    hll_obj *list_tail;
    // Read the first element
    {
        hll_obj *car = NULL;
        if ((result = hll_read(reader, &car)) != HLL_READ_OK) {
            return result;
        }

        list_head = list_tail = hll_make_cons(ctx, car, hll_nil);
    }

    // Now enter the loop of parsing other list elements.
    for (;;) {
        lex_result = hll_lexer_peek(lexer);
        if (lex_result != HLL_LEX_OK) {
            return report_error(
                HLL_READ_LEX_FAILED, reader,
                "Lexing failed when parsing element of list: %s",
                hll_lex_result_str(lex_result));
        } else if (lexer->token_kind == HLL_TOK_EOF) {
            return report_error(HLL_READ_MISSING_RPAREN, reader,
                                "Missing closing paren (eof ecnountered)");
        } else if (lexer->token_kind == HLL_TOK_RPAREN) {
            // Rparen means that we should exit.
            hll_lexer_eat(lexer);
            *head = list_head;
            return HLL_READ_OK;
        } else if (lexer->token_kind == HLL_TOK_DOT) {
            // After dot there should be one more list element and closing paren
            hll_lexer_eat(lexer);
            if ((result = hll_read(reader, &hll_unwrap_cons(list_tail)->cdr)) !=
                HLL_READ_OK) {
                return result;
            }

            // Now peek what we expect to be right paren
            lex_result = hll_lexer_peek(lexer);
            if (lex_result != HLL_LEX_OK) {
                return report_error(
                    HLL_READ_LEX_FAILED, reader,
                    "Lexing failed when parsing element after dot of list: %s",
                    hll_lex_result_str(lex_result));
            }

            if (lexer->token_kind != HLL_TOK_RPAREN) {
                return report_error(HLL_READ_MISSING_RPAREN, reader,
                                    "Missing closing paren after dot");
            }
            hll_lexer_eat(lexer);
            *head = list_head;
            return HLL_READ_OK;
        }

        // If token is non terminating, add one more element to list
        hll_obj *obj = NULL;
        if ((result = hll_read(reader, &obj)) != HLL_READ_OK) {
            return result;
        }

        hll_unwrap_cons(list_tail)->cdr = hll_make_cons(ctx, obj, hll_nil);
        list_tail = hll_unwrap_cons(list_tail)->cdr;
    }

    assert(!"Unreachable");
}

static hll_read_result
read_quote(hll_reader *reader, hll_obj **head) {
    hll_obj *symb = hll_find_symb(reader->ctx, HLL_BUILTIN_QUOTE_SYMB_NAME,
                                  HLL_BUILTIN_QUOTE_SYMB_LEN);
    hll_lexer_eat(reader->lexer);

    hll_obj *car = NULL;
    hll_read_result result = hll_read(reader, &car);
    if (result == HLL_READ_OK) {
        assert(car != NULL);
        *head = hll_make_cons(reader->ctx, symb,
                              hll_make_cons(reader->ctx, car, hll_nil));
    }

    return result;
}

hll_read_result
hll_read(hll_reader *reader, hll_obj **head) {
    hll_read_result result = HLL_READ_OK;

    hll_lexer *lexer = reader->lexer;
    hll_lex_result lex_result = hll_lexer_peek(lexer);
    if (lex_result != HLL_LEX_OK) {
        return report_error(HLL_READ_LEX_FAILED, reader,
                            "Lexing failed when reading object: %s",
                            hll_lex_result_str(lex_result));
    } else {
        switch (lexer->token_kind) {
        default:
            result = report_error(HLL_READ_UNEXPECTED_TOKEN, reader,
                                  "Unexpected token when reading object");
            break;
        case HLL_TOK_EOF:
            result = HLL_READ_EOF;
            break;
        case HLL_TOK_NUMI:
            *head = hll_make_int(reader->ctx, lexer->token_int);
            hll_lexer_eat(lexer);
            break;
        case HLL_TOK_SYMB:
            *head =
                hll_find_symb(reader->ctx, lexer->buffer, lexer->token_length);
            hll_lexer_eat(lexer);
            break;
        case HLL_TOK_LPAREN:
            result = read_cons(reader, head);
            break;
        case HLL_TOK_QUOTE:
            result = read_quote(reader, head);
            break;
        }
    }

    return result;
}
