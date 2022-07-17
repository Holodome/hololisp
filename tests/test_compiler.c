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
    uint8_t bytecode[] = { // +
                           HLL_BYTECODE_SYMB, 0x00, 0x00, HLL_BYTECODE_FIND,
                           // (1 2)
                           HLL_BYTECODE_NIL, HLL_BYTECODE_NIL,
                           // 1
                           HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_APPEND,
                           // 2
                           HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_APPEND,
                           HLL_BYTECODE_POP,
                           // (+ 1 2)
                           HLL_BYTECODE_CALL, HLL_BYTECODE_END
    };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_compiler_compiles_complex_arithmetic_operation(void) {
    char const *source = "(+ (* 3 5) 2 (/ 2 1))";
    uint8_t bytecode[] = {
        // +
        HLL_BYTECODE_SYMB,
        0x00,
        0x00,
        HLL_BYTECODE_FIND,
        // ((* 3 5) 2 (/ 2 1))
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        // *
        HLL_BYTECODE_SYMB,
        0x00,
        0x01,
        HLL_BYTECODE_FIND,
        // (3 5)
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        // 3
        HLL_BYTECODE_CONST,
        0x00,
        0x00,
        HLL_BYTECODE_APPEND,
        // 5
        HLL_BYTECODE_CONST,
        0x00,
        0x01,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        // (* 3 5)
        HLL_BYTECODE_CALL,
        HLL_BYTECODE_APPEND,
        // 2
        HLL_BYTECODE_CONST,
        0x00,
        0x02,
        HLL_BYTECODE_APPEND,
        // /
        HLL_BYTECODE_SYMB,
        0x00,
        0x02,
        HLL_BYTECODE_FIND,
        // (2 1)
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        // 2
        HLL_BYTECODE_CONST,
        0x00,
        0x02,
        HLL_BYTECODE_APPEND,
        // 1
        HLL_BYTECODE_CONST,
        0x00,
        0x03,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        // (/ 2 1)
        HLL_BYTECODE_CALL,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        // (+ (* 3 5) 2 (/ 2 1))
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
    uint8_t bytecode[] = { // t
                           HLL_BYTECODE_TRUE, HLL_BYTECODE_JN, 0x00, 0x09,
                           // 1
                           HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_NIL,
                           HLL_BYTECODE_JN, 0x00, 0x05,
                           // 0
                           HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_END
    };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_compiler_compiles_quote(void) {
    char const *source = "'(1 2)";
    uint8_t bytecode[] = { HLL_BYTECODE_NIL, HLL_BYTECODE_NIL,
                           // 1
                           HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_APPEND,
                           // 2
                           HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_APPEND,
                           HLL_BYTECODE_POP };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_lambda_application_working(void) {
    char const *source = "((lambda (x) (* x 2)) 10)";
    uint8_t bytecode[] = {
        // (x)
        HLL_BYTECODE_NIL, HLL_BYTECODE_NIL, HLL_BYTECODE_SYMB, 0x00, 0x00,
        HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
        // (* x 2)
        HLL_BYTECODE_NIL, HLL_BYTECODE_NIL, HLL_BYTECODE_SYMB, 0x00, 0x01,
        HLL_BYTECODE_APPEND, HLL_BYTECODE_SYMB, 0x00, 0x00, HLL_BYTECODE_APPEND,
        HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
        // lambda
        HLL_BYTECODE_MAKE_LAMBDA,
        // (10)
        HLL_BYTECODE_NIL, HLL_BYTECODE_NIL, HLL_BYTECODE_CONST, 0x00, 0x01,
        HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
        // (lambda... 10)
        HLL_BYTECODE_CALL, HLL_BYTECODE_END
    };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_compiler_compiles_let(void) {
    char const *source = "(let ((c 2) (a (+ c 1))))";
    uint8_t bytecode[] = {
        HLL_BYTECODE_PUSHENV,
        // c
        HLL_BYTECODE_SYMB,
        0x00,
        0x00,
        // 2
        HLL_BYTECODE_CONST,
        0x00,
        0x00,
        // (c 2)
        HLL_BYTECODE_LET,
        // a
        HLL_BYTECODE_SYMB,
        0x00,
        0x01,
        // +
        HLL_BYTECODE_SYMB,
        0x00,
        0x02,
        HLL_BYTECODE_FIND,
        // (c 1)
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_SYMB,
        0x00,
        0x00,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_CONST,
        0x00,
        0x01,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        // (+ c 1)
        HLL_BYTECODE_CALL,
        HLL_BYTECODE_LET,
        HLL_BYTECODE_POPENV,
        HLL_BYTECODE_END,
    };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    TEST_ASSERT(memcmp(result->ops, bytecode, sizeof(bytecode)) == 0);
}

static void
test_compiler_compiles_let_with_body(void) {
    char const *source = "(let ((c 2) (a (+ c 1))) (* c a) a)";
    uint8_t bytecode[] = {
        HLL_BYTECODE_PUSHENV,
        // c
        HLL_BYTECODE_SYMB,
        0x00,
        0x00,
        // 2
        HLL_BYTECODE_CONST,
        0x00,
        0x00,
        // (c 2)
        HLL_BYTECODE_LET,
        // a
        HLL_BYTECODE_SYMB,
        0x00,
        0x01,
        // +
        HLL_BYTECODE_SYMB,
        0x00,
        0x02,
        HLL_BYTECODE_FIND,
        // (c 1)
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_SYMB,
        0x00,
        0x00,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_CONST,
        0x00,
        0x01,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        // (+ c 1)
        HLL_BYTECODE_CALL,
        HLL_BYTECODE_LET,
        // (* c a)
        HLL_BYTECODE_SYMB,
        0x00,
        0x03,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_NIL,
        HLL_BYTECODE_SYMB,
        0x00,
        0x00,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_SYMB,
        0x00,
        0x01,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_APPEND,
        HLL_BYTECODE_POP,
        HLL_BYTECODE_CALL,
        HLL_BYTECODE_POP,
        // c
        HLL_BYTECODE_SYMB,
        0x00,
        0x01,
        HLL_BYTECODE_FIND,
        HLL_BYTECODE_POPENV,
        HLL_BYTECODE_END,
    };
    hll_vm *vm = hll_make_vm(NULL);

    hll_bytecode *result = hll_compile(vm, source);
    TEST_ASSERT(result != NULL);

    hll_dump_bytecode(stdout, result);

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
              TCASE(test_compiler_compiles_quote),
              TCASE(test_lambda_application_working),
              TCASE(test_compiler_compiles_let),
              TCASE(test_compiler_compiles_let_with_body)};
