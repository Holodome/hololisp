#include "acutest.h"
#include "lexer.h"

void
test_lexer_accepts_null_and_returns_eof() {
    hll_lexer      lexer  = hll_lexer_create(NULL, NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
}

void
test_lexer_eats_simple_symbol() {
    hll_lexer      lexer  = hll_lexer_create("hello", NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_SYMB);
    TEST_ASSERT(lexer.token_length == 5);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
}

void
test_lexer_returns_simple_symbol(void) {
    char           buffer[4096];
    hll_lexer      lexer  = hll_lexer_create("hello", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_SYMB);
    TEST_ASSERT(lexer.token_length == 5);
    TEST_ASSERT(strcmp(lexer.buffer, "hello") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
}

#if 0
void
test_lexer_eats_string_literal(void) {
    hll_lexer      lexer  = hll_lexer_create("\"hello world\"", NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_STR);
    TEST_ASSERT(lexer.token_length == 11);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
}

void
test_lexer_returns_string_literal(void) {
    char      buffer[4096];
    hll_lexer lexer =
        hll_lexer_create("\"hello world\"", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_STR);
    TEST_ASSERT(lexer.token_length == 11);
    TEST_ASSERT(strcmp(lexer.buffer, "hello world") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
}
#endif

void
test_lexer_eats_number(void) {
    hll_lexer      lexer  = hll_lexer_create("123", NULL, 0);
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_SYMB);
    TEST_ASSERT(lexer.token_length == 3);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
}

void
test_lexer_returns_number(void) {
    char           buffer[4096];
    hll_lexer      lexer  = hll_lexer_create("123", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);

    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_NUMI);
    TEST_ASSERT(lexer.token_length == 3);
    TEST_ASSERT(strcmp(lexer.buffer, "123") == 0);
    TEST_ASSERT(lexer.token_int == 123);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
}

void
test_lexer_parses_character_tokens(void) {
    char const     *chars[]       = { ".", "(", ")", "'" };
    hll_ltoken_kind token_kinds[] = { HLL_LTOK_DOT, HLL_LTOK_LPAREN,
                                      HLL_LTOK_RPAREN, HLL_LTOK_QUOTE };
    for (size_t i = 0; i < sizeof(chars) / sizeof(*chars); ++i) {
        hll_lexer      lexer  = hll_lexer_create(chars[i], NULL, 0);
        hll_lex_result result = hll_lexer_peek(&lexer);

        TEST_ASSERT(result == HLL_LEX_OK);
        TEST_ASSERT(lexer.token_kind == token_kinds[i]);

        result = hll_lexer_eat_peek(&lexer);
        TEST_ASSERT(result == HLL_LEX_OK);
        TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
    }
}

void
test_lexer_parse_basic_syntax_multiple_tokens(void) {
    char           buffer[4096];
    hll_lexer      lexer  = hll_lexer_create("(cons -6 +7) (wha-te#ver . '-)", buffer, sizeof(buffer));
    hll_lex_result result = hll_lexer_peek(&lexer);
    
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_LPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_SYMB);
    TEST_ASSERT(lexer.token_length == 4);
    TEST_ASSERT(strcmp(lexer.buffer, "cons") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_NUMI);
    TEST_ASSERT(lexer.token_length == 2);
    TEST_ASSERT(strcmp(lexer.buffer, "-6") == 0);
    TEST_ASSERT(lexer.token_int == -6);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_NUMI);
    TEST_ASSERT(lexer.token_length == 2);
    TEST_ASSERT(strcmp(lexer.buffer, "+7") == 0);
    TEST_ASSERT(lexer.token_int == 7);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_RPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_LPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_SYMB);
    TEST_ASSERT(lexer.token_length == 10);
    TEST_ASSERT(strcmp(lexer.buffer, "wha-te#ver") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_DOT);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_QUOTE);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_SYMB);
    TEST_ASSERT(lexer.token_length == 1);
    TEST_ASSERT(strcmp(lexer.buffer, "-") == 0);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_RPAREN);

    result = hll_lexer_eat_peek(&lexer);
    TEST_ASSERT(result == HLL_LEX_OK);
    TEST_ASSERT(lexer.token_kind == HLL_LTOK_EOF);
}

TEST_LIST = {
    { "test_lexer_accepts_null_and_returns_eof",
      test_lexer_accepts_null_and_returns_eof },
    { "test_lexer_eats_simple_symbol", test_lexer_eats_simple_symbol },
    { "test_lexer_returns_simple_symbol", test_lexer_returns_simple_symbol },
    { "test_lexer_eats_number", test_lexer_eats_number },
    { "test_lexer_returns_number", test_lexer_returns_number },
    { "test_lexer_parses_character_tokens",
      test_lexer_parses_character_tokens },
    { "test_lexer_parse_basic_syntax_multiple_tokens", test_lexer_parse_basic_syntax_multiple_tokens },

    { NULL, NULL }
};
