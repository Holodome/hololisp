#include "hll_mem.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

void *hll_sb_grow_impl(void *arr, size_t inc, size_t stride) {
  if (arr == NULL) {
    void *result = hll_alloc(sizeof(struct hll_sb_header) + stride * inc);
    struct hll_sb_header *header = result;
    header->size = 0;
    header->capacity = inc;
    return header + 1;
  }

  struct hll_sb_header *header = hll_sb_header(arr);
  size_t double_current = header->capacity * 2;
  size_t min_needed = header->size + inc;

  size_t new_capacity =
      double_current > min_needed ? double_current : min_needed;
  void *result = hll_realloc(
      header, sizeof(struct hll_sb_header) + stride * header->capacity,
      sizeof(struct hll_sb_header) + stride * new_capacity);
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
    void *result = calloc(1, new_size);
    if (result == NULL) {
      perror("failed to allocate memory");
      exit(EXIT_FAILURE);
    }
    return result;
  }

  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);
  if (result == NULL) {
    perror("failed to allocate memory");
    exit(EXIT_FAILURE);
  }
  return result;
}
