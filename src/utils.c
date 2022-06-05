#include "utils.h"

#include <stdio.h>
#include <stdlib.h>

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
