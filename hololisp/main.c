#include <stdio.h>
#include <stdlib.h>

#include "hll_hololisp.h"

static char *
read_entire_file(char const *filename) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    int file_size = ftell(f);
    if (file_size < 0) {
        fclose(f);
        return NULL;
    }

    rewind(f);

    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(f);
        return NULL;
    }

    if (fread(buffer, file_size, 1, f) != 1) {
        free(buffer);
        fclose(f);
        return NULL;
    }

    buffer[file_size] = '\0';

    if (fclose(f) != 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

int
main(int argc, char const **argv) {
    if (argc < 2) {
        fprintf(stderr, "invalid number of arguments\n");
        return EXIT_FAILURE;
    }

    char const *filename = argv[1];
    char *file_contents = read_entire_file(filename);
    if (file_contents == NULL) {
        fprintf(stderr, "failed to read file '%s'\n", filename);
        return EXIT_FAILURE;
    }

    struct hll_vm *vm = hll_make_vm(NULL);
    hll_interpret_result result = hll_interpret(vm, file_contents);

    int rc = EXIT_SUCCESS;
    switch (result) {
    case HLL_RESULT_COMPILE_ERROR:
        fprintf(stderr, "Compile error\n");
        rc = EXIT_FAILURE;
        break;
    case HLL_RESULT_RUNTIME_ERROR:
        fprintf(stderr, "Runtime error\n");
        rc = EXIT_FAILURE;
        break;
    case HLL_RESULT_OK: break;
    }

    hll_delete_vm(vm);
    free(file_contents);

    return rc;
}
