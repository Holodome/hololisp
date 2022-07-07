///
/// This file contains functions related to memory used in hololisp.
///
#ifndef __HLL_MEM_H__
#define __HLL_MEM_H__

#include <stdint.h>
#include <stddef.h>

/// Stretchy buffer
/// This is implementation of type-safe generic vector in C based on std_stretchy_buffer.

typedef struct {
    size_t size;
    size_t capacity;
} hll_sb_header;

#define _hll_sb_header(_a) ((hll_sb_header *)((char *)(_a) - sizeof(hll_sb_header)))
#define _hll_sb_size(_a) (_hll_sb_header(_a)->size)
#define _hll_sb_capacity(_a) (_hll_sb_header(_a)->capacity)

#define _hll_sb_needgrow(_a, _n) (((_a) == NULL) || (_hll_sb_size(_a) + (_n) >= _hll_sb_capacity(_a)))
#define _hll_sb_maybegrow(_a, _n) (_hll_sb_needgrow(_a, _n) ? hll_sb_grow(_a, _n) : 0)
#define _hll_sb_grow(_a, _b) (*(void **)(&(_a)) = hll_sb_grow_impl((_a), (_b), sizeof(*(_a))))

#define hll_sb_free(_a) (((_a) != NULL) ? free(_hll_sb_header(_a)),0 : 0)
#define hll_sb_push(_a, _v) (_hll_sb_maybegrow(_a, 1), (_a)[_hll_sb_size(_a)++] = (_v))
#define hll_sb_last(_a) ((_a)[_hll_sb_size(_a) - 1])
#define hll_sb_len(_a) (((_a) != NULL) ? _hll_sb_size(_a) : 0)

void *hll_sb_grow_impl(void *arr, size_t inc, size_t stride);

#define HLL_MEMORY_ARENA_DEFAULT_BLOCK_SIZE (1 << 20)
#define HLL_MEMORY_ARENA_ALIGN 16

typedef struct _hll_memory_arena_block {
    struct _hll_memory_arena_block *next;
    size_t used;
    size_t size;
    char *data;
} _hll_memory_arena_block;

typedef struct hll_memory_arena {
    _hll_memory_arena_block *block;
    size_t min_block_size;
} hll_memory_arena;

void *hll_memory_arena_alloc(hll_memory_arena *arena, size_t size);
void hll_memory_arena_clear(hll_memory_arena *arena);

#endif
