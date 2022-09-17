#include "../hololisp/hll_compiler.h"
#include "../hololisp/hll_obj.h"
#include "../hololisp/hll_vm.h"

#include "acutest.h"

static void test_reader_reports_eof(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;
  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_NIL);
}

static void test_reader_parses_num(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "123", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);

  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_NUM);
  TEST_ASSERT(hll_unwrap_num(ast) == 123);
}

static void test_reader_parses_symbol(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "hello-world", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_SYMB);
  TEST_ASSERT(strcmp(hll_unwrap_zsymb(ast), "hello-world") == 0);
}

static void test_reader_parses_one_element_list(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "(100)", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_NUM);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
}

static void test_reader_parses_list(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "(100 -100 abc)", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_NUM);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_OBJ_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_NUM);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_OBJ_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
}

static void test_reader_parses_nested_lists(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "(+ (* 3 2) hello)", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);

  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_CONS);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_CONS);
  {
    hll_value inner = hll_unwrap_car(ast);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(inner)) == HLL_OBJ_SYMB);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(inner)) == HLL_OBJ_CONS);
    inner = hll_unwrap_cdr(inner);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(inner)) == HLL_OBJ_NUM);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(inner)) == HLL_OBJ_CONS);
    inner = hll_unwrap_cdr(inner);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(inner)) == HLL_OBJ_NUM);
    TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(inner)) == HLL_OBJ_NIL);
  }
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
}

static void test_reader_reports_unclosed_list(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "(", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  (void)ast;
  TEST_ASSERT(reader.error_count);
}

static void test_reader_reports_stray_rparen(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, ")", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  (void)ast;
  TEST_ASSERT(reader.error_count);
}

static void test_reader_parses_nil(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "()", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_NIL);
}

static void test_reader_parses_simple_dotted_cons(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "(abc . 123)", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NUM);
}

static void test_reader_parses_dotted_list(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "(a b c . 123)", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_OBJ_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_OBJ_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NUM);
}

static void test_reader_parses_quote(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  vm->forbid_gc = 1;

  struct hll_lexer lexer;
  hll_lexer_init(&lexer, "'1", vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  hll_value ast = hll_read_ast(&reader);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
  ast = hll_unwrap_car(ast);
  TEST_ASSERT(hll_get_value_kind(ast) == HLL_OBJ_CONS);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_SYMB);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) != HLL_OBJ_NIL);
  ast = hll_unwrap_cdr(ast);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_car(ast)) == HLL_OBJ_NUM);
  TEST_ASSERT(hll_get_value_kind(hll_unwrap_cdr(ast)) == HLL_OBJ_NIL);
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
