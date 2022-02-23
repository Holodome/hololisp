#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lisp.h"

int
main(void) {
    char *filename = "examples/a.lisp";

    setbuf(stdout, 0);
    FILE *f = fopen(filename, "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *file_data = calloc(size + 1, 1);
    fread(file_data, size, 1, f);
    fclose(f);

    lisp_runtime *lisp = lisp_create();
    lisp_exec_buffer(lisp, file_data);

    return 0;
}
