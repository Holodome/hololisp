#ifndef __HLMA_H__
#define __HLMA_H__

#include <stddef.h>  // size_t
#include <stdint.h>

#ifndef HLMA_DEF
#ifdef HLMA_STATIC
#define HLMA_DEF static
#else
#define HLMA_DEF extern
#endif
#endif  // HLMA_DEF

#ifndef HLMA_DEFAULT_BLOCK_SIZE
#define HLMA_DEFAULT_BLOCK_SIZE (1 << 20)
#endif

#ifndef HLMA_ALIGN
#define HLMA_ALIGN 16
#endif  // HLMA_ALIGN

#ifndef HLMA_DEBUG_INFO
#define HLMA_DEBUG_INFO 0
#endif  // HLMA_DEBUG_INFO

typedef struct hlma_block {
    struct hlma_block *next;
    size_t used;
    size_t size;
    char *data;
} hlma_block;

typedef struct hlma_arena {
    hlma_block *block;
    size_t min_block_size;

#if HLMA_DEBUG_INFO
    size_t allocation_count;
    size_t total_size;
#endif  // HLMA_DEBUG_INFO
} hlma_arena;

#if defined(__GNUC__) || defined(__clang__)
__attribute__((malloc))
#endif
HLMA_DEF void *
hlma_alloc(hlma_arena *arena, size_t size);

HLMA_DEF void hlma_clear(hlma_arena *arena);

#endif  //  __HLMA_H__
#ifdef HLMA_IMPL

#include <assert.h>
#include <stdlib.h>

static size_t
_align_forward_pow2(size_t value, size_t align) {
    size_t align_mask = align - 1;
    assert(!(align_mask & align));
    size_t align_value = value & align_mask;
    size_t result = value;
    if (align_mask) {
        result += align - align_value;
    }
    return result;
}

static hlma_block *
_alloc_block(size_t size, size_t align) {
    size_t size_to_allocate =
        _align_forward_pow2(size + sizeof(hlma_block), align);
    void *memory = calloc(size_to_allocate, 1);
    hlma_block *block = memory;

    block->data = (char *)_align_forward_pow2(
        (uintptr_t)block + sizeof(hlma_block), align);
    block->size = size;

    return block;
}

HLMA_DEF void *
hlma_alloc(hlma_arena *arena, size_t size) {
    if (size == 0) {
        return NULL;
    }

    hlma_block *block = arena->block;
    size_t size_aligned = _align_forward_pow2(size, HLMA_ALIGN);
    if (block == NULL || block->used + size_aligned > block->size) {
        if (!arena->min_block_size) {
            arena->min_block_size = HLMA_DEFAULT_BLOCK_SIZE;
        }

        size_t block_size = size_aligned;
        if (arena->min_block_size > block_size) {
            block_size = arena->min_block_size;
        }

        block = _alloc_block(block_size, HLMA_ALIGN);
        block->next = arena->block;
        arena->block = block;
    }

    size_t align_mask = HLMA_ALIGN - 1;
    assert(!(HLMA_ALIGN & align_mask));
    assert(!((uintptr_t)block->data & align_mask));

    uintptr_t start_offset = (uintptr_t)block->data + block->used;
    assert(!(start_offset & align_mask));

    void *result = (char *)start_offset;
    block->used += size_aligned;
    assert(!(block->used & align_mask));

    return result;
}

static void
_free_last_block(hlma_arena *arena) {
    hlma_block *block = arena->block;
    arena->block = block->next;
    free(block);
}

HLMA_DEF void
hlma_clear(hlma_arena *arena) {
    while (arena->block != NULL) {
        int is_last = arena->block->next == NULL;
        _free_last_block(arena);
        if (is_last) {
            break;
        }
    }
}

#endif  // HLMA_IMPL
