//
// Virtual machine definition.
// Virtual machine is what is responsible for executing any hololisp code.

#ifndef HLL_VM_H
#define HLL_VM_H

#include <stdbool.h>
#include <stdint.h>

#include "hll_hololisp.h"

struct hll_bytecode;

typedef struct hll_vm {
  struct hll_config config;

  // Stores name of current file being executed. Files are executed atomically,
  // so it can be reused across different compile stages to provide source
  // information.
  const char *current_filename;
  // Current parsed source.
  const char *source;

  // true object
  struct hll_obj *true_;
  // nil object
  struct hll_obj *nil;
  // Global env. It is stored across calls to interpret, allowing defining
  // toplevel functions.
  struct hll_obj *global_env;

  uint32_t error_count;
  // We use xorshift64 for random number generation. Because hololisp is
  // single-threaded, we can use single global variable for rng state.
  uint64_t rng_state;
} hll_vm;

// Used to report error in current state contained by vm.
// vm must have current_filename field present if message needs to include
// source location.
// offset specifies byte offset of reported location in file.
// len specifies length of reported part (e.g. token).
void hll_report_error(hll_vm *vm, size_t offset, uint32_t len, const char *msg);

void hll_add_binding(hll_vm *vm, const char *symb,
                     struct hll_obj *(*bind)(hll_vm *vm, struct hll_obj *args));

bool hll_interpret_bytecode(hll_vm *vm, const struct hll_bytecode *bytecode,
                            bool print_result);

struct hll_obj *hll_expand_macro(hll_vm *vm, const struct hll_obj *macro,
                                 struct hll_obj *args);

void hll_print(hll_vm *vm, struct hll_obj *obj, void *file);

void hll_runtime_error(hll_vm *vm, const char *fmt, ...);

#endif
