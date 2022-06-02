#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>

struct hll_lexer;
struct hll_lisp_ctx;
struct hll_lisp_obj_head;

typedef struct hll_parser {
    struct hll_lexer    *lexer;
    struct hll_lisp_ctx *ctx;
} hll_parser;

typedef enum {
    HLL_PARSE_OK = 0x0,
} hll_parse_result;

hll_parser hll_parser_create(struct hll_lexer *lexer,
                             struct hll_lisp_ctx     *ctx);

hll_parse_result hll_parse(hll_parser *parser, struct hll_lisp_obj_head **head);

#endif
