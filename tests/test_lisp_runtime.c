#include "../src/lexer.h"
#include "../src/lisp.h"
#include "../src/lisp_gcs.h"
#include "../src/parser.h"
#include "acutest.h"

static void
test_lisp_print_nil(void) {
    char const *source = "()";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;
    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);

    FILE *out = tmpfile();
    hll_lisp_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "()") == 0);
}

static void
test_lisp_print_number(void) {
    char const *source = "123";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;
    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);

    FILE *out = tmpfile();
    hll_lisp_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "123") == 0);
}

static void
test_lisp_print_symbol(void) {
    char const *source = "hello-world";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;
    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);

    FILE *out = tmpfile();
    hll_lisp_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "hello-world") == 0);
}

static void
test_lisp_print_single_element_list(void) {
    char const *source = "(123)";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;
    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);

    FILE *out = tmpfile();
    hll_lisp_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "(123)") == 0);
}

static void
test_lisp_print_list(void) {
    char const *source = "(+ 1 2)";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;
    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);

    FILE *out = tmpfile();
    hll_lisp_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "(+ 1 2)") == 0);
}

#define TCASE(_name) \
    { #_name, _name }

TEST_LIST = { TCASE(test_lisp_print_nil), TCASE(test_lisp_print_number),
              TCASE(test_lisp_print_symbol),
              TCASE(test_lisp_print_single_element_list),
              TCASE(test_lisp_print_list) };
