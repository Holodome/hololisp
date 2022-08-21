#include "hll_mem.h"

#include <assert.h>
#include <stdlib.h>

void *hll_sb_grow_impl(void *arr, size_t inc, size_t stride) {
  if (arr == NULL) {
    void *result = hll_alloc(sizeof(hll_sb_header) + stride * inc);
    assert(result != NULL);
    hll_sb_header *header = result;
    header->size = 0;
    header->capacity = inc;
    return header + 1;
  }

  hll_sb_header *header = hll_sb_header(arr);
  size_t double_current = header->capacity * 2;
  size_t min_needed = header->size + inc;

  size_t new_capacity =
      double_current > min_needed ? double_current : min_needed;
  void *result =
      hll_realloc(header, sizeof(hll_sb_header) + stride * header->capacity,
                  sizeof(hll_sb_header) + stride * new_capacity);
  header = result;
  header->capacity = new_capacity;
  return header + 1;
}

#ifdef HLL_MEM_CHECK
static size_t global_allocated_size;
size_t hll_mem_check(void) { return global_allocated_size; }
#endif

void *hll_realloc(void *ptr, size_t old_size, size_t new_size) {
#ifdef HLL_MEM_CHECK
  assert(global_allocated_size >= old_size);
  global_allocated_size -= old_size;
  global_allocated_size += new_size;
#else
  (void)old_size;
#endif
  if (old_size == 0) {
    assert(ptr == NULL);
    return calloc(1, new_size);
  }

  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  return realloc(ptr, new_size);
}
