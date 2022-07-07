#ifndef __HLL_COMPILER_H__
#define __HLL_COMPILER_H__

#include <stdint.h>
#include <stddef.h>

struct hll_vm;

typedef enum {
    /// End of file
    HLL_TOK_EOF,
    /// Integer
    HLL_TOK_NUMI,
    /// Symbol
    HLL_TOK_SYMB,
    /// .
    HLL_TOK_DOT,
    /// (
    HLL_TOK_LPAREN,
    /// )
    HLL_TOK_RPAREN,
    /// '
    HLL_TOK_QUOTE,
    /// Error
    HLl_TOK_ERROR
} hll_token_kind ;

typedef struct {
    hll_token_kind kind;
    uint32_t line;
    uint32_t column;
    uint32_t length;
    int64_t value;
} hll_token;

typedef struct {

    bool has_errors;

    char *buffer;
    size_t buffer_size;

    char const *cursor;
    uint32_t line;
    char const *token_start;
    char const *line_start;

    hll_token next;
} hll_lexer;

void hll_lexer_peek(hll_lexer *lexer);
void hll_lexer_eat(hll_lexer *lexer);
void hll_lexer_eat_peek(hll_lexer *lexer);

typedef struct {
    struct hll_vm *vm;

    hll_lexer *lexer;
} hll_reader;

typedef struct {

} hll_compiler;

void *hll_compile(struct hll_vm *vm, char const *source);

#endif