#include "reader.h"

#include <assert.h>

#include "lexer.h"
#include "lisp.h"

char const *
hll_parse_result_str(hll_parse_result res) {
    char const *str = NULL;

    switch (res) {
    case HLL_PARSE_OK:
        str = "ok";
        break;
    case HLL_PARSE_EOF:
        str = "eof";
        break;
    case HLL_PARSE_LEX_FAILED:
        str = "lex failed";
        break;
    case HLL_PARSE_UNEXPECTED_TOKEN:
        str = "unexpected token";
        break;
    case HLL_PARSE_MISSING_RPAREN:
        str = "missing rparen";
        break;
    case HLL_PARSE_STRAY_DOT:
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

static hll_parse_result
read_cons(hll_reader *reader, hll_obj **head) {
    hll_parse_result result = HLL_PARSE_OK;

    // Pull the pointers

    hll_lexer *lexer = reader->lexer;
    hll_ctx *ctx = reader->ctx;

    // Test initial conditions

    hll_lex_result lex_result = hll_lexer_peek(lexer);
    assert(lex_result == HLL_LEX_OK);
    assert(lexer->token_kind == HLL_TOK_LPAREN);
    lex_result = hll_lexer_eat_peek(lexer);

    if (lex_result != HLL_LEX_OK) {
        return HLL_PARSE_LEX_FAILED;
    } else if (lexer->token_kind == HLL_TOK_DOT) {
        return HLL_PARSE_STRAY_DOT;
    } else if (lexer->token_kind == HLL_TOK_RPAREN) {
        *head = hll_nil;
        hll_lexer_eat(lexer);
        return HLL_PARSE_OK;
    }

    if ((lex_result = hll_lexer_peek(lexer)) != HLL_LEX_OK) {
        return HLL_PARSE_LEX_FAILED;
    } else if (lexer->token_kind == HLL_TOK_EOF) {
        return HLL_PARSE_MISSING_RPAREN;
    }

    hll_obj *car = NULL;
    if ((result = hll_parse(reader, &car)) != HLL_PARSE_OK) {
        return result;
    }

    hll_obj *list_head;
    hll_obj *list_tail;
    list_head = list_tail = hll_make_cons(ctx, car, hll_nil);

    for (;;) {
        lex_result = hll_lexer_peek(lexer);
        if (lex_result != HLL_LEX_OK) {
            return HLL_PARSE_LEX_FAILED;
        } else if (lexer->token_kind == HLL_TOK_RPAREN) {
            hll_lexer_eat(lexer);
            *head = list_head;
            break;
        } else if (lexer->token_kind == HLL_TOK_DOT) {
            hll_lexer_eat(lexer);
            if ((result =
                     hll_parse(reader, &hll_unwrap_cons(list_tail)->cdr)) !=
                HLL_PARSE_OK) {
                return result;
            }

            lex_result = hll_lexer_peek(lexer);
            if (lex_result != HLL_LEX_OK) {
                return HLL_PARSE_LEX_FAILED;
            }

            if (lexer->token_kind != HLL_TOK_RPAREN) {
                return HLL_PARSE_MISSING_RPAREN;
            }
            hll_lexer_eat(lexer);
            *head = list_head;
            break;
        }

        hll_obj *obj = NULL;
        if ((result = hll_parse(reader, &obj)) != HLL_PARSE_OK) {
            return result;
        }

        assert(obj != NULL);
        hll_unwrap_cons(list_tail)->cdr = hll_make_cons(ctx, obj, hll_nil);
        list_tail = hll_unwrap_cons(list_tail)->cdr;
    }

    return HLL_PARSE_OK;
}

static hll_parse_result
read_quote(hll_reader *reader, hll_obj **head) {
    hll_obj *symb = hll_find_symb(
        reader->ctx, HLL_BUILTIN_QUOTE_SYMB_NAME, HLL_BUILTIN_QUOTE_SYMB_LEN);
    hll_lexer_eat(reader->lexer);

    hll_obj *car = NULL;
    hll_parse_result result = hll_parse(reader, &car);
    if (result == HLL_PARSE_OK) {
        assert(car != NULL);
        *head = hll_make_cons(reader->ctx, symb,
                              hll_make_cons(reader->ctx, car, hll_nil));
    }

    return result;
}

hll_parse_result
hll_parse(hll_reader *reader, hll_obj **head) {
    hll_parse_result result = HLL_PARSE_OK;

    hll_lexer *lexer = reader->lexer;
    hll_lex_result lex_result = hll_lexer_peek(lexer);
    if (lex_result != HLL_LEX_OK) {
        result = HLL_PARSE_LEX_FAILED;
    } else {
        switch (lexer->token_kind) {
        default:
            result = HLL_PARSE_UNEXPECTED_TOKEN;
            break;
        case HLL_TOK_EOF:
            result = HLL_PARSE_EOF;
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
