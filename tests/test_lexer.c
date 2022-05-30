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

TEST_LIST = {
    { "test_lexer_accepts_null_and_returns_eof",
      test_lexer_accepts_null_and_returns_eof },
    { "test_lexer_eats_simple_symbol", test_lexer_eats_simple_symbol },
    { "test_lexer_returns_simple_symbol", test_lexer_returns_simple_symbol },
    { "test_lexer_eats_number", test_lexer_eats_number },
    { "test_lexer_returns_number", test_lexer_returns_number },

    { NULL, NULL }
};
