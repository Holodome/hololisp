#include "hll_mem.h"

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

static hll_memory_arena_block *
alloc_block(size_t size, size_t align) {
    size_t size_to_allocate =
        _align_forward_pow2(size + sizeof(hll_memory_arena_block), align);
    void *memory = calloc(size_to_allocate, 1);
    hll_memory_arena_block *block = memory;

    block->data = (char *)_align_forward_pow2(
        (uintptr_t)block + sizeof(hll_memory_arena_block), align);
    block->size = size;

    return block;
}

void *
hll_memory_arena_alloc(hll_memory_arena *arena, size_t size) {
    if (size == 0) {
        return NULL;
    }

    hll_memory_arena_block *block = arena->block;
    size_t size_aligned = _align_forward_pow2(size, HLL_MEMORY_ARENA_ALIGN);
    if (block == NULL || block->used + size_aligned > block->size) {
        if (!arena->min_block_size) {
            arena->min_block_size = HLL_MEMORY_ARENA_DEFAULT_BLOCK_SIZE;
        }

        size_t block_size = size_aligned;
        if (arena->min_block_size > block_size) {
            block_size = arena->min_block_size;
        }

        block = alloc_block(block_size, HLL_MEMORY_ARENA_ALIGN);
        block->next = arena->block;
        arena->block = block;
    }

    size_t align_mask = HLL_MEMORY_ARENA_ALIGN - 1;
    assert(!(HLL_MEMORY_ARENA_ALIGN & align_mask));
    assert(!((uintptr_t)block->data & align_mask));

    uintptr_t start_offset = (uintptr_t)block->data + block->used;
    assert(!(start_offset & align_mask));

    void *result = (char *)start_offset;
    block->used += size_aligned;
    assert(!(block->used & align_mask));

    return result;
}

static void
_free_last_block(hll_memory_arena *arena) {
    hll_memory_arena_block *block = arena->block;
    arena->block = block->next;
    free(block);
}

void
hll_memory_arena_clear(hll_memory_arena *arena) {
    while (arena->block != NULL) {
        int is_last = arena->block->next == NULL;
        _free_last_block(arena);
        if (is_last) {
            break;
        }
    }
}

void *
hll_sb_grow_impl(void *arr, size_t inc, size_t stride) {
    if (arr == NULL) {
        void *result = malloc(sizeof(hll_sb_header) + stride * inc);
        assert(result != NULL);
        hll_sb_header *header = result;
        header->size = 0;
        header->capacity = inc;
        return header + 1;
    }

    hll_sb_header *header = hll_sb_header(arr);
    size_t double_current = header->capacity * 2;
    size_t min_needed = header->size + inc;

    size_t new_capacity = double_current > min_needed ? double_current : min_needed;
    void *result = realloc(header, sizeof(hll_sb_header) + stride * new_capacity);
    header = result;
    header->capacity = new_capacity;
    return header + 1;
}
