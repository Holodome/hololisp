#ifndef __UTILS_H__
#define __UTILS_H__

#include <stddef.h>

typedef enum {
    HLL_FS_IO_OK = 0x0,
    HLL_FS_IO_FOPEN_FAILED = 0x1,
    HLL_FS_IO_GET_SIZE_FAILED = 0x2,
    HLL_FS_IO_CLOSE_FILE_FAILED = 0x3
} hll_fs_io_result;

#define hll_open_file(_file, _filename, _mode) hll_open_file_((void **)(_file), _filename, _mode)
/// Opens file with given mode. Wrapper around fopen with proper error handling.
/// \param file File pointer to write result to.
/// \param filename Filename to pass to fopen.
/// \param mode Mode to pass to fopen.
/// \return Status code.
hll_fs_io_result hll_open_file_(void **file, char const *filename, char const *mode);

/// Gets file size with error checking.
/// \param file File pointer.
/// \param size Resulting size pointer.
/// \return Status code.
hll_fs_io_result hll_get_file_size(void *file, size_t *size);

/// Closes files with error checking.
/// \param file File pointer.
/// \return Status code.
hll_fs_io_result hll_close_file(void *file);

#endif
