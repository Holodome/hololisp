///
/// Virtual machine definition.
/// Virtual machine is what is responsible for executing any hololisp code.

///
#ifndef __HLL_VM_H__
#define __HLL_VM_H__

#include <stdbool.h>
#include <stdint.h>

#include "hll_hololisp.h"

typedef struct hll_vm {
    struct hll_config config;
} hll_vm;

hll_interpret_result hll_interpret_bytecode(hll_vm *vm, void *bytecode);

#endif
