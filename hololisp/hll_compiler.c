#include "hll_compiler.h"

#include "hll_vm.h"
#include "hll_hololisp.h"
#include "hll_mem.h"


void *
hll_compile(struct hll_vm *vm, char const *source) {
    hll_memory_arena arena = { 0 };

    hll_lexer lexer = { 0 };
    lexer.cursor = source;
    lexer.error_fn = vm->config.error_fn;
    lexer.line_start = source;

    hll_reader reader = { 0 };
    reader.error_fn = vm->config.error_fn;
    reader.lexer = &lexer;
    reader.arena = &arena;

    hll_ast *ast = hll_read_ast(&reader);

    hll_compiler compiler = { 0 };

    void *result = hll_compile_ast(&compiler, ast);

    hll_memory_arena_clear(&arena);

    return result;
}

hll_ast *
hll_read_ast(hll_reader *reader) {
    (void)reader;
    return NULL;
}

void *
hll_compile_ast(hll_compiler *compiler, hll_ast *ast) {
    (void)compiler;
    (void)ast;
    return NULL;
}