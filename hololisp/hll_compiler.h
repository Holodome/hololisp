#ifndef HLL_COMPILER_H
#define HLL_COMPILER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hll_bytecode.h"
#include "hll_hololisp.h"
#include "hll_mem.h"

// Kinds of tokens recognized by lexer.
typedef enum {
  // End of file
  HLL_TOK_EOF,
  // Integer
  HLL_TOK_INT,
  // Symbol
  HLL_TOK_SYMB,
  // .
  HLL_TOK_DOT,
  // (
  HLL_TOK_LPAREN,
  // )
  HLL_TOK_RPAREN,
  // '
  HLL_TOK_QUOTE,
  // Comment
  HLL_TOK_COMMENT,
  // Unexpected sequence of tokens.
  HLL_TOK_UNEXPECTED
} hll_token_kind;

// Coupled token definition.
typedef struct {
  hll_token_kind kind;
  size_t offset;
  uint32_t length;
  double value;
} hll_token;

// Lexer is designed in way it is possible to use outside of compiler to allow
// asy writing of tools like syntax highlighter and code formatter.
// Thus is does not act as a individual step of translation but as
// helper for reader.
typedef struct {
  // Used for error reporting. If NULL, no errors are reported
  struct hll_vm *vm;
  // Mark that errors have been encountered during lexing.
  bool has_errors;
  // Current parsing location
  const char *cursor;
  // Input start. Used to calculate each token's offset
  const char *input;
  // Next peeked token. Stores results of lexing
  hll_token next;
} hll_lexer;

// Initializes lexer to given buffer.
void hll_lexer_init(hll_lexer *lexer, const char *input, struct hll_vm *vm);
void hll_lexer_next(hll_lexer *lexer);

typedef struct {
  // Needed for error reporting.
  struct hll_vm *vm;
  // Mark that errors have been encountered during parsing.
  bool has_errors;
  // Lexer used for reading.
  // Reader process all tokens produced by lexer until EOF, so lifetime of
  // this lexer is associated with reader.
  hll_lexer *lexer;

  bool should_return_old_token;
} hll_reader;

void hll_reader_init(hll_reader *reader, hll_lexer *lexer, struct hll_vm *vm);
struct hll_obj *hll_read_ast(hll_reader *reader);

typedef struct {
  struct hll_vm *vm;
  bool has_errors;
  struct hll_bytecode *bytecode;
} hll_compiler;

void hll_compiler_init(hll_compiler *compiler, struct hll_vm *vm,
                       struct hll_bytecode *bytecode);
void hll_compile_ast(hll_compiler *compiler, const struct hll_obj *ast);

// Compiles hololisp code as a hololisp bytecode.
// Because internally lisp is represented as a tree of conses (lists),
// we first transform code into lisp AST and then by traversing it compile it.
// Though it is possible to compile simple code using stack-based traversal of
// tree instead of compiling it to AST it the first place, but lisp is
// different from other simple languages that it has macro system. Macros
// operate on AST, thus we have to go through the AST step.
struct hll_bytecode *hll_compile(struct hll_vm *vm, const char *source);

#endif
