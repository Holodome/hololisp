///
/// Virtual machine definition.
/// Virtual machine is what is responsible for executing any hololisp code.

///
#ifndef __HLL_VM_H__
#define __HLL_VM_H__

#include <stdbool.h>
#include <stdint.h>

struct hll_config;

typedef struct hll_vm {
    struct hll_config *config;
} hll_vm;

#endif
