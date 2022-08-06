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

  // Global env frame. Can be used to add c bindings whilst using unified
  // storage.
  struct hll_obj *env;
  // Toplevel env.
  struct hll_obj *global_env;

  uint32_t error_count;
} hll_vm;

// Used to report error in current state contained by vm.
// vm must have current_filename field present if message needs to include
// source location.
// offset specifies byte offset of reported location in file.
// len specifies length of reported part (e.g. token).
void hll_report_error(hll_vm *vm, size_t offset, uint32_t len, const char *msg);

void hll_add_binding(hll_vm *vm, char const *symb,
                     struct hll_obj *(*bind)(hll_vm *vm, struct hll_obj *args));

bool hll_interpret_bytecode(hll_vm *vm, struct hll_bytecode *bytecode,
                            bool print_result);

void hll_print(hll_vm *vm, struct hll_obj *obj, void *file);


#endif
