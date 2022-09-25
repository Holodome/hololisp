#ifndef HLL_GC_H
#define HLL_GC_H

#include "hll_hololisp.h"

typedef struct hll_gc {
  // backpointer to vm. Although it creates circular reference,
  // it is unavoidable in cotext of vm. GC is deeply integrated into VM runtime,
  // su separating them would be unreasanoble.
  struct hll_vm *vm;

  // Linked list of all objects.
  struct hll_obj *all_objs;
  // Count all allocated bytes to know when to trigger garbage collection.
  size_t bytes_allocated;
  // If bytes_allocated becomes greater than this value, trigger next gc.
  // May not be greater than min_heap_size specified in config.
  size_t next_gc;
  hll_value *gray_objs;
  hll_value *temp_roots;
  uint32_t forbid;
} hll_gc;

hll_gc *hll_make_gc(struct hll_vm *vm);
void hll_delete_gc(hll_gc *gc);

void hll_push_forbid_gc(hll_gc *gc);
void hll_pop_forbid_gc(hll_gc *gc);

void hll_gc_push_temp_root(hll_gc *gc, hll_value value);
void hll_gc_pop_temp_root(hll_gc *gc);

// Garbage collector tracked allocation
#define hll_gc_free(_vm, _ptr, _size) hll_gc_realloc(_vm, _ptr, _size, 0)
#define hll_gc_alloc(_vm, _size) hll_gc_realloc(_vm, NULL, 0, _size)
void *hll_gc_realloc(hll_gc *gc, void *ptr, size_t old_size, size_t new_size)
    __attribute__((alloc_size(4)));

#endif
