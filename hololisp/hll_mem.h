//
// This file contains functions related to memory used in hololisp.
//
#ifndef HLL_MEM_H
#define HLL_MEM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Stretchy buffer
// This is implementation of type-safe generic vector in C based on
// std_stretchy_buffer.

struct hll_sb_header {
  size_t size;
  size_t capacity;
};

#define hll_sb_header(_a)                                                      \
  ((struct hll_sb_header *)((char *)(_a) - sizeof(struct hll_sb_header)))
#define hll_sb_size(_a) (hll_sb_header(_a)->size)
#define hll_sb_capacity(_a) (hll_sb_header(_a)->capacity)

#define hll_sb_needgrow(_a, _n)                                                \
  (((_a) == NULL) || (hll_sb_size(_a) + (_n) >= hll_sb_capacity(_a)))
#define hll_sb_maybegrow(_a, _n)                                               \
  (hll_sb_needgrow(_a, _n) ? hll_sb_grow(_a, _n) : 0)
#define hll_sb_grow(_a, _b)                                                    \
  (*(void **)(&(_a)) = hll_sb_grow_impl((_a), (_b), sizeof(*(_a))))

#define hll_sb_free(_a)                                                        \
  (((_a) != NULL)                                                              \
   ? hll_free(hll_sb_header(_a), hll_sb_capacity(_a) * sizeof(*(_a)) +         \
                                     sizeof(struct hll_sb_header)),            \
   0 : 0)
#define hll_sb_push(_a, _v)                                                    \
  (hll_sb_maybegrow(_a, 1), (_a)[hll_sb_size(_a)++] = (_v))
#define hll_sb_last(_a) ((_a)[hll_sb_size(_a) - 1])
#define hll_sb_len(_a) (((_a) != NULL) ? hll_sb_size(_a) : 0)
#define hll_sb_pop(_a) ((_a)[--hll_sb_size(_a)])
#define hll_sb_purge(_a) ((_a) ? (hll_sb_size(_a) = 0) : 0)

void *hll_sb_grow_impl(void *arr, size_t inc, size_t stride);

#define hll_free(_ptr, _size) (void)hll_realloc(_ptr, _size, 0)
#define hll_alloc(_size) hll_realloc(NULL, 0, _size)

// realloc that hololisp uses for all memory allocations internally.
// If new_size is 0, behaves as 'free'.
// If old_size is 0, behaves as 'calloc'
// Otherwise behaves as 'realloc'
void *hll_realloc(void *ptr, size_t old_size, size_t new_size);

#ifdef HLL_MEM_CHECK
// Retturns number of bytes allocated but not freed.
size_t hll_mem_check(void);
#endif

#endif
