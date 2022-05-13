#include "lexer.c"
#include "unicode.c"

#include "test.h"

#include <stdio.h>
#include <assert.h>

static char const *
test_lexer_returns_eof_on_empty() {
    hll_lexer lexer = hll_lexer_init(NULL, NULL, 0);
    hll_lex_result result = hll_lexer_token(&lexer);

    hlt_assert(result == HLL_LOK);
    hlt_assert(lexer.token_kind == HLL_LTOK_EOF);

    return 0;
}

char const *
all_tests() {
    hlt_run_test(test_lexer_returns_eof_on_empty);

    return 0;
}
