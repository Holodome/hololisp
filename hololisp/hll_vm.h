//
// Virtual machine definition.
// Virtual machine is what is responsible for executing any hololisp code.

#ifndef HLL_VM_H
#define HLL_VM_H

#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>

#include "hll_hololisp.h"

typedef struct hll_call_frame {
  const struct hll_bytecode *bytecode;
  const uint8_t *ip;
  hll_value env;
  hll_value func;
} hll_call_frame;

#define HLL_CALL_STACK_SIZE 1024
#define HLL_STACK_SIZE (1024 * 1024)

typedef struct hll_vm {
  struct hll_config config;
  struct hll_meta_storage *debug;
  struct hll_gc *gc;

  // We use xorshift64 for random number generation. Because hololisp is
  // single-threaded, we can use single global variable for rng state.
  uint64_t rng_state;

  // Global env. It is stored across calls to interpret, allowing defining
  // toplevel functions.
  hll_value global_env;
  hll_value macro_env;

  // Current execution state
  hll_value *stack_bottom;
  hll_value *stack;
  hll_call_frame *call_stack_bottom;
  hll_call_frame *call_stack;
  hll_value env;

  jmp_buf err_jmp;
} hll_vm;

#define hll_vm_stack_push(_vm, _value)                                         \
  do {                                                                         \
    assert(_vm->stack + 1 < _vm->stack_bottom + HLL_STACK_SIZE);               \
    *_vm->stack++ = (_value);                                                  \
  } while (0)
#define hll_vm_stack_pop(_vm)                                                  \
  ({                                                                           \
    assert(_vm->stack > _vm->stack_bottom);                                    \
    *--_vm->stack;                                                             \
  })
#define hll_vm_stack_nth(_vm, _nth)                                            \
  ({                                                                           \
    assert(_vm->stack_bottom + (_nth) <= _vm->stack);                          \
    _vm->stack[-(_nth)];                                                       \
  })
#define hll_vm_stack_top(_vm) hll_vm_stack_nth(_vm, 1)

#define hll_vm_call_stack_push(_vm, _value)                                    \
  do {                                                                         \
    assert(_vm->call_stack + 1 <                                               \
           _vm->call_stack_bottom + HLL_CALL_STACK_SIZE);                      \
    *_vm->call_stack++ = (_value);                                             \
  } while (0)
#define hll_vm_call_stack_pop(_vm)                                             \
  ({                                                                           \
    assert(_vm->call_stack > _vm->call_stack_bottom);                          \
    *--_vm->call_stack;                                                        \
  })
#define hll_vm_call_stack_nth(_vm, _nth)                                       \
  ({                                                                           \
    assert(_vm->call_stack_bottom + (_nth) >= _vm->call_stack);                \
    _vm->call_stack[-(_nth)];                                                  \
  })
#define hll_vm_call_stack_top(_vm) hll_vm_call_stack_nth(_vm, 1)

HLL_PUB void hll_add_binding(hll_vm *vm, const char *symb,
                             hll_value (*bind)(hll_vm *vm, hll_value args));

HLL_PUB hll_interpret_result hll_interpret_bytecode(hll_vm *vm,
                                                    hll_value compiled,
                                                    bool print_result);

HLL_PUB void hll_add_variable(hll_vm *vm, hll_value env, hll_value name,
                              hll_value value);

typedef enum {
  HLL_EXPAND_MACRO_OK,
  HLL_EXPAND_MACRO_ERR_ARGS
} hll_expand_macro_result;

HLL_PUB hll_expand_macro_result hll_expand_macro(hll_vm *vm, hll_value macro,
                                                 hll_value args,
                                                 hll_value *dst);

HLL_PUB void hll_print_value(hll_vm *vm, hll_value obj);

HLL_PUB bool hll_find_var(hll_value env, hll_value car, hll_value *found);

__attribute__((format(printf, 2, 3))) HLL_PUB void
hll_runtime_error(hll_vm *vm, const char *fmt, ...);

HLL_PUB hll_value hll_interpret_bytecode_internal(hll_vm *vm, hll_value env_,
                                                  hll_value compiled);

HLL_PUB void hll_print(hll_vm *vm, const char *str);

//
// Accessor overriding functions. By lisp standards, car returns car of object
// if object is cons, nil if object is nil and panics otherwise.
//

HLL_PUB hll_value hll_car(hll_vm *vm, hll_value lis);
HLL_PUB hll_value hll_cdr(hll_vm *vm, hll_value lis);

#endif
