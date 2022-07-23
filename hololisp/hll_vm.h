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

  struct hll_obj *true_;
  struct hll_obj *nil;

  // Global env frame. Can be used to add c bindings whilst using unified
  // storage.
  struct hll_obj *env;
  // Env in which binding functions are executed. Practically it means that any
  // variable lookup done from such function is allowed only to succeed on
  // already-defined global variables.
  struct hll_obj *global_env;
} hll_vm;

void hll_add_binding(hll_vm *vm, char const *symb,
                     struct hll_obj *(*bind)(hll_vm *vm, struct hll_obj *args));

bool hll_interpret_bytecode(hll_vm *vm, struct hll_bytecode *bytecode,
                            bool print_result);

#endif
