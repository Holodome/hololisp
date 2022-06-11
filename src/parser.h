#ifndef __PARSER_H__
#define __PARSER_H__

#include <stdint.h>

struct hll_lexer;
struct hll_ctx;
struct hll_obj;

typedef struct hll_parser {
    struct hll_lexer *lexer;
    struct hll_ctx *ctx;
} hll_parser;

typedef enum {
    HLL_PARSE_OK = 0x0,
    HLL_PARSE_EOF = 0x1,
    HLL_PARSE_LEX_FAILED = 0x2,
    HLL_PARSE_UNEXPECTED_TOKEN = 0x3,
    HLL_PARSE_MISSING_RPAREN = 0x4,
    HLL_PARSE_STRAY_DOT = 0x5
} hll_parse_result;

char const *hll_parse_result_str(hll_parse_result res);

hll_parser hll_parser_create(struct hll_lexer *lexer, struct hll_ctx *ctx);

hll_parse_result hll_parse(hll_parser *parser, struct hll_obj **head)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__warn_unused_result__))
#endif
    ;

#endif
