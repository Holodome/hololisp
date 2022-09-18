//
// Virtual machine definition.
// Virtual machine is what is responsible for executing any hololisp code.

#ifndef HLL_VM_H
#define HLL_VM_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "hll_hololisp.h"

struct hll_call_frame {
  const struct hll_bytecode *bytecode;
  const uint8_t *ip;
  hll_value env;
  hll_value func;
};

struct hll_vm {
  struct hll_config config;

  hll_value nthcdr_symb;
  hll_value quote_symb;

  // Global env. It is stored across calls to interpret, allowing defining
  // toplevel functions.
  hll_value global_env;

  uint32_t error_count;
  // We use xorshift64 for random number generation. Because hololisp is
  // single-threaded, we can use single global variable for rng state.
  uint64_t rng_state;

  // Current execution state
  hll_value *stack;
  struct hll_call_frame *call_stack;
  hll_value env;

  //
  // Garbage collector stuff
  //

  // Linked list of all objects.
  // Uses next field
  struct hll_obj *all_objs;
  // Count all allocated bytes to know when to trigger garbage collection.
  size_t bytes_allocated;
  // If bytes_allocated becomes greater than this value, trigger next gc.
  // May not be greater than min_heap_size specified in config.
  size_t next_gc;
  hll_value *gray_objs;
  hll_value *temp_roots;
  uint32_t forbid_gc;
};

// Garbage collector tracked allocation
#define hll_gc_free(_vm, _ptr, _size) hll_gc_realloc(_vm, _ptr, _size, 0)
#define hll_gc_alloc(_vm, _size) hll_gc_realloc(_vm, NULL, 0, _size)
HLL_PUB void *hll_gc_realloc(struct hll_vm *vm, void *ptr, size_t old_size,
                             size_t new_size) __attribute__((alloc_size(4)));

// Used to report error in current state contained by vm.
// vm must have current_filename field present if message needs to include
// source location.
// offset specifies byte offset of reported location in file.
// len specifies length of reported part (e.g. token).
HLL_PUB void hll_report_errorv(struct hll_vm *vm, size_t offset, uint32_t len,
                               const char *fmt, va_list args);
HLL_PUB void hll_report_error(struct hll_vm *vm, size_t offset, uint32_t len,
                              const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));
HLL_PUB void hll_report_error_value(struct hll_vm *vm, hll_value value,
                                    const char *msg, ...)
    __attribute__((format(printf, 3, 4)));
HLL_PUB void hll_report_error_valuev(struct hll_vm *vm, hll_value value,
                                     const char *msg, va_list args);

HLL_PUB void hll_add_binding(struct hll_vm *vm, const char *symb,
                             hll_value (*bind)(struct hll_vm *vm,
                                               hll_value args));

HLL_PUB bool hll_interpret_bytecode(struct hll_vm *vm, hll_value compiled,
                                    bool print_result);

HLL_PUB void hll_add_variable(struct hll_vm *vm, hll_value env, hll_value name,
                              hll_value value);

HLL_PUB hll_value hll_expand_macro(struct hll_vm *vm, hll_value macro,
                                   hll_value args);

HLL_PUB void hll_print_value(struct hll_vm *vm, hll_value obj);

HLL_PUB bool hll_find_var(struct hll_vm *vm, hll_value env, hll_value car,
                          hll_value *found);

HLL_PUB void hll_runtime_error(struct hll_vm *vm, const char *fmt, ...);

hll_value hll_interpret_bytecode_internal(struct hll_vm *vm, hll_value env_,
                                          hll_value compiled);

void hll_print(struct hll_vm *vm, const char *str);

#endif
