#include "hll_utils.h"

// 2008 edition of the POSIX standard (IEEE Standard 1003.1-2008)
#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __linux__
#include <linux/limits.h>
#endif

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
    hll_fs_io_result result;

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

