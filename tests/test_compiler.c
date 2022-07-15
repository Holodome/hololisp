#include "../hololisp/hll_bytecode.h"
#include "../hololisp/hll_compiler.h"
#include "../hololisp/hll_vm.h"
#include "acutest.h"

static void
test_compiler_compiles_integer(void) {
    char const *source = "1";
    uint8_t bytecode[] = { HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_END };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_compiler_compiles_addition(void) {
    char const *source = "(+ 1 2)";
    uint8_t bytecode[] = { HLL_BYTECODE_SYMB,
                           0x00,
                           0x00,
                           HLL_BYTECODE_FIND,
                           HLL_BYTECODE_NIL,
                           HLL_BYTECODE_NIL,
                           HLL_BYTECODE_CONST,
                           0x00,
                           0x00,
                           HLL_BYTECODE_APPEND,
                           HLL_BYTECODE_CONST,
                           0x00,
                           0x01,
                           HLL_BYTECODE_APPEND,
                           HLL_BYTECODE_POP,
                           HLL_BYTECODE_CALL,
                           HLL_BYTECODE_END };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_compiler_compiles_complex_arithmetic_operation(void) {
    char const *source = "(+ (* 3 5) 2 (/ 2 1))";
    uint8_t bytecode[] = {
        HLL_BYTECODE_SYMB,
        0x00,
        0x00,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_SYMB,
        0x00,
        0x01,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_CONST,
        0x00,
        0x00,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_CONST,
        0x00,
        0x01,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        HLL_BYTECODE_CALL,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_CONST,
        0x00,
        0x02,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_SYMB,
        0x00,
        0x02,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_CONST,
        0x00,
        0x02,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_CONST,
        0x00,
        0x03,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        HLL_BYTECODE_CALL,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        HLL_BYTECODE_CALL,
        HLL_BYTECODE_END,

    };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_compiler_compiles_if(void) {
    /*
0:#0    TRUE
1:#1    JN 9 (->#B)
2:#4    CONST 1
3:#7    TRUE
4:#8    JT 5 (->#E)
5:#B    CONST 0
6:#E    END
     */
    char const *source = "(if t 1 0)";
    uint8_t bytecode[] = { HLL_BYTECODE_TRUE,
                           HLL_BYTECODE_JN,
                           0x00,
                           0x09,
                           HLL_BYTECODE_CONST,
                           0x00,
                           0x00,
                           HLL_BYTECODE_TRUE,
                           HLL_BYTECODE_JT,
                           0x00,
                           0x05,
                           HLL_BYTECODE_CONST,
                           0x00,
                           0x01,
                           HLL_BYTECODE_END };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_compiler_compiles_quote(void) {
    char const *source = "'(1 2)";
    uint8_t bytecode[] = { HLL_BYTECODE_NIL,
                           HLL_BYTECODE_NIL,
                           HLL_BYTECODE_CONST,
                           0x00,
                           0x00,
                           HLL_BYTECODE_APPEND,
                           HLL_BYTECODE_CONST,
                           0x00,
                           0x01,
                           HLL_BYTECODE_APPEND,
                           HLL_BYTECODE_POP };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

#define TCASE(_name)  \
    {                 \
#_name, _name \
    }

TEST_LIST = { TCASE(test_compiler_compiles_integer),
              TCASE(test_compiler_compiles_addition),
              TCASE(test_compiler_compiles_complex_arithmetic_operation),
              TCASE(test_compiler_compiles_if),
              TCASE(test_compiler_compiles_quote) };
