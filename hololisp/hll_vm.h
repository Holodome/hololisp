///
/// Virtual machine definition.
/// Virtual machine is what is responsible for executing any hololisp code.

///
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
} hll_vm;

bool hll_interpret_bytecode(hll_vm *vm, struct hll_bytecode *bytecode);

#endif
