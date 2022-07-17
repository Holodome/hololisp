#ifndef HLL_BYTECODE_H
#define HLL_BYTECODE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    /// Bytecode must be terminated with 0.
    HLL_BYTECODE_END = 0x0,
    // Pushes nil on stack
    HLL_BYTECODE_NIL,
    // Pushes true on stack
    HLL_BYTECODE_TRUE,
    // Pushes constant on stack (u16 index)
    HLL_BYTECODE_CONST,
    // Pushed symbol on stack (u16 index)
    HLL_BYTECODE_SYMB,
    // Uses 3 last items on stack. First two are considered list head and tail,
    // 3 is element that needs to be appended to list. Pops last element
    // leaving only head and tail.
    HLL_BYTECODE_APPEND,
    // Removes top element from stack
    HLL_BYTECODE_POP,
    // Does lookup of given symbol across all envs in lexical order.
    // Pushes on stack cons of variable storage (pair name-value).
    // Returning cons allows changing value in-place.
    HLL_BYTECODE_FIND,
    // Does context-sensitive call. Uses two elements on stack.
    // First is callable object. This is lisp object either a function (lambda)
    // or C binding. In first case creates new env and calls that function.
    HLL_BYTECODE_CALL,
    // Jump if true (u16 offset).
    HLL_BYTECODE_JN,
    // Create function. Uses two values on stack. First is arguments list,
    // second is function body. Appends new function on stack.
    HLL_BYTECODE_MAKE_LAMBDA,
    // Defines new variable with given name in current env (lexical env).
    // If variable with same name is defined in current env, error.
    // If variable with same name is defined in outer scope it is redefined in
    // current.
    HLL_BYTECODE_LET,
    // Creates new variable scope context
    HLL_BYTECODE_PUSHENV,
    // Removes last variable scope context
    HLL_BYTECODE_POPENV,
    HLL_BYTECODE_CAR,
    HLL_BYTECODE_CDR,
    HLL_BYTECODE_SETCAR,
    HLL_BYTECODE_SETCDR,
} hll_bytecode_op;

typedef struct hll_bytecode {
    uint8_t *ops;
    int64_t *constant_pool;
    char const **symbol_pool;
} hll_bytecode;

void hll_dump_bytecode(void *file, hll_bytecode *bytecode);

#endif
