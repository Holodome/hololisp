#include "../src/lexer.h"
#include "../src/lisp.h"
#include "../src/lisp_gcs.h"
#include "../src/parser.h"
#include "acutest.h"

static void
test_parser_reports_eof(void) {
    char const *source = "";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;
    hll_parse_result result = hll_parse(&parser, &obj);

    TEST_ASSERT(result == HLL_PARSE_EOF);
    TEST_ASSERT(obj == NULL);
}

static void
test_parser_parses_num(void) {
    char const *source = "123";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;

    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_LOBJ_INT);
    TEST_ASSERT(hll_unwrap_int(obj)->value == 123);

    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
}

static void
test_parser_parses_symbol(void) {
    char const *source = "hello-world";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;

    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_LOBJ_SYMB);
    TEST_ASSERT(strcmp(hll_unwrap_symb(obj)->symb, source) == 0);

    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
}

static void
test_parser_parses_one_element_list(void) {
    char const *source = "(100)";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;

    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
    hll_lisp_cons *cons = hll_unwrap_cons(obj);
    TEST_ASSERT(cons->cdr == hll_nil);

    hll_lisp_obj_head *car = cons->car;
    TEST_ASSERT(car->kind == HLL_LOBJ_INT);
    TEST_ASSERT(hll_unwrap_int(car)->value == 100);

    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
}

static void
test_parser_parses_list(void) {
    char const *source = "(-100 100 abc)";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;

    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
    hll_lisp_cons *cons = hll_unwrap_cons(obj);

    hll_lisp_obj_head *car = cons->car;
    TEST_ASSERT(car->kind == HLL_LOBJ_INT);
    TEST_ASSERT(hll_unwrap_int(car)->value == -100);

    TEST_ASSERT(cons->cdr != hll_nil);
    obj = cons->cdr;
    TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
    cons = hll_unwrap_cons(obj);

    car = cons->car;
    TEST_ASSERT(car->kind == HLL_LOBJ_INT);
    TEST_ASSERT(hll_unwrap_int(car)->value == 100);

    TEST_ASSERT(cons->cdr != hll_nil);
    obj = cons->cdr;
    TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
    cons = hll_unwrap_cons(obj);

    car = cons->car;
    TEST_ASSERT(car->kind == HLL_LOBJ_SYMB);
    TEST_ASSERT(strcmp(hll_unwrap_symb(car)->symb, "abc") == 0);

    TEST_ASSERT(cons->cdr == hll_nil);

    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
}

static void
test_parser_parses_nested_lists(void) {
    char const *source = "(+ (* 3 2) hello)";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;

    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_OK);
    TEST_ASSERT(obj != NULL);
    {
        TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
        hll_lisp_cons *cons = hll_unwrap_cons(obj);
        hll_lisp_obj_head *car = cons->car;
        TEST_ASSERT(car->kind == HLL_LOBJ_SYMB);
        TEST_ASSERT(strcmp(hll_unwrap_symb(car)->symb, "+") == 0);

        obj = cons->cdr;
        TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
        cons = hll_unwrap_cons(obj);
        car = cons->car;

        hll_lisp_cons *old_cons = cons;

        {
            TEST_ASSERT(car->kind == HLL_LOBJ_CONS);
            cons = hll_unwrap_cons(car);

            car = cons->car;
            TEST_ASSERT(car->kind == HLL_LOBJ_SYMB);
            TEST_ASSERT(strcmp(hll_unwrap_symb(car)->symb, "*") == 0);

            TEST_ASSERT(cons->cdr != hll_nil);
            obj = cons->cdr;
            TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
            cons = hll_unwrap_cons(obj);

            car = cons->car;
            TEST_ASSERT(car->kind == HLL_LOBJ_INT);
            TEST_ASSERT(hll_unwrap_int(car)->value == 3);

            TEST_ASSERT(cons->cdr != hll_nil);
            obj = cons->cdr;
            TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
            cons = hll_unwrap_cons(obj);

            car = cons->car;
            TEST_ASSERT(car->kind == HLL_LOBJ_INT);
            TEST_ASSERT(hll_unwrap_int(car)->value == 2);

            TEST_ASSERT(cons->cdr == hll_nil);
        }

        cons = old_cons;
        TEST_ASSERT(cons->cdr != hll_nil);
        obj = cons->cdr;
        TEST_ASSERT(obj->kind == HLL_LOBJ_CONS);
        cons = hll_unwrap_cons(obj);

        car = cons->car;
        TEST_ASSERT(car->kind == HLL_LOBJ_SYMB);
        TEST_ASSERT(strcmp(hll_unwrap_symb(car)->symb, "hello") == 0);

        TEST_ASSERT(cons->cdr == hll_nil);
    }

    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
}

static void
test_parser_reports_unclosed_list(void) {
    char const *source = "(";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_lisp_obj_head *obj = NULL;
    hll_parse_result result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_MISSING_RPAREN);
    TEST_ASSERT(obj == NULL);

    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
}

static void
test_parser_returns_eof_arbitrary_amount_of_times(void) {
    char const *source = "";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_parse_result result;
    hll_lisp_obj_head *obj;

    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_EOF);
}

static void 
test_parser_reports_stary_rparen(void) {
    char const *source = ")";
    char buffer[4096];

    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_parser parser = hll_parser_create(&lexer, &ctx);

    hll_parse_result result;
    hll_lisp_obj_head *obj;

    result = hll_parse(&parser, &obj);
    TEST_ASSERT(result == HLL_PARSE_UNEXPECTED_TOKEN);
}

#if 0
static void
test_parser_parses_quoted_list(void) {
}

static void
test_parser_parses_quoted_symbol(void) {
    TEST_ASSERT(0);
}

static void
test_parser_parses_quoted_int(void) {
    TEST_ASSERT(0);
}
#endif

#define TCASE(_name) \
    { #_name, _name }

TEST_LIST = { TCASE(test_parser_reports_eof),
              TCASE(test_parser_parses_num),
              TCASE(test_parser_parses_symbol),
              /* TCASE(test_parser_parses_quoted_symbol), */
              /* TCASE(test_parser_parses_quoted_int), */
              /* TCASE(test_parser_parses_quoted_list), */
              TCASE(test_parser_parses_one_element_list),
              TCASE(test_parser_parses_list),
              TCASE(test_parser_parses_nested_lists),
              TCASE(test_parser_reports_unclosed_list),
              TCASE(test_parser_returns_eof_arbitrary_amount_of_times),
              TCASE(test_parser_reports_stary_rparen),
              { NULL, NULL } };
