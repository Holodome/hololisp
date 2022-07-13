#include "hll_vm.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "hll_compiler.h"
#include "hll_hololisp.h"

static void
default_error_fn(hll_vm *vm, uint32_t line, uint32_t column,
                 char const *message) {
    (void)vm;
    fprintf(stderr, "[ERROR]: %" PRIu32 ":%" PRIu32 ": %s", line + 1,
            column + 1, message);
}

static void
default_write_fn(hll_vm *vm, char const *text) {
    (void)vm;
    printf("%s", text);
}

static void
initialize_default_config(hll_config *config) {
    config->write_fn = default_write_fn;
    config->error_fn = default_error_fn;

    config->heap_size = 10 << 20;
    config->min_heap_size = 1 << 20;
    config->heap_grow_percent = 50;

    config->user_data = NULL;
}

hll_vm *
hll_make_vm(hll_config const *config) {
    hll_vm *vm = calloc(1, sizeof(hll_vm));

    if (config == NULL) {
        initialize_default_config(&vm->config);
    } else {
        vm->config = *config;
    }

    return vm;
}

void
hll_delete_vm(hll_vm *vm) {
    free(vm);
}

hll_interpret_result
hll_interpret(hll_vm *vm, char const *source) {
    void *bytecode = hll_compile(vm, source);
    if (bytecode == NULL) {
        return HLL_RESULT_COMPILE_ERROR;
    }

    return hll_interpret_bytecode(vm, bytecode);
}

hll_interpret_result
hll_interpret_bytecode(hll_vm *vm, void *bytecode) {
    (void)vm;
    (void)bytecode;
    return HLL_RESULT_OK;
}
