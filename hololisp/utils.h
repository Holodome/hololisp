#ifndef __HLL_UTILS_H__
#define __HLL_UTILS_H__

#include <stddef.h>

typedef enum {
    HLL_FS_IO_OK = 0x0,
    HLL_FS_IO_FOPEN_FAILED = 0x1,
    HLL_FS_IO_GET_SIZE_FAILED = 0x2,
    HLL_FS_IO_CLOSE_FILE_FAILED = 0x3,
    HLL_FS_IO_READ_FAILED = 0x4,
    HLL_FS_IO_GET_FULL_PATH_FAILED = 0x4,
    HLL_FS_IO_BUFFER_UNDERFLOW = 0x5,
} hll_fs_io_result;

/// @brief Wrapper around hll_open_file_. Should be used as public interface
/// @breief
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

// @brief Checks if stdin is in interactive mode (like terminal session)
// @return 1 is interactive 0 otherwise
int is_stdin_interactive(void);

#endif
