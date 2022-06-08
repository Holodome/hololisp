#include "parser.h"

#include <assert.h>

#include "lexer.h"
#include "lisp.h"

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

    // Accumulate list

    hll_lisp_obj_head *expr = hll_nil;
    while (result == HLL_PARSE_OK && lex_result == HLL_LEX_OK &&
           lexer->token_kind != HLL_LTOK_RPAREN &&
           lexer->token_kind != HLL_LTOK_EOF &&
           lexer->token_kind != HLL_LTOK_DOT) {
        hll_lisp_obj_head *car = NULL;
        result = hll_parse(parser, &car);

        if (result == HLL_PARSE_OK) {
            assert(car != NULL);
            expr = hll_make_cons(ctx, car, expr);
            lex_result = hll_lexer_peek(lexer);
        }
    }

    // Finalize
    if (result != HLL_PARSE_OK) {
    } else if (lex_result != HLL_LEX_OK) {
        result = HLL_PARSE_LEX_FAILED;
    } else if (lexer->token_kind == HLL_LTOK_RPAREN) {
        hll_lexer_eat(lexer);
        *head = hll_reverse_list(expr);
    } else if (lexer->token_kind == HLL_LTOK_DOT &&
               (lex_result = hll_lexer_eat_peek(lexer)) == HLL_LEX_OK) {
        hll_lisp_obj_head *car = NULL;
        if ((result = hll_parse(parser, &car)) == HLL_PARSE_OK) {
            lex_result = hll_lexer_peek(lexer);
            *head = hll_reverse_list(expr);
            hll_unwrap_cons(expr)->cdr = car;
        }
    } else {
        result = HLL_PARSE_MISSING_RPAREN;
    }

    return result;
}

static hll_parse_result
read_quote(hll_parser *parser, hll_lisp_obj_head **head) {
    hll_parse_result result = HLL_PARSE_OK;

    hll_lisp_obj_head *symb = hll_find_symb(
        parser->ctx, HLL_BUILTIN_QUOTE_SYMB_NAME, HLL_BUILTIN_QUOTE_SYMB_LEN);
    hll_lexer_eat(parser->lexer);

    hll_lisp_obj_head *car = NULL;
    result = hll_parse(parser, &car);
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
