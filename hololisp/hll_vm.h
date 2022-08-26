//
// Virtual machine definition.
// Virtual machine is what is responsible for executing any hololisp code.

#ifndef HLL_VM_H
#define HLL_VM_H

#include <stdbool.h>
#include <stdint.h>

#include "hll_hololisp.h"

struct hll_bytecode;

typedef struct {
  const struct hll_bytecode *bytecode;
  const uint8_t *ip;
  struct hll_obj *env;
} hll_call_frame;

typedef struct hll_vm {
  struct hll_config config;

  // Stores name of current file being executed. Files are executed atomically,
  // so it can be reused across different compile stages to provide source
  // information.
  const char *current_filename;
  // Current parsed source.
  const char *source;

  struct hll_obj *true_;
  struct hll_obj *nil;
  struct hll_obj *nthcdr_symb;
  struct hll_obj *quote_symb;

  // Global env. It is stored across calls to interpret, allowing defining
  // toplevel functions.
  struct hll_obj *global_env;

  uint32_t error_count;
  // We use xorshift64 for random number generation. Because hololisp is
  // single-threaded, we can use single global variable for rng state.
  uint64_t rng_state;

  // Current execution state
  struct hll_obj **stack;
  hll_call_frame *call_stack;
  struct hll_obj *env;

  //
  // Garbage collector stuff
  //

  // Linked list of all objects.
  // Uses next field
  struct hll_obj *all_objects;
  // Count all allocated bytes to know when to trigger garbage collection.
  size_t bytes_allocated;
  // If bytes_allocated becomes greater than this value, trigger next gc.
  // May not be greater than min_heap_size specified in config.
  size_t next_gc;
  struct hll_obj **gray_objs;
  struct hll_obj **temp_roots;
  uint32_t forbid_gc;
} hll_vm;

// Garbage collector tracked allocation
#define hll_gc_free(_vm, _ptr, _size) hll_gc_realloc(_vm, _ptr, _size, 0)
#define hll_gc_alloc(_vm, _size) hll_gc_realloc(_vm, NULL, 0, _size)
void *hll_gc_realloc(hll_vm *vm, void *ptr, size_t old_size, size_t new_size);

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
void hll_add_variable(hll_vm *vm, struct hll_obj *env, struct hll_obj *name,
                      struct hll_obj *value);
struct hll_obj *hll_expand_macro(hll_vm *vm, struct hll_obj *macro,
                                 struct hll_obj *args);

void hll_print(hll_vm *vm, struct hll_obj *obj, void *file);

struct hll_obj *hll_find_var(hll_vm *vm, struct hll_obj *env,
                             struct hll_obj *car);
void hll_runtime_error(hll_vm *vm, const char *fmt, ...);

#endif
