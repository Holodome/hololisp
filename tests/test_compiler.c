#include "../hololisp/hll_bytecode.h"
#include "../hololisp/hll_compiler.h"
#include "../hololisp/hll_vm.h"
#define TEST_MSG_MAXSIZE 16384
#include "acutest.h"

static void test_bytecode_equals(uint8_t *expected, size_t expected_len,
                                 hll_bytecode *test) {
  TEST_ASSERT(test != NULL);

  TEST_CHECK(expected_len == hll_sb_len(test->ops) &&
             memcmp(expected, test->ops, expected_len) == 0);

  FILE *f = tmpfile();
  hll_dump_bytecode(f, test);
  size_t len = ftell(f);
  rewind(f);

  char b[16384] = {0};
  fread(b, len, 1, f);
  TEST_MSG("expected: %s", b);
}

static void test_compiler_compiles_integer(void) {
  char const *source = "1";
  uint8_t bytecode[] = {HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_END};
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_compiles_addition(void) {
  char const *source = "(+ 1 2)";
  uint8_t bytecode[] = {
      // +
      HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_FIND, HLL_BYTECODE_CDR,
      // (1 2)
      HLL_BYTECODE_NIL, HLL_BYTECODE_NIL,
      // 1
      HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_APPEND,
      // 2
      HLL_BYTECODE_CONST, 0x00, 0x02, HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
      // (+ 1 2)
      HLL_BYTECODE_CALL, HLL_BYTECODE_END};
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_compiles_complex_arithmetic_operation(void) {
  char const *source = "(+ (* 3 5) 2 (/ 2 1))";
  uint8_t bytecode[] = {
      // +
      HLL_BYTECODE_CONST,
      0x00,
      0x00,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      // ((* 3 5) 2 (/ 2 1))
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_NIL,
      // *
      HLL_BYTECODE_CONST,
      0x00,
      0x01,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      // (3 5)
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_NIL,
      // 3
      HLL_BYTECODE_CONST,
      0x00,
      0x02,
      HLL_BYTECODE_APPEND,
      // 5
      HLL_BYTECODE_CONST,
      0x00,
      0x03,
      HLL_BYTECODE_APPEND,
      HLL_BYTECODE_POP,
      // (* 3 5)
      HLL_BYTECODE_CALL,
      HLL_BYTECODE_APPEND,
      // 2
      HLL_BYTECODE_CONST,
      0x00,
      0x04,
      HLL_BYTECODE_APPEND,
      // /
      HLL_BYTECODE_CONST,
      0x00,
      0x05,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      // (2 1)
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_NIL,
      // 2
      HLL_BYTECODE_CONST,
      0x00,
      0x04,
      HLL_BYTECODE_APPEND,
      // 1
      HLL_BYTECODE_CONST,
      0x00,
      0x06,
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
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_compiles_if(void) {
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
  uint8_t bytecode[] = {// t
                        HLL_BYTECODE_TRUE, HLL_BYTECODE_JN, 0x00, 0x09,
                        // 1
                        HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_NIL,
                        HLL_BYTECODE_JN, 0x00, 0x05,
                        // 0
                        HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_END};
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_compiles_quote(void) {
  char const *source = "'(1 2)";
  uint8_t bytecode[] = {HLL_BYTECODE_NIL, HLL_BYTECODE_NIL,
                        // 1
                        HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_APPEND,
                        // 2
                        HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP, HLL_BYTECODE_END};
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_lambda_application_working(void) {
  char const *source = "((lambda (x) (* x 2)) 10)";
  uint8_t bytecode[] = {// (x)
                        HLL_BYTECODE_NIL, HLL_BYTECODE_NIL, HLL_BYTECODE_CONST,
                        0x00, 0x00, HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
                        // (* x 2)
                        HLL_BYTECODE_NIL, HLL_BYTECODE_NIL, HLL_BYTECODE_CONST,
                        0x00, 0x01, HLL_BYTECODE_APPEND, HLL_BYTECODE_CONST,
                        0x00, 0x00, HLL_BYTECODE_APPEND, HLL_BYTECODE_CONST,
                        0x00, 0x02, HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
                        // lambda
                        HLL_BYTECODE_MAKE_LAMBDA,
                        // (10)
                        HLL_BYTECODE_NIL, HLL_BYTECODE_NIL, HLL_BYTECODE_CONST,
                        0x00, 0x03, HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
                        // (lambda... 10)
                        HLL_BYTECODE_CALL, HLL_BYTECODE_END};
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_compiles_let(void) {
  char const *source = "(let ((c 2) (a (+ c 1))))";
  uint8_t bytecode[] = {
      HLL_BYTECODE_PUSHENV,
      // c
      HLL_BYTECODE_CONST,
      0x00,
      0x00,
      // 2
      HLL_BYTECODE_CONST,
      0x00,
      0x01,
      // (c 2)
      HLL_BYTECODE_LET,
      // a
      HLL_BYTECODE_CONST,
      0x00,
      0x02,
      // +
      HLL_BYTECODE_CONST,
      0x00,
      0x03,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      // (c 1)
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_CONST,
      0x00,
      0x00,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      HLL_BYTECODE_APPEND,
      HLL_BYTECODE_CONST,
      0x00,
      0x04,
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
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_compiles_let_with_body(void) {
  char const *source = "(let ((c 2) (a (+ c 1))) (* c a) a)";
  uint8_t bytecode[] = {
      HLL_BYTECODE_PUSHENV,
      // c
      HLL_BYTECODE_CONST,
      0x00,
      0x00,
      // 2
      HLL_BYTECODE_CONST,
      0x00,
      0x01,
      // (c 2)
      HLL_BYTECODE_LET,
      // a
      HLL_BYTECODE_CONST,
      0x00,
      0x02,
      // +
      HLL_BYTECODE_CONST,
      0x00,
      0x03,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      // (c 1)
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_CONST,
      0x00,
      0x00,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      HLL_BYTECODE_APPEND,
      HLL_BYTECODE_CONST,
      0x00,
      0x04,
      HLL_BYTECODE_APPEND,
      HLL_BYTECODE_POP,
      // (+ c 1)
      HLL_BYTECODE_CALL,
      HLL_BYTECODE_LET,
      // (* c a)
      HLL_BYTECODE_CONST,
      0x00,
      0x05,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_CONST,
      0x00,
      0x00,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      HLL_BYTECODE_APPEND,
      HLL_BYTECODE_CONST,
      0x00,
      0x02,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      HLL_BYTECODE_APPEND,
      HLL_BYTECODE_POP,
      HLL_BYTECODE_CALL,
      HLL_BYTECODE_POP,
      // c
      HLL_BYTECODE_CONST,
      0x00,
      0x02,
      HLL_BYTECODE_FIND,
      HLL_BYTECODE_CDR,
      HLL_BYTECODE_POPENV,
      HLL_BYTECODE_END,
  };
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_compiles_setf_symbol(void) {
  char const *source = "(defvar x) (setf x t)";
  uint8_t bytecode[] = {// defvar x
                        HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_NIL,
                        HLL_BYTECODE_LET, HLL_BYTECODE_POP,
                        // setf
                        HLL_BYTECODE_TRUE, HLL_BYTECODE_CONST, 0x00, 0x00,
                        HLL_BYTECODE_FIND, HLL_BYTECODE_SETCDR,
                        HLL_BYTECODE_END};
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_compiles_setf_cdr(void) {
  char const *source = "(defvar x '(1)) (setf (cdr x) '(2))";
  uint8_t bytecode[] = {
      // defvar x
      HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_NIL, HLL_BYTECODE_NIL,
      HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
      HLL_BYTECODE_LET, HLL_BYTECODE_POP,
      // setf
      HLL_BYTECODE_NIL, HLL_BYTECODE_NIL, HLL_BYTECODE_CONST, 0x00, 0x02,
      HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,

      HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_FIND, HLL_BYTECODE_CDR,

      HLL_BYTECODE_SETCDR, HLL_BYTECODE_END};
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

static void test_compiler_basic_special_forms(void) {
  char const *source = "(let ((f (lambda (x) (* x 2))) (y (f 2)))\n"
                       "  (setf f (lambda (x) (f (f x))))\n"
                       "  (defvar l (list (if (= y 4) 1 0) (f 1)))\n"
                       "  (setf (car l) (* 100 (car l))))";
  uint8_t bytecode[] = {HLL_BYTECODE_PUSHENV,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x00, // f
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x01, // x
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x02, // *
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x01, // x
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x03, // 2
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_MAKE_LAMBDA,
                        HLL_BYTECODE_LET,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x04, // y
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x00, // f
                        HLL_BYTECODE_FIND,
                        HLL_BYTECODE_CDR,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x03, // 2
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_CALL,
                        HLL_BYTECODE_LET,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x01, // x
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x00, // f
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x00, // f
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x01, // x
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_MAKE_LAMBDA,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x00, // f
                        HLL_BYTECODE_FIND,
                        HLL_BYTECODE_SETCDR,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x05, // l
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x06, // =
                        HLL_BYTECODE_FIND,
                        HLL_BYTECODE_CDR,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x04, // y
                        HLL_BYTECODE_FIND,
                        HLL_BYTECODE_CDR,
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x07, // 4
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_CALL,
                        HLL_BYTECODE_JN,
                        0x00,
                        0x09,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x08, // 1
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_JN,
                        0x00,
                        0x05,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x09, // 0
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x00, // f
                        HLL_BYTECODE_FIND,
                        HLL_BYTECODE_CDR,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x08, // 1
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_CALL,
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_LET,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x02, // *
                        HLL_BYTECODE_FIND,
                        HLL_BYTECODE_CDR,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_NIL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x0A, // 100
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x05, // l
                        HLL_BYTECODE_FIND,
                        HLL_BYTECODE_CDR,
                        HLL_BYTECODE_CAR,
                        HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP,
                        HLL_BYTECODE_CALL,
                        HLL_BYTECODE_CONST,
                        0x00,
                        0x05, // l
                        HLL_BYTECODE_FIND,
                        HLL_BYTECODE_CDR,
                        HLL_BYTECODE_SETCAR,
                        HLL_BYTECODE_POPENV,
                        HLL_BYTECODE_END};
  hll_vm *vm = hll_make_vm(NULL);

  hll_bytecode *result = hll_compile(vm, source);
  test_bytecode_equals(bytecode, sizeof(bytecode), result);
}

#define TCASE(_name)                                                           \
  { #_name, _name }

TEST_LIST = {TCASE(test_compiler_compiles_integer),
             TCASE(test_compiler_compiles_addition),
             TCASE(test_compiler_compiles_complex_arithmetic_operation),
             TCASE(test_compiler_compiles_if),
             TCASE(test_compiler_compiles_quote),
             TCASE(test_lambda_application_working),
             TCASE(test_compiler_compiles_let),
             TCASE(test_compiler_compiles_let_with_body),
             TCASE(test_compiler_compiles_setf_symbol),
             TCASE(test_compiler_compiles_setf_cdr),
             TCASE(test_compiler_basic_special_forms),
             {NULL, NULL}};
