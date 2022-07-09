#ifndef __HLL_COMPILER_H__
#define __HLL_COMPILER_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "hll_hololisp.h"
#include "hll_mem.h"

/// Kinds of tokens recognized by lexer.
typedef enum {
    /// End of file
    HLL_TOK_EOF,
    /// Integer
    HLL_TOK_INT,
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

/// Coupled token definition.
typedef struct {
    hll_token_kind kind;
    uint32_t line;
    uint32_t column;
    uint32_t length;
    int64_t value;
    char const *start;
} hll_token;

typedef enum {
    HLL_LEX_START,
    HLL_LEX_NUMBER,
    HLL_LEX_DOTS,
    HLL_LEX_SYMB,
    HLL_LEX_UNEXPECTED,

    HLL_LEX_FIN,
    HLL_LEX_FIN_LPAREN,
    HLL_LEX_FIN_RPAREN,
    HLL_LEX_FIN_QUOTE,
    HLL_LEX_FIN_NUMBER,
    HLL_LEX_FIN_DOTS,
    HLL_LEX_FIN_SYMB,
    HLL_LEX_FIN_EOF,
    HLL_LEX_FIN_UNEXPECTED,
} hll_lexer_state;

/// Lexer is designed in way it is possible to use outside of compiler to allow
/// asy writing of tools like syntax highlighter and code formatter.
/// Thus is does not act as a individual step of translation but as
/// helper for reader.
typedef struct {
    /// If NULL, errors are not reported.
    hll_error_fn *error_fn;
    /// Mark that errors have been encountered during lexing.
    bool has_errors;
    /// Buffer where strings should be written.
    char *buffer;
    size_t buffer_size;

    char const *cursor;
    uint32_t line;
    char const *line_start;

    hll_token next;
    bool should_return_old;
} hll_lexer;

void hll_lexer_peek(hll_lexer *lexer);
void hll_lexer_eat(hll_lexer *lexer);
void hll_lexer_eat_peek(hll_lexer *lexer);

typedef enum {
    HLL_AST_NONE,
    HLL_AST_NIL,
    HLL_AST_TRUE,
    HLL_AST_INT,
    HLL_AST_CONS,
    HLL_AST_SYMB,
} hll_ast_kind;

typedef struct hll_ast {
    hll_ast_kind kind;
    union {
        int64_t num;
        char const *symb;
        struct {
            struct hll_ast *car;
            struct hll_ast *cdr;
        } cons;
    } as;
} hll_ast;

typedef struct {
    /// If NULL, errors are not reported.
    hll_error_fn *error_fn;
    /// Mark that errors have been encountered during parsing.
    bool has_errors;
    /// Lexer used for reading.
    /// Reader process all tokens produced by lexer until EOF, so lifetime of
    /// this lexer is associated with reader.
    hll_lexer *lexer;
    /// This is the arena that is used for allocating AST nodes during parsing.
    /// After work with AST is finished, they can all be freed at once.
    struct hll_memory_arena *arena;
} hll_reader;

hll_ast *hll_read_ast(hll_reader *reader);

typedef struct {
    void *a;
} hll_compiler;

void *hll_compile_ast(hll_compiler *compiler, hll_ast *ast);

/// Compiles hololisp code as a hololisp bytecode.
/// Because internally lisp is represented as a tree of conses (lists),
/// we first transform code into lisp AST and then by traversing it compile it.
/// Though it is possible to compile simple code using stack-based traversal of tree instead of
/// compiling it to AST it the first place, but lisp is different from other simple
/// languages that it has macro system. Macros operate on AST, thus we have to go through the AST step.
void *hll_compile(struct hll_vm *vm, char const *source);

#endif
