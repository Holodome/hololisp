#include "hll_utils.h"

// 2008 edition of the POSIX standard (IEEE Standard 1003.1-2008)
#define _POSIX_C_SOURCE 200809L
#define _POSIX_SOURCE 200809L
#define _GNU_SOURCE 1

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/limits.h>
#endif

char const *
hll_get_fs_io_result_string(hll_fs_io_result result) {
    char const *str = NULL;
    switch (result) {
    case HLL_FS_IO_OK:
        str = "ok";
        break;
    case HLL_FS_IO_FOPEN_FAILED:
        str = "failed to open file";
        break;
    case HLL_FS_IO_GET_SIZE_FAILED:
        str = "failed to get file size";
        break;
    case HLL_FS_IO_CLOSE_FILE_FAILED:
        str = "failed to close file";
        break;
    case HLL_FS_IO_READ_FAILED:
        str = "failed to read from file";
        break;
    case HLL_FS_IO_GET_FULL_PATH_FAILED:
        str = "failed to get full path";
        break;
    case HLL_FS_IO_BUFFER_OVERFLOW:
        str = "buffer is too small";
        break;
    case HLL_FS_IO_WRITE_FAILED:
        str = "failed to write to file";
        break;
    }

    return str;
}

hll_fs_io_result
hll_open_file_(void **file, char const *filename, char const *mode) {
    hll_fs_io_result result = HLL_FS_IO_OK;

    FILE *temp = fopen(filename, mode);
    if (temp == NULL) {
        result = HLL_FS_IO_FOPEN_FAILED;
    } else {
        *file = temp;
    }

    return result;
}

hll_fs_io_result
hll_get_file_size(void *file, size_t *size) {
    hll_fs_io_result result = HLL_FS_IO_OK;

    if (fseek(file, 0, SEEK_END) != 0) {
        result = HLL_FS_IO_GET_SIZE_FAILED;
    } else {
        int file_size = ftell(file);

        if (file_size < 0 || fseek(file, 0, SEEK_SET)) {
            result = HLL_FS_IO_GET_SIZE_FAILED;
        } else {
            *size = file_size;
        }
    }

    return result;
}

hll_fs_io_result
hll_close_file(void *file) {
    hll_fs_io_result result = HLL_FS_IO_OK;

    if (fclose(file) != 0) {
        result = HLL_FS_IO_CLOSE_FILE_FAILED;
    }

    return result;
}

int
is_stdin_interactive(void) {
    return isatty(STDIN_FILENO);
}

hll_fs_io_result
hll_get_full_file_path(char const *filename, char *buffer, size_t buffer_size,
                       size_t *path_length) {
    hll_fs_io_result result = HLL_FS_IO_OK;
    char local_buffer[PATH_MAX + 1];
    if (realpath(filename, local_buffer) == NULL) {
        result = HLL_FS_IO_GET_FULL_PATH_FAILED;
    } else {
        size_t length = strlen(local_buffer);
        if (length + 1 >= buffer_size) {
            result = HLL_FS_IO_BUFFER_OVERFLOW;
        } else {
            *path_length = length;
            strcpy(buffer, local_buffer);
        }
    }

    return result;
}

hll_fs_io_result
hll_read_entire_file(char const *filename, char **data, size_t *data_size) {
    hll_fs_io_result result;

    FILE *f = NULL;
    if ((result = hll_open_file(&f, filename, "r")) == HLL_FS_IO_OK) {
        size_t size = 0;
        if ((result = hll_get_file_size(f, &size)) == HLL_FS_IO_OK) {
            char *buffer = calloc(1, size + 1);
            if (fread(buffer, size, 1, f) != 1) {
                result = HLL_FS_IO_READ_FAILED;
            } else {
                if (data_size != NULL) {
                    *data_size = size;
                }
                *data = buffer;
            }
        }
    }

    return result;
}

hll_fs_io_result
hll_write_to_file(char const *filename, char const *data, size_t data_size) {
    hll_fs_io_result result;

    FILE *f = NULL;
    if ((result = hll_open_file(&f, filename, "wb")) == HLL_FS_IO_OK) {
        if (fwrite(data, data_size, 1, f) != 1) {
            result = HLL_FS_IO_WRITE_FAILED;
        } else {
            result = HLL_FS_IO_OK;
        }
    }

    return result;
}

hll_string_builder
hll_create_string_builder(size_t size) {
    hll_string_builder b = { 0 };

    b.buffer = calloc(size, 1);
    b.buffer_size = size;
    b.grow_inc = 4096;

    return b;
}

void
hll_string_builder_printf(hll_string_builder *b, char const *fmt, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    size_t written = vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (b->written + written > b->buffer_size) {
        b->buffer_size += b->grow_inc;
        b->buffer = realloc(b->buffer, b->buffer_size);
    }

    strcpy(b->buffer + b->written, buffer);
    b->written += written;
}

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

static _hll_memory_arena_block *
alloc_block(size_t size, size_t align) {
    size_t size_to_allocate =
        _align_forward_pow2(size + sizeof(_hll_memory_arena_block), align);
    void *memory = calloc(size_to_allocate, 1);
    _hll_memory_arena_block *block = memory;

    block->data = (char *)_align_forward_pow2(
        (uintptr_t)block + sizeof(_hll_memory_arena_block), align);
    block->size = size;

    return block;
}

void *
hll_memory_arena_alloc(hll_memory_arena *arena, size_t size) {
    if (size == 0) {
        return NULL;
    }

    _hll_memory_arena_block *block = arena->block;
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
    _hll_memory_arena_block *block = arena->block;
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
