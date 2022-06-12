#ifndef __READER_H__
#define __READER_H__

#include <stdint.h>

struct hll_lexer;
struct hll_ctx;
struct hll_obj;

typedef struct hll_reader {
    struct hll_lexer *lexer;
    struct hll_ctx *ctx;
} hll_reader;

typedef enum {
    HLL_READ_OK = 0x0,
    HLL_READ_EOF = 0x1,
    HLL_READ_LEX_FAILED = 0x2,
    HLL_READ_UNEXPECTED_TOKEN = 0x3,
    HLL_READ_MISSING_RPAREN = 0x4,
    HLL_READ_STRAY_DOT = 0x5
} hll_read_result;

char const *hll_read_result_str(hll_read_result res);
hll_reader hll_reader_create(struct hll_lexer *lexer, struct hll_ctx *ctx);

hll_read_result hll_read(hll_reader *reader, struct hll_obj **head)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__warn_unused_result__))
#endif
    ;

#endif
