#include <check.h>

#include "../src/lexer.h"

START_TEST(test_lexer_accepts_null_and_returns_eof) {
    hll_lexer      lexer  = hll_lexer_create(NULL, NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_EOF);
}
END_TEST

START_TEST(test_lexer_eats_simple_symbol) {
    hll_lexer      lexer  = hll_lexer_create("hello", NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_SYMB);
    ck_assert_int_eq(lexer.token_length, 5);

    result = hll_lexer_eat_peek(&lexer);
    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_EOF);
}
END_TEST

START_TEST(test_lexer_returns_simple_symbol) {
    char           buffer[4096];
    hll_lexer      lexer  = hll_lexer_create("hello", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_SYMB);
    ck_assert_int_eq(lexer.token_length, 5);
    ck_assert_str_eq(lexer.buffer, "hello");

    result = hll_lexer_eat_peek(&lexer);
    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_EOF);
}
END_TEST

START_TEST(test_lexer_eats_string_literal) {
    hll_lexer      lexer  = hll_lexer_create("\"hello world\"", NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_STR);
    ck_assert_int_eq(lexer.token_length, 11);

    result = hll_lexer_eat_peek(&lexer);
    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_EOF);
}
END_TEST

START_TEST(test_lexer_returns_string_literal) {
    char      buffer[4096];
    hll_lexer lexer =
        hll_lexer_create("\"hello world\"", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_STR);
    ck_assert_int_eq(lexer.token_length, 11);
    ck_assert_str_eq(lexer.buffer, "hello world");

    result = hll_lexer_eat_peek(&lexer);
    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_EOF);
}
END_TEST

START_TEST(test_lexer_eats_number) {
    hll_lexer      lexer  = hll_lexer_create("123", NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_SYMB);
    ck_assert_int_eq(lexer.token_length, 3);

    result = hll_lexer_eat_peek(&lexer);
    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_EOF);
}
END_TEST

START_TEST(test_lexer_returns_number) {
    char           buffer[4096];
    hll_lexer      lexer  = hll_lexer_create("123", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_NUMI);
    ck_assert_int_eq(lexer.token_length, 3);
    ck_assert_str_eq(lexer.buffer, "123");
    ck_assert_int_eq(lexer.token_int, 123);

    result = hll_lexer_eat_peek(&lexer);
    ck_assert_int_eq(result, HLL_LEX_OK);
    ck_assert_int_eq(lexer.token_kind, HLL_LTOK_EOF);
}
END_TEST

Suite *
create_lexer_suite(void) {
    Suite *s;
    TCase *tc_core;

    s       = suite_create("lexer");
    tc_core = tcase_create("core");

    tcase_add_test(tc_core, test_lexer_accepts_null_and_returns_eof);
    tcase_add_test(tc_core, test_lexer_eats_string_literal);
    tcase_add_test(tc_core, test_lexer_returns_string_literal);
    tcase_add_test(tc_core, test_lexer_eats_number);
    tcase_add_test(tc_core, test_lexer_returns_number);
    tcase_add_test(tc_core, test_lexer_eats_simple_symbol);
    tcase_add_test(tc_core, test_lexer_returns_simple_symbol);

    suite_add_tcase(s, tc_core);

    return s;
}
