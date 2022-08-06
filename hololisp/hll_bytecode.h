#ifndef HLL_BYTECODE_H
#define HLL_BYTECODE_H

#include <stddef.h>
#include <stdint.h>

/*
NIL
    s[n++] = nil
TRUE
    s[n++] = true
CONST(u16 x)
    s[n++] = consts[x]
APPEND
    a := cons(s[n - 1], nil);
    if list_head == NULL:
        s[n - 3] = s[n - 2] = a
    else:
        s[n - 2].cdr = cons
        s[n - 2] = a
    pop()
POP
    --n
FIND
    symb := s[--n]
    s[n++] = search(env, symb)
CALL
    args := s[--n]
    callable := s[--n]
    callable(args)
JN(u16 x)
    if s[n - 1] == nil:
        ip += x
MAKELAMBDA
    body := s[--n]
    params := s[--n]
    s[n++] = lambda(cur_env, body, params)
LET
    value := s[--n]
    name := s[--n]
    cur_env[name] = value
PUSHENV
    cur_env = env()
POPENV
    cur_env = cur_env.up
CAR
    list := s[--n]
    s[n++] = list.car
CDR
    list := s[--n]
    s[n++] = list.cdr
SETCAR
    list := s[--n]
    value := s[--n]
    list.car = value
SETCDR
    list := s[--n]
    value := s[--n]
    list.cdr = value
 */

typedef enum {
  /// Bytecode must be terminated with 0.
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
  HLL_BYTECODE_LOADFN,
} hll_bytecode_op;

typedef struct hll_bytecode {
  // Bytecode dynamic array
  uint8_t *ops;
  // Constant pool dynamic array
  struct hll_obj **constant_pool;
} hll_bytecode;

void hll_free_bytecode(hll_bytecode *bytecode);
void hll_dump_bytecode(void *file, hll_bytecode *bytecode);

#endif
