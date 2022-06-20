#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // isatty
#define _POSIX_C_SOURCE 1
#define _POSIX_SOURCE 1
#include <limits.h>

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
            result = HLL_FS_IO_BUFFER_UNDERFLOW;
        } else {
            *path_length = length;
            strcpy(buffer, local_buffer);
        }
    }

    return result;
}

hll_fs_io_result
hll_read_entire_file(char const *filename, char **data, size_t *data_size) {
    hll_fs_io_result result = HLL_FS_IO_OK;

    FILE *f = NULL;
    if ((result = hll_open_file(&f, filename, "r")) == HLL_FS_IO_OK) {
        size_t size = 0;
        if ((result = hll_get_file_size(f, &size)) == HLL_FS_IO_OK) {
            char *buffer = calloc(1, size + 1);
            if (fread(buffer, size, 1, f) != 1) {
                result = HLL_FS_IO_READ_FAILED;
            } else {
                *data_size = size;
                *data = buffer;
            }
        }
    }

    return result;
}
