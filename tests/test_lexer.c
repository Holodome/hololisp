#include "../hololisp/hll_compiler.h"
#include "acutest.h"

static void test_lexer_parses_simple_symbol(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, "hello", &tu);
  hll_lexer_next(&lexer);

  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 5);
  TEST_ASSERT(strncmp(lexer.input + lexer.next.offset, "hello",
                      lexer.next.length) == 0);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_parses_number(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, "123", &tu);
  hll_lexer_next(&lexer);

  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_NUM);
  TEST_ASSERT(lexer.next.length == 3);
  TEST_ASSERT(lexer.next.value = 123);
  TEST_ASSERT(
      strncmp(lexer.input + lexer.next.offset, "123", lexer.next.length) == 0);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_parse_basic_syntax_multiple_tokens(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, "(cons -6 +7) (wha-te!ver . '-)", &tu);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_LPAREN);

  hll_lexer_next(&lexer);

  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 4);
  TEST_ASSERT(
      strncmp(lexer.input + lexer.next.offset, "cons", lexer.next.length) == 0);

  hll_lexer_next(&lexer);

  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_NUM);
  TEST_ASSERT(lexer.next.length == 2);
  TEST_ASSERT(
      strncmp(lexer.input + lexer.next.offset, "-6", lexer.next.length) == 0);
  TEST_ASSERT(lexer.next.value == -6);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_NUM);
  TEST_ASSERT(lexer.next.length == 2);
  TEST_ASSERT(
      strncmp(lexer.input + lexer.next.offset, "+7", lexer.next.length) == 0);
  TEST_ASSERT(lexer.next.value == 7);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_RPAREN);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_LPAREN);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 10);
  TEST_ASSERT(strncmp(lexer.input + lexer.next.offset, "wha-te!ver",
                      lexer.next.length) == 0);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_DOT);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_QUOTE);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 1);
  TEST_ASSERT(
      strncmp(lexer.input + lexer.next.offset, "-", lexer.next.length) == 0);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_RPAREN);

  hll_lexer_next(&lexer);

  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_skips_whitespace_symbols(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, " \f\n\r\t\vhello", &tu);
  hll_lexer_next(&lexer);

  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_dont_think_that_plus_and_minus_are_numbers(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, "+ - +- -+", &tu);
  hll_lexer_next(&lexer);

  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_parses_comments(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  const char *data = "hello ; this is comment\n"
                     "world ; this is comment too";
  hll_lexer lexer;
  hll_lexer_init(&lexer, data, &tu);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_COMMENT);
  TEST_ASSERT(strncmp(lexer.input + lexer.next.offset, "; this is comment",
                      lexer.next.length) == 0);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_COMMENT);
  TEST_ASSERT(strncmp(lexer.input + lexer.next.offset, "; this is comment too",
                      lexer.next.length) == 0);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_parses_dot(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, ".123 . 123.", &tu);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 4);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_DOT);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 4);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_parses_lparen(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, "(abc ( bca(", &tu);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_LPAREN);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 3);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_LPAREN);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 3);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_LPAREN);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_parses_rparen(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, ")abc ) bca)", &tu);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_RPAREN);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 3);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_RPAREN);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);
  TEST_ASSERT(lexer.next.length == 3);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_RPAREN);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_reports_symbol_consisting_of_only_dots(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, "...", &tu);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_parses_quote(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, "'abc", &tu);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_QUOTE);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_SYMB);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count == 0);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

static void test_lexer_reports_too_big_integer(void) {
  struct hll_vm *vm = hll_make_vm(NULL);
  hll_translation_unit tu = hll_make_tu(vm, NULL, NULL, 0);
  hll_lexer lexer;
  hll_lexer_init(&lexer, "11111111111111111111111111111111111111111111111111",
                 &tu);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_NUM);

  hll_lexer_next(&lexer);
  TEST_ASSERT(lexer.error_count);
  TEST_ASSERT(lexer.next.kind == HLL_TOK_EOF);
}

#define TCASE(_name)                                                           \
  {                                                                            \
#_name, _name                                                              \
  }

TEST_LIST = {TCASE(test_lexer_parses_simple_symbol),
             TCASE(test_lexer_parses_number),
             TCASE(test_lexer_parse_basic_syntax_multiple_tokens),
             TCASE(test_lexer_parses_comments),
             TCASE(test_lexer_dont_think_that_plus_and_minus_are_numbers),
             TCASE(test_lexer_skips_whitespace_symbols),
             TCASE(test_lexer_parses_dot),
             TCASE(test_lexer_parses_lparen),
             TCASE(test_lexer_parses_rparen),
             TCASE(test_lexer_reports_symbol_consisting_of_only_dots),
             TCASE(test_lexer_parses_quote),
             TCASE(test_lexer_reports_too_big_integer),

             {NULL, NULL}};
