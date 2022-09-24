#include "../hololisp/hll_compiler.h"
#include "../hololisp/hll_value.h"
#include "../hololisp/hll_vm.h"

#include "acutest.h"

static void test_reader_reports_eof(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;
  hll_lexer lexer;
  hll_lexer_init(&lexer, "", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_NIL);
}

static void test_reader_parses_num(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "123", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);

  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_NUM);
  TEST_ASSERT(hll_unwrap_num(ast) == 123);
}

static void test_reader_parses_symbol(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "hello-world", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_SYMB);
  TEST_ASSERT(strcmp(hll_unwrap_zsymb(ast), "hello-world") == 0);
}

static void test_reader_parses_one_element_list(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "(100)", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_NUM);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
}

static void test_reader_parses_list(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "(100 -100 abc)", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_NUM);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_VALUE_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_NUM);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_VALUE_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
}

static void test_reader_parses_nested_lists(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "(+ (* 3 2) hello)", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);

  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_CONS);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_CONS);
  {
    hll_value inner = hll_unwrap_car(ast);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(inner)) == HLL_VALUE_SYMB);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(inner)) == HLL_VALUE_CONS);
    inner = hll_unwrap_cdr(inner);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(inner)) == HLL_VALUE_NUM);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(inner)) == HLL_VALUE_CONS);
    inner = hll_unwrap_cdr(inner);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(inner)) == HLL_VALUE_NUM);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(inner)) == HLL_VALUE_NIL);
  }
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
}

static void test_reader_reports_unclosed_list(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "(", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  (void)ast;
  TEST_ASSERT(reader.error_count);
}

static void test_reader_reports_stray_rparen(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, ")", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  (void)ast;
  TEST_ASSERT(reader.error_count);
}

static void test_reader_parses_nil(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "()", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_NIL);
}

static void test_reader_parses_simple_dotted_cons(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "(abc . 123)", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NUM);
}

static void test_reader_parses_dotted_list(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "(a b c . 123)", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_VALUE_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_VALUE_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NUM);
}

static void test_reader_parses_quote(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  hll_lexer lexer;
  hll_lexer_init(&lexer, "'1", vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, NULL);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_VALUE_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_VALUE_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_VALUE_NUM);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_VALUE_NIL);
}

#define TCASE(_name)                                                           \
  { #_name, _name }

TEST_LIST = {TCASE(test_reader_reports_eof),
             TCASE(test_reader_parses_num),
             TCASE(test_reader_parses_symbol),
             TCASE(test_reader_parses_one_element_list),
             TCASE(test_reader_parses_list),
             TCASE(test_reader_parses_nested_lists),
             TCASE(test_reader_reports_unclosed_list),
             TCASE(test_reader_reports_stray_rparen),
             TCASE(test_reader_parses_nil),
             TCASE(test_reader_parses_simple_dotted_cons),
             TCASE(test_reader_parses_dotted_list),
             TCASE(test_reader_parses_quote),
             {NULL, NULL}};
