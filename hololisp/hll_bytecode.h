#ifndef HLL_BYTECODE_H
#define HLL_BYTECODE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  // Bytecode must be terminated with 0.
  HLL_BYTECODE_END = 0x0,
  // Pushes nil on stack
  HLL_BYTECODE_NIL,
  // Pushes true on stack
  HLL_BYTECODE_TRUE,
  // Pushes constant on stack (u16 index)
  HLL_BYTECODE_CONST,
  // Uses 3 last items on stack. First two are considered list head and tail,
  // 3 is element that needs to be appended to list. Pops last element
  // leaving only head and tail.
  // The reason for this instruction is to ease up creation of lists,
  // in favor of conses because the latter appear much rarer than lists.
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
  // Jump if true (i16 offset, two's complement).
  HLL_BYTECODE_JN,
  // Defines new variable with given name in current env (lexical env).
  // If variable with same name is defined in current env, error.
  // If variable with same name is defined in outer scope it is redefined in
  // current.
  HLL_BYTECODE_LET,
  // Creates new variable scope context. Used in let statements.
  // Form applications that feature manipulation of envs do it inline.
  HLL_BYTECODE_PUSHENV,
  // Removes last variable scope context. Used when leaving let statements
  HLL_BYTECODE_POPENV,
  // Returns car of top object on stack. If object is nil, return nil
  HLL_BYTECODE_CAR,
  // Returns cdr of top object on stack. If object is nil, return nil
  HLL_BYTECODE_CDR,
  // Sets car of 2-nd object on stack. Pops the value.
  HLL_BYTECODE_SETCAR,
  // Sets cdr of 2-nd object on stack. Pops the value.
  HLL_BYTECODE_SETCDR,
  // Creates function object using constant index (u16). Object in constant slot
  // should be compiled function object. It is copied and pushed on top of the
  // stack.
  // Then all symbols referenced in function definition are captured.
  HLL_BYTECODE_MAKEFUN,
  HLL_BYTECODE_DUP,
} hll_bytecode_op;

typedef struct hll_bytecode {
  // Bytecode dynamic array
  uint8_t *ops;
  // Constant pool dynamic array
  struct hll_obj **constant_pool;
} hll_bytecode;

void hll_free_bytecode(hll_bytecode *bytecode);
void hll_dump_bytecode(void *file, const hll_bytecode *bytecode);


void hll_dump_object(void *file, struct hll_obj *obj);

#endif
