#ifndef __HLL_UTILS_H__
#define __HLL_UTILS_H__

#include <stddef.h>
#include <stdint.h>

#include "hll_ext.h"

typedef enum {
    HLL_FS_IO_OK = 0x0,
    HLL_FS_IO_FOPEN_FAILED = 0x1,
    HLL_FS_IO_GET_SIZE_FAILED = 0x2,
    HLL_FS_IO_CLOSE_FILE_FAILED = 0x3,
    HLL_FS_IO_READ_FAILED = 0x4,
    HLL_FS_IO_GET_FULL_PATH_FAILED = 0x5,
    HLL_FS_IO_BUFFER_OVERFLOW = 0x6,
    HLL_FS_IO_WRITE_FAILED = 0x7
} hll_fs_io_result;

char const *hll_get_fs_io_result_string(hll_fs_io_result result);

/// @brief Wrapper around hll_open_file_. Should be used as public interface
#define hll_open_file(_file, _filename, _mode) \
    hll_open_file_((void **)(_file), _filename, _mode)

/// @brief Opens file with given mode. Wrapper around fopen with proper error
/// handling.
/// @param file File pointer to write result to.
/// @param filename Filename to pass to fopen.
/// @param mode Mode to pass to fopen.
/// @return Status code.
hll_fs_io_result hll_open_file_(void **file, char const *filename,
                                char const *mode);

/// @brief Gets file size with error checking.
/// @param file File pointer.
/// @param size Resulting size pointer.
/// @return Status code.
hll_fs_io_result hll_get_file_size(void *file, size_t *size);

/// @brief Closes files with error checking.
/// @param file File pointer.
/// @return Status code.
hll_fs_io_result hll_close_file(void *file);

hll_fs_io_result hll_get_full_file_path(char const *filename, char *buffer,
                                        size_t buffer_size,
                                        size_t *path_length);

hll_fs_io_result hll_read_entire_file(char const *filename, char **data,
                                      size_t *data_size);

hll_fs_io_result hll_write_to_file(char const *filename, char const *data,
                                   size_t data_size);

// @brief Checks if stdin is in interactive mode (like terminal session)
// @return 1 is interactive 0 otherwise
int is_stdin_interactive(void);

typedef struct {
    char *buffer;
    size_t buffer_size;
    size_t written;

    size_t grow_inc;
} hll_string_builder;

hll_string_builder hll_create_string_builder(size_t buffer_size);

HLL_ATTR(format(printf, 2, 3))
void hll_string_builder_printf(hll_string_builder *b, char const *fmt, ...);

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
