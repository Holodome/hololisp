#include "../hololisp/hll_bytecode.h"
#include "../hololisp/hll_compiler.h"
#include "../hololisp/hll_mem.h"
#include "../hololisp/hll_value.h"
#include "../hololisp/hll_vm.h"

#define TEST_MSG_MAXSIZE 16384
#include "acutest.h"

static void test_bytecode_equals(uint8_t *expected, size_t expected_len,
                                 struct hll_bytecode *test) {
  TEST_ASSERT(test != NULL);

  TEST_CHECK(expected_len == hll_sb_len(test->ops) &&
             memcmp(expected, test->ops, expected_len) == 0);

  FILE *f = tmpfile();
  hll_dump_bytecode(f, test);
  size_t len = ftell(f);
  rewind(f);

  char b[16384] = {0};
  fread(b, len, 1, f);
  TEST_MSG("got: %s", b);
}

static void test_compiler_compiles_integer(void) {
  const char *source = "1";
  uint8_t bytecode[] = {HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_END};
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_addition(void) {
  const char *source = "(+ 1 2)";
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
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_complex_arithmetic_operation(void) {
  const char *source = "(+ (* 3 5) 2 (/ 2 1))";
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
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
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
  const char *source = "(if t 1 0)";
  uint8_t bytecode[] = {// t
                        HLL_BYTECODE_TRUE, HLL_BYTECODE_JN, 0x00, 0x07,
                        // 1
                        HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_NIL,
                        HLL_BYTECODE_JN, 0x00, 0x03,
                        // 0
                        HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_END};
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_quote(void) {
  const char *source = "'(1 2)";
  uint8_t bytecode[] = {HLL_BYTECODE_NIL, HLL_BYTECODE_NIL,
                        // 1
                        HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_APPEND,
                        // 2
                        HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_APPEND,
                        HLL_BYTECODE_POP, HLL_BYTECODE_END};
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_defun(void) {
  const char *source = "(define (f x) (* x 2))";
  uint8_t function_bytecode[] = {HLL_BYTECODE_CONST,
                                 0x00,
                                 0x00,
                                 HLL_BYTECODE_FIND,
                                 HLL_BYTECODE_CDR, // *

                                 HLL_BYTECODE_NIL,
                                 HLL_BYTECODE_NIL,
                                 HLL_BYTECODE_CONST,
                                 0x00,
                                 0x01, // x
                                 HLL_BYTECODE_FIND,
                                 HLL_BYTECODE_CDR,
                                 HLL_BYTECODE_APPEND,
                                 HLL_BYTECODE_CONST,
                                 0x00,
                                 0x02, // 2
                                 HLL_BYTECODE_APPEND,
                                 HLL_BYTECODE_POP,

                                 HLL_BYTECODE_CALL,
                                 HLL_BYTECODE_END};

  uint8_t program_bytecode[] = {HLL_BYTECODE_CONST,
                                0x00,
                                0x00, // f
                                HLL_BYTECODE_MAKEFUN,
                                0x00,
                                0x01, // function object
                                HLL_BYTECODE_LET,
                                HLL_BYTECODE_END};

  (void)function_bytecode;

  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(program_bytecode, sizeof(program_bytecode), compiled);

  TEST_ASSERT(hll_sb_len(compiled->constant_pool) >= 1);
  hll_value func = compiled->constant_pool[1];
  TEST_ASSERT(hll_get_value_kind(func) == HLL_OBJ_FUNC);

  struct hll_bytecode *function_bytecode_compiled =
      hll_unwrap_func(func)->bytecode;
  test_bytecode_equals(function_bytecode, sizeof(function_bytecode),
                       function_bytecode_compiled);
}

static void test_compiler_compiles_let(void) {
  const char *source = "(let ((c 2) (a (+ c 1))))";
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
      HLL_BYTECODE_POP,
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
      HLL_BYTECODE_POP,
      HLL_BYTECODE_NIL,
      HLL_BYTECODE_POPENV,
      HLL_BYTECODE_END,
  };
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_let_with_body(void) {
  const char *source = "(let ((c 2) (a (+ c 1))) (* c a) a)";
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
      HLL_BYTECODE_POP,
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
      HLL_BYTECODE_POP,
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
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_setf_symbol(void) {
  const char *source = "(define x) (set! x t)";
  uint8_t bytecode[] = {// defvar x
                        HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_NIL,
                        HLL_BYTECODE_LET, HLL_BYTECODE_POP,
                        // set
                        HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_FIND,
                        HLL_BYTECODE_TRUE, HLL_BYTECODE_SETCDR,
                        HLL_BYTECODE_END};
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_setf_cdr(void) {
  const char *source = "(define x '(1)) (set! (cdr x) '(2))";
  uint8_t bytecode[] = {
      // defvar x
      HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_NIL, HLL_BYTECODE_NIL,
      HLL_BYTECODE_CONST, 0x00, 0x01, HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,
      HLL_BYTECODE_LET, HLL_BYTECODE_POP,
      // set
      HLL_BYTECODE_CONST, 0x00, 0x00, HLL_BYTECODE_FIND, HLL_BYTECODE_CDR,
      HLL_BYTECODE_NIL, HLL_BYTECODE_NIL, HLL_BYTECODE_CONST, 0x00, 0x02,
      HLL_BYTECODE_APPEND, HLL_BYTECODE_POP,

      HLL_BYTECODE_SETCDR, HLL_BYTECODE_END};
  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_macro(void) {
  const char *source = "(defmacro (hello) (+ 1 2 3)) (hello)";
  uint8_t bytecode[] = {
      HLL_BYTECODE_NIL, HLL_BYTECODE_POP, HLL_BYTECODE_CONST, 0x00, 0x00,
      HLL_BYTECODE_END};

  struct hll_vm *vm = hll_make_vm(NULL);
  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(bytecode, sizeof(bytecode), compiled);
}

static void test_compiler_compiles_lambda(void) {
  const char *source = "((lambda (x) (+ x x x)) 3)";
  uint8_t function_bytecode[] = {HLL_BYTECODE_CONST,
                                 0x00,
                                 0x00,
                                 HLL_BYTECODE_FIND,
                                 HLL_BYTECODE_CDR, // *

                                 HLL_BYTECODE_NIL,
                                 HLL_BYTECODE_NIL,
                                 HLL_BYTECODE_CONST,
                                 0x00,
                                 0x01, // x
                                 HLL_BYTECODE_FIND,
                                 HLL_BYTECODE_CDR,
                                 HLL_BYTECODE_APPEND,
                                 HLL_BYTECODE_CONST,
                                 0x00,
                                 0x01, // x
                                 HLL_BYTECODE_FIND,
                                 HLL_BYTECODE_CDR,
                                 HLL_BYTECODE_APPEND,
                                 HLL_BYTECODE_CONST,
                                 0x00,
                                 0x01, // x
                                 HLL_BYTECODE_FIND,
                                 HLL_BYTECODE_CDR,
                                 HLL_BYTECODE_APPEND,

                                 HLL_BYTECODE_POP,

                                 HLL_BYTECODE_CALL,
                                 HLL_BYTECODE_END};

  uint8_t program_bytecode[] = {HLL_BYTECODE_MAKEFUN,
                                0x00,
                                0x00, // function object
                                HLL_BYTECODE_NIL,
                                HLL_BYTECODE_NIL,
                                HLL_BYTECODE_CONST,
                                0x00,
                                0x01,
                                HLL_BYTECODE_APPEND,
                                HLL_BYTECODE_POP,
                                HLL_BYTECODE_CALL,
                                HLL_BYTECODE_END};

  (void)function_bytecode;

  struct hll_vm *vm = hll_make_vm(NULL);

  hll_value result;
  bool is_compiled = hll_compile(vm, source, &result);
  TEST_ASSERT(is_compiled);
  struct hll_bytecode *compiled = hll_unwrap_func(result)->bytecode;
  test_bytecode_equals(program_bytecode, sizeof(program_bytecode), compiled);

  TEST_ASSERT(hll_sb_len(compiled->constant_pool) >= 1);
  hll_value func = compiled->constant_pool[0];
  TEST_ASSERT(hll_get_value_kind(func) == HLL_OBJ_FUNC);

  struct hll_bytecode *function_bytecode_compiled =
      hll_unwrap_func(func)->bytecode;
  test_bytecode_equals(function_bytecode, sizeof(function_bytecode),
                       function_bytecode_compiled);
}

#define TCASE(_name)                                                           \
  { #_name, _name }

TEST_LIST = {TCASE(test_compiler_compiles_integer),
             TCASE(test_compiler_compiles_addition),
             TCASE(test_compiler_compiles_complex_arithmetic_operation),
             TCASE(test_compiler_compiles_if),
             TCASE(test_compiler_compiles_quote),
             TCASE(test_compiler_compiles_defun),
             TCASE(test_compiler_compiles_let),
             TCASE(test_compiler_compiles_let_with_body),
             TCASE(test_compiler_compiles_setf_symbol),
             TCASE(test_compiler_compiles_setf_cdr),
             TCASE(test_compiler_compiles_macro),
             TCASE(test_compiler_compiles_lambda),
             {NULL, NULL}};
