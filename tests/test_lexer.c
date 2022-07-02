#include "../hololisp/hll_lexer.h"
#include "acutest.h"

static void
test_lexer_accepts_null_and_returns_eof(void) {
    hll_lexer lexer = hll_lexer_create(NULL, NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_eats_simple_symbol(void) {
    hll_lexer lexer = hll_lexer_create("hello", NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 5);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_returns_simple_symbol(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("hello", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 5);
    TEST_ASSERT(strcmp(lexer.buffer, "hello") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_eats_number(void) {
    hll_lexer lexer = hll_lexer_create("123", NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_NUMI);
    TEST_ASSERT(lexer.token_length == 3);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_returns_number(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("123", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_NUMI);
    TEST_ASSERT(lexer.token_length == 3);
    TEST_ASSERT(strcmp(lexer.buffer, "123") == 0);
    TEST_ASSERT(lexer.token_int == 123);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_parse_basic_syntax_multiple_tokens(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("(cons -6 +7) (wha-te!ver . '-)", buffer,
                                       sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_LPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 4);
    TEST_ASSERT(strcmp(lexer.buffer, "cons") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_NUMI);
    TEST_ASSERT(lexer.token_length == 2);
    TEST_ASSERT(strcmp(lexer.buffer, "-6") == 0);
    TEST_ASSERT(lexer.token_int == -6);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_NUMI);
    TEST_ASSERT(lexer.token_length == 2);
    TEST_ASSERT(strcmp(lexer.buffer, "+7") == 0);
    TEST_ASSERT(lexer.token_int == 7);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_RPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_LPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 10);
    TEST_ASSERT(strcmp(lexer.buffer, "wha-te!ver") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_DOT);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_QUOTE);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 1);
    TEST_ASSERT(strcmp(lexer.buffer, "-") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_RPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_eats_and_peeks(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("123 abc", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_NUMI);
    // Nothing should change
    result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_NUMI);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    // Nothing should change
    result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
    // Nothing should change
    result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
    // The same way, eating EOF should do nothing
    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_skips_whitespace_symbols(void) {
    char buffer[4096];
    hll_lexer lexer =
        hll_lexer_create(" \f\n\r\t\vhello", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_dont_think_that_plus_and_minus_are_numbers(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("+ - +- -+", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_handles_buffer_overflow(void) {
    char buffer[5];

    hll_lexer lexer = hll_lexer_create("hello-world", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_BUF_OVERFLOW);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 11);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_skips_comments(void) {
    char buffer[4096];
    char const *data =
        "hello ; this is comment\n"
        "world ; this is comment too\n";

    hll_lexer lexer = hll_lexer_create(data, buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_parses_dot(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create(".123 . 123.", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 4);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_DOT);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 4);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_parses_lparen(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("(abc ( bca(", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_LPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 3);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_LPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 3);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_LPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_parses_rparen(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create(")abc ) bca)", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_RPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 3);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_RPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);
    TEST_ASSERT(lexer.token_length == 3);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_RPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_reports_symbol_consisting_of_only_dots(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("...", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_ALL_DOT_SYMB);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_DOT);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_parses_quote(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("'abc", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_QUOTE);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_SYMB);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_stores_error_code_from_last_peek(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create("...", buffer, sizeof(buffer));

    hll_lex_result result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_ALL_DOT_SYMB);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_DOT);

    result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_ALL_DOT_SYMB);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_DOT);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_reports_correct_column_numbers(void) {
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create(" (1 2 ) ", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 0);
    TEST_ASSERT(lexer.token_column == 1);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 0);
    TEST_ASSERT(lexer.token_column == 2);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 0);
    TEST_ASSERT(lexer.token_column == 4);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 0);
    TEST_ASSERT(lexer.token_column == 6);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
    TEST_ASSERT(lexer.token_line == 0);
    TEST_ASSERT(lexer.token_column == 8);
}

static void
test_lexer_reports_correct_line_numbers_at_their_start(void) {
    char buffer[4096];
    hll_lexer lexer =
        hll_lexer_create("0\n1\n\n3\n\n\n6\n\n\n\n", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 0);
    TEST_ASSERT(lexer.token_column == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 1);
    TEST_ASSERT(lexer.token_column == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 3);
    TEST_ASSERT(lexer.token_column == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 6);
    TEST_ASSERT(lexer.token_column == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
    TEST_ASSERT(lexer.token_line == 10);
    TEST_ASSERT(lexer.token_column == 0);
}

static void
test_lexer_reports_correct_columns_on_multiline_string(void) {
    char const *source =
        "hello world\n"
        "1 2 3\n"
        "\n"
        "\n"
        " 1 2 3 \n";
    char buffer[4096];
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 0);
    TEST_ASSERT(lexer.token_column == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 0);
    TEST_ASSERT(lexer.token_column == 6);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 1);
    TEST_ASSERT(lexer.token_column == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 1);
    TEST_ASSERT(lexer.token_column == 2);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 1);
    TEST_ASSERT(lexer.token_column == 4);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 4);
    TEST_ASSERT(lexer.token_column == 1);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 4);
    TEST_ASSERT(lexer.token_column == 3);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_line == 4);
    TEST_ASSERT(lexer.token_column == 5);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
    TEST_ASSERT(lexer.token_line == 5);
    TEST_ASSERT(lexer.token_column == 0);
}

static void
test_lexer_reports_too_big_integer(void) {
    char buffer[4096];
    hll_lexer lexer =
        hll_lexer_create("11111111111111111111111111111111111111111111111111",
                         buffer, sizeof(buffer));

    hll_lex_result result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_INT_TOO_BIG);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_NUMI);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_ext_eats_comment(void) {
    hll_lexer lexer = hll_lexer_create("; Hello world\n", NULL, 0);
    lexer.is_ext = 1;

    hll_lex_result result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EXT_COMMENT);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

static void
test_lexer_ext_returns_comment(void) {
    char buffer[4096];
    hll_lexer lexer =
        hll_lexer_create("; Hello world\n", buffer, sizeof(buffer));
    lexer.is_ext = 1;

    hll_lex_result result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EXT_COMMENT);
    TEST_ASSERT(strcmp(lexer.buffer, " Hello world") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_TOK_EOF);
}

#define TCASE(_name) \
    { #_name, _name }

TEST_LIST = { TCASE(test_lexer_accepts_null_and_returns_eof),
              TCASE(test_lexer_eats_simple_symbol),
              TCASE(test_lexer_returns_simple_symbol),
              TCASE(test_lexer_eats_number),
              TCASE(test_lexer_returns_number),
              TCASE(test_lexer_parse_basic_syntax_multiple_tokens),
              TCASE(test_lexer_eats_and_peeks),
              TCASE(test_lexer_skips_comments),
              TCASE(test_lexer_handles_buffer_overflow),
              TCASE(test_lexer_dont_think_that_plus_and_minus_are_numbers),
              TCASE(test_lexer_skips_whitespace_symbols),
              TCASE(test_lexer_parses_dot),
              TCASE(test_lexer_parses_lparen),
              TCASE(test_lexer_parses_rparen),
              TCASE(test_lexer_reports_symbol_consisting_of_only_dots),
              TCASE(test_lexer_parses_quote),
              TCASE(test_lexer_reports_correct_column_numbers),
              TCASE(test_lexer_stores_error_code_from_last_peek),
              TCASE(test_lexer_reports_correct_line_numbers_at_their_start),
              TCASE(test_lexer_reports_correct_columns_on_multiline_string),
              TCASE(test_lexer_reports_too_big_integer),
              TCASE(test_lexer_ext_eats_comment),
              TCASE(test_lexer_ext_returns_comment),

              { NULL, NULL } };
