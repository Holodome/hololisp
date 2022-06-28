#include "../src/error_reporter.h"
#include "../src/lexer.h"
#include "../src/lisp.h"
#include "../src/reader.h"
#include "../src/lisp_gc.h"
#include "acutest.h"

static void
test_reader_reports_eof(void) {
    char const *source = "";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);

    TEST_ASSERT(result == HLL_READ_EOF);
    TEST_ASSERT(obj == NULL);
}

static void
test_reader_parses_num(void) {
    char const *source = "123";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;

    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(obj)->value == 123);

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
}

static void
test_reader_parses_symbol(void) {
    char const *source = "hello-world";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;

    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_OBJ_SYMB);
    TEST_ASSERT(strcmp(hll_unwrap_symb(obj)->symb, source) == 0);

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
}

static void
test_reader_parses_one_element_list(void) {
    char const *source = "(100)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;

    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
    hll_cons *cons = hll_unwrap_cons(obj);
    TEST_ASSERT(cons->cdr->kind == HLL_OBJ_NIL);

    hll_obj *car = cons->car;
    TEST_ASSERT(car->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(car)->value == 100);

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
}

static void
test_reader_parses_list(void) {
    char const *source = "(-100 100 abc)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;

    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
    hll_cons *cons = hll_unwrap_cons(obj);

    hll_obj *car = cons->car;
    TEST_ASSERT(car->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(car)->value == -100);

    TEST_ASSERT(cons->cdr->kind != HLL_OBJ_NIL);
    obj = cons->cdr;
    TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
    cons = hll_unwrap_cons(obj);

    car = cons->car;
    TEST_ASSERT(car->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(car)->value == 100);

    TEST_ASSERT(cons->cdr->kind != HLL_OBJ_NIL);
    obj = cons->cdr;
    TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
    cons = hll_unwrap_cons(obj);

    car = cons->car;
    TEST_ASSERT(car->kind == HLL_OBJ_SYMB);
    TEST_ASSERT(strcmp(hll_unwrap_symb(car)->symb, "abc") == 0);

    TEST_ASSERT(cons->cdr->kind == HLL_OBJ_NIL);

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
}

static void
test_reader_parses_nested_lists(void) {
    char const *source = "(+ (* 3 2) hello)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;

    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj != NULL);
    {
        TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
        hll_cons *cons = hll_unwrap_cons(obj);
        hll_obj *car = cons->car;
        TEST_ASSERT(car->kind == HLL_OBJ_SYMB);
        TEST_ASSERT(strcmp(hll_unwrap_symb(car)->symb, "+") == 0);

        obj = cons->cdr;
        TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
        cons = hll_unwrap_cons(obj);
        car = cons->car;

        hll_cons *old_cons = cons;

        {
            TEST_ASSERT(car->kind == HLL_OBJ_CONS);
            cons = hll_unwrap_cons(car);

            car = cons->car;
            TEST_ASSERT(car->kind == HLL_OBJ_SYMB);
            TEST_ASSERT(strcmp(hll_unwrap_symb(car)->symb, "*") == 0);

            TEST_ASSERT(cons->cdr->kind != HLL_OBJ_NIL);

            obj = cons->cdr;
            TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
            cons = hll_unwrap_cons(obj);

            car = cons->car;
            TEST_ASSERT(car->kind == HLL_OBJ_INT);
            TEST_ASSERT(hll_unwrap_int(car)->value == 3);

            TEST_ASSERT(cons->cdr->kind != HLL_OBJ_NIL);
            obj = cons->cdr;
            TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
            cons = hll_unwrap_cons(obj);

            car = cons->car;
            TEST_ASSERT(car->kind == HLL_OBJ_INT);
            TEST_ASSERT(hll_unwrap_int(car)->value == 2);

            TEST_ASSERT(cons->cdr->kind == HLL_OBJ_NIL);
        }

        cons = old_cons;
        TEST_ASSERT(cons->cdr->kind != HLL_OBJ_NIL);
        obj = cons->cdr;
        TEST_ASSERT(obj->kind == HLL_OBJ_CONS);
        cons = hll_unwrap_cons(obj);

        car = cons->car;
        TEST_ASSERT(car->kind == HLL_OBJ_SYMB);
        TEST_ASSERT(strcmp(hll_unwrap_symb(car)->symb, "hello") == 0);

        TEST_ASSERT(cons->cdr->kind == HLL_OBJ_NIL);
    }

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
}

static void
test_reader_reports_unclosed_list(void) {
    char const *source = "(";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_MISSING_RPAREN);
    TEST_ASSERT(obj == NULL);

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
}

static void
test_reader_returns_eof_arbitrary_amount_of_times(void) {
    char const *source = "";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_read_result result;
    hll_obj *obj;

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_EOF);
}

static void
test_reader_reports_stary_rparen(void) {
    char const *source = ")";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_read_result result;
    hll_obj *obj;

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_UNEXPECTED_TOKEN);
}

static void
test_reader_parses_nil(void) {
    char const *source = "()";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_read_result result;
    hll_obj *obj;

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_OBJ_NIL);
}

static void
test_reader_parses_simple_dotted_cons(void) {
    char const *source = "(abc . 123)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_read_result result;
    hll_obj *obj;

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj->kind == HLL_OBJ_CONS);

    hll_cons *cons = hll_unwrap_cons(obj);
    TEST_ASSERT(cons->car->kind == HLL_OBJ_SYMB);
    TEST_ASSERT(cons->cdr->kind == HLL_OBJ_INT);
}

static void
test_reader_parses_dotted_list(void) {
    char const *source = "(a b c . 123)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_read_result result;
    hll_obj *obj;

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj->kind == HLL_OBJ_CONS);

    hll_cons *cons = hll_unwrap_cons(obj);
    TEST_ASSERT(cons->car->kind == HLL_OBJ_SYMB);
    cons = hll_unwrap_cons(cons->cdr);
    TEST_ASSERT(cons->car->kind == HLL_OBJ_SYMB);
    cons = hll_unwrap_cons(cons->cdr);
    TEST_ASSERT(cons->car->kind == HLL_OBJ_SYMB);
    TEST_ASSERT(cons->cdr->kind == HLL_OBJ_INT);
}

static void
test_reader_parses_quote(void) {
    char const *source = "'1";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_read_result result;
    hll_obj *obj;

    result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_OBJ_CONS);

    hll_cons *cons = hll_unwrap_cons(obj);
    TEST_ASSERT(cons->car->kind == HLL_OBJ_SYMB);
    TEST_ASSERT(strcmp(hll_unwrap_symb(cons->car)->symb, "quote") == 0);

    TEST_ASSERT(cons->cdr->kind == HLL_OBJ_CONS);
    cons = hll_unwrap_cons(cons->cdr);
    TEST_ASSERT(cons->car->kind == HLL_OBJ_INT);
    TEST_ASSERT(cons->cdr->kind == HLL_OBJ_NIL);
}

#define TCASE(_name) \
    { #_name, _name }

TEST_LIST = { TCASE(test_reader_reports_eof),
              TCASE(test_reader_parses_num),
              TCASE(test_reader_parses_symbol),
              TCASE(test_reader_parses_one_element_list),
              TCASE(test_reader_parses_list),
              TCASE(test_reader_parses_nested_lists),
              TCASE(test_reader_reports_unclosed_list),
              TCASE(test_reader_returns_eof_arbitrary_amount_of_times),
              TCASE(test_reader_reports_stary_rparen),
              TCASE(test_reader_parses_nil),
              TCASE(test_reader_parses_simple_dotted_cons),
              TCASE(test_reader_parses_dotted_list),
              TCASE(test_reader_parses_quote),
              { NULL, NULL } };
