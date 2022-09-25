//
// Virtual machine definition.
// Virtual machine is what is responsible for executing any hololisp code.

#ifndef HLL_VM_H
#define HLL_VM_H

#include <stdbool.h>
#include <stdint.h>

#include "hll_hololisp.h"

typedef struct {
  const struct hll_bytecode *bytecode;
  const uint8_t *ip;
  hll_value env;
  hll_value func;
} hll_call_frame;

typedef struct hll_vm {
  struct hll_config config;
  struct hll_debug_storage *ds;
  struct hll_gc *gc;

  // We use xorshift64 for random number generation. Because hololisp is
  // single-threaded, we can use single global variable for rng state.
  uint64_t rng_state;

  // Global env. It is stored across calls to interpret, allowing defining
  // toplevel functions.
  hll_value global_env;

  // Current execution state
  hll_value *stack;
  hll_call_frame *call_stack;
  hll_value env;

} hll_vm;

HLL_PUB void hll_add_binding(hll_vm *vm, const char *symb,
                             hll_value (*bind)(hll_vm *vm, hll_value args));

HLL_PUB bool hll_interpret_bytecode(hll_vm *vm, hll_value compiled,
                                    bool print_result);

HLL_PUB void hll_add_variable(hll_vm *vm, hll_value env, hll_value name,
                              hll_value value);

HLL_PUB hll_value hll_expand_macro(hll_vm *vm, hll_value macro, hll_value args);

HLL_PUB void hll_print_value(hll_vm *vm, hll_value obj);

HLL_PUB bool hll_find_var(hll_vm *vm, hll_value env, hll_value car,
                          hll_value *found);

HLL_PUB void hll_runtime_error(hll_vm *vm, const char *fmt, ...);

hll_value hll_interpret_bytecode_internal(hll_vm *vm, hll_value env_,
                                          hll_value compiled);

void hll_print(hll_vm *vm, const char *str);

#endif
