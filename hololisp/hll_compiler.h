//
// hll_compiler.h
//
// This file contains lexer, parser and compiler definitions for use with
// hololisp.
//
// Lexer accepts c string of input and produces sequence of tokens.
// Lexer uses FSM internally and is able to generate tokens on demand
// one-by-one.
//
// Parser uses naive recursive-descent algorithm and produces tree of lisp
// objects, like lists.
//
// Compiler takes tree of tokens that can either be result of parsing or come
// from some other source, like be result of compilation.
// Compiler produces lisp function object that contains compiled lisp code
// generated from input.
//
// Despite lexer and parser being embedded and used only in intermediate
// step before compiling, they are made public for more usability.
//
#ifndef HLL_COMPILER_H
#define HLL_COMPILER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hll_hololisp.h"

#define HLL_LOCATION_TABLE_SIZE 8192

// Describes entry of location table.
struct hll_location_entry {
  struct hll_location_entry *next;
  uint64_t hash;
  // We store location information as offsets instead of line + column to speed
  // up lexing. This differs from textbook approach.
  size_t offset;
  uint32_t length;
};

// Table of locations used during compilation. Because of the nature of lisp,
// we want to allow using values from language to be used directly as AST,
// which makes embedding location info into values undesirable because
// of memory waste in most use cases.
// Instead, during compilation we create table of location infos,
// where each create object is mapped to its location using hash table
// which uses object pointers as hash values.
// Table is deleted just after the compilation finishes,
// using information from it to build mapping for bytecode instructions to
// source locations.
struct hll_location_table {
  struct hll_location_entry hash_table[HLL_LOCATION_TABLE_SIZE];
  uint32_t entry_count;
};

void hll_add_location_info(struct hll_location_table *table, uint64_t hash,
                           size_t offset, uint32_t length);

// Kinds of tokens recognized by lexer.
enum hll_token_kind {
  HLL_TOK_EOF,
  HLL_TOK_NUM,
  HLL_TOK_SYMB,
  HLL_TOK_DOT,
  HLL_TOK_LPAREN,
  HLL_TOK_RPAREN,
  HLL_TOK_QUOTE, // '
  HLL_TOK_COMMENT,
  HLL_TOK_UNEXPECTED
};

// Token generated as result of lexing.
struct hll_token {
  enum hll_token_kind kind;
  // offset from start of file. Can be used to generate line information
  size_t offset;
  // length of token in bytes
  uint32_t length;
  // Value of token if it is a number
  double value;
};

// Lexer is designed in way it is possible to use outside of compiler to allow
// asy writing of tools like syntax highlighter and code formatter.
// Thus is does not act as a individual step of translation but as
// helper for reader.
struct hll_lexer {
  // Used for error reporting. If NULL, no errors are reported
  struct hll_vm *vm;
  // Mark that errors have been encountered during lexing.
  uint32_t error_count;
  // Current parsing location
  const char *cursor;
  // Input start. Used to calculate each token's offset
  const char *input;
  // Next peeked token. Stores result of lexing
  struct hll_token next;
};

HLL_PUB void hll_lexer_init(struct hll_lexer *lexer, const char *input,
                            struct hll_vm *vm) __attribute__((nonnull(1, 2)));

// Generate next token and return it. If EOF is reached, EOF token is
// guaranteed to always be returned after that.
HLL_PUB const struct hll_token *hll_lexer_next(struct hll_lexer *lexer)
    __attribute__((nonnull));

// Meta information used in reader.
// It is named 'meta' because is not directly related to AST generation.
// This is used to build debug information that is later embedded into bytecode.
struct hll_compilation_unit {
  // Index of compilation unit in global table.
  uint32_t compilatuon_unit;
  // Pointer to location table. Location table is populated during parsing,
  // and information stored in it is later used when building bytecode.
  struct hll_location_table locs;
};

// Reader that generates AST using tokens generated by lexer. This structure
// is also commonly known as parser.
struct hll_reader {
  // Needed for error reporting and object allocations.
  struct hll_vm *vm;
  // Mark that errors have been encountered during parsing.
  uint32_t error_count;
  // Lexer used for reading.
  // Reader process all tokens produced by lexer until EOF, so lifetime of
  // this lexer is associated with reader.
  struct hll_lexer *lexer;
  // Last token generated by lexer. This is shortcut not to access it through
  // lexer every time. This is pointer to 'next' field of lexer.
  const struct hll_token *token;
  // Internal flag used in lexer. Used for generating 'peek-eat' workflow
  // during reading.
  bool should_return_old_token;
  // Meta information used when generating AST.
  struct hll_compilation_unit *cu;
};

// Initialized reader to use given lexer. Lexer is taken as pointer to allow
// used decide where it comes from.
HLL_PUB void hll_reader_init(struct hll_reader *reader, struct hll_lexer *lexer,
                             struct hll_vm *vm, struct hll_compilation_unit *cu)
    __attribute__((nonnull(1, 2, 3)));

// Reads whole ast tree.
HLL_PUB hll_value hll_read_ast(struct hll_reader *reader)
    __attribute__((nonnull));

// Structure that holds state of compiler.
struct hll_compiler {
  struct hll_vm *vm;
  uint32_t error_count;

  hll_value env;
  struct hll_bytecode *bytecode;

  struct hll_compilation_unit *cu;
};

HLL_PUB void hll_compiler_init(struct hll_compiler *compiler, struct hll_vm *vm,
                               hll_value env, struct hll_compilation_unit *cu)
    __attribute__((nonnull(1, 2)));

// Compiles ast into a function object. Garbage collection is forbidden to
// happen during execution of this function.
HLL_PUB hll_value hll_compile_ast(struct hll_compiler *compiler, hll_value ast)
    __attribute__((nonnull));

// Compiles hololisp code as a hololisp bytecode.
// Returns true on success, false otherwise.
// writes resulting function to location pointed by 'compiled' parameter.
// If function does not success, contents of memory pointed by 'compiled'
// are not defined.
HLL_PUB bool hll_compile(struct hll_vm *vm, const char *source,
                         hll_value *compiled) __attribute__((nonnull));

#endif
