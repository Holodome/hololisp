#include "parser.h"

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

hll_parser
hll_parser_create(hll_lexer *lexer, hll_lisp_ctx *ctx) {
    hll_parser parser = { 0 };

    parser.lexer = lexer;
    parser.ctx = ctx;

    return parser;
}

static hll_parse_result
read_cons(hll_parser *parser, hll_lisp_obj_head **head) {
    hll_parse_result result = HLL_PARSE_OK;

    // Pull the pointers

    hll_lexer *lexer = parser->lexer;
    hll_lisp_ctx *ctx = parser->ctx;

    // Test initial conditions

    hll_lex_result lex_result = hll_lexer_peek(lexer);
    assert(lex_result == HLL_LEX_OK);
    assert(lexer->token_kind == HLL_LTOK_LPAREN);
    lex_result = hll_lexer_eat_peek(lexer);

    if (lex_result != HLL_LEX_OK) {
        return HLL_PARSE_LEX_FAILED;
    } else if (lexer->token_kind == HLL_LTOK_DOT) {
        return HLL_PARSE_STRAY_DOT;
    } else if (lexer->token_kind == HLL_LTOK_RPAREN) {
        *head = hll_nil;
        hll_lexer_eat(lexer);
        return HLL_PARSE_OK;
    }

    if ((lex_result = hll_lexer_peek(lexer)) != HLL_LEX_OK) {
        return HLL_PARSE_LEX_FAILED;
    } else if (lexer->token_kind == HLL_LTOK_EOF) {
        return HLL_PARSE_MISSING_RPAREN;
    }

    hll_lisp_obj_head *car = NULL;
    if ((result = hll_parse(parser, &car)) != HLL_PARSE_OK) {
        return result;
    }

    hll_lisp_obj_head *list_head;
    hll_lisp_obj_head *list_tail;
    list_head = list_tail = hll_make_cons(ctx, car, hll_nil);

    for (;;) {
        lex_result = hll_lexer_peek(lexer);
        if (lex_result != HLL_LEX_OK) {
            return HLL_PARSE_LEX_FAILED;
        } else if (lexer->token_kind == HLL_LTOK_RPAREN) {
            hll_lexer_eat(lexer);
            *head = list_head;
            break;
        } else if (lexer->token_kind == HLL_LTOK_DOT) {
            hll_lexer_eat(lexer);
            if ((result =
                     hll_parse(parser, &hll_unwrap_cons(list_tail)->cdr)) !=
                HLL_PARSE_OK) {
                return result;
            }

            lex_result = hll_lexer_peek(lexer);
            if (lex_result != HLL_LEX_OK) {
                return HLL_PARSE_LEX_FAILED;
            }

            if (lexer->token_kind != HLL_LTOK_RPAREN) {
                return HLL_PARSE_MISSING_RPAREN;
            }
            hll_lexer_eat(lexer);
            *head = list_head;
            break;
        }

        hll_lisp_obj_head *obj = NULL;
        if ((result = hll_parse(parser, &obj)) != HLL_PARSE_OK) {
            return result;
        }

        assert(obj != NULL);
        hll_unwrap_cons(list_tail)->cdr = hll_make_cons(ctx, obj, hll_nil);
        list_tail = hll_unwrap_cons(list_tail)->cdr;
    }

    return HLL_PARSE_OK;
}

static hll_parse_result
read_quote(hll_parser *parser, hll_lisp_obj_head **head) {
    hll_lisp_obj_head *symb = hll_find_symb(
        parser->ctx, HLL_BUILTIN_QUOTE_SYMB_NAME, HLL_BUILTIN_QUOTE_SYMB_LEN);
    hll_lexer_eat(parser->lexer);

    hll_lisp_obj_head *car = NULL;
    hll_parse_result result = hll_parse(parser, &car);
    if (result == HLL_PARSE_OK) {
        assert(car != NULL);
        *head = hll_make_cons(parser->ctx, symb,
                              hll_make_cons(parser->ctx, car, hll_nil));
    }

    return result;
}

hll_parse_result
hll_parse(hll_parser *parser, hll_lisp_obj_head **head) {
    hll_parse_result result = HLL_PARSE_OK;

    hll_lexer *lexer = parser->lexer;
    hll_lex_result lex_result = hll_lexer_peek(lexer);
    if (lex_result != HLL_LEX_OK) {
        result = HLL_PARSE_LEX_FAILED;
    } else {
        switch (lexer->token_kind) {
        default:
            result = HLL_PARSE_UNEXPECTED_TOKEN;
            break;
        case HLL_LTOK_EOF:
            result = HLL_PARSE_EOF;
            break;
        case HLL_LTOK_NUMI:
            *head = hll_make_int(parser->ctx, lexer->token_int);
            hll_lexer_eat(lexer);
            break;
        case HLL_LTOK_SYMB:
            *head =
                hll_find_symb(parser->ctx, lexer->buffer, lexer->token_length);
            hll_lexer_eat(lexer);
            break;
        case HLL_LTOK_LPAREN:
            result = read_cons(parser, head);
            break;
        case HLL_LTOK_QUOTE:
            result = read_quote(parser, head);
            break;
        }
    }

    return result;
}
