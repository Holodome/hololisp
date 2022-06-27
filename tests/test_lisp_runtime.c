#include "../src/error_reporter.h"
#include "../src/lexer.h"
#include "../src/lisp.h"
#include "../src/lisp_std.h"
#include "../src/reader.h"
#include "acutest.h"

static void
test_lisp_print_nil(void) {
    char const *source = "()";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *out = tmpfile();
    hll_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "()") == 0);
}

static void
test_lisp_print_number(void) {
    char const *source = "123";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *out = tmpfile();
    hll_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "123") == 0);
}

static void
test_lisp_print_symbol(void) {
    char const *source = "hello-world";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *out = tmpfile();
    hll_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "hello-world") == 0);
}

static void
test_lisp_print_single_element_list(void) {
    char const *source = "(123)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *out = tmpfile();
    hll_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "(123)") == 0);
}

static void
test_lisp_print_list(void) {
    char const *source = "(+ 1 2)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *out = tmpfile();
    hll_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "(+ 1 2)") == 0);
}

static void
test_lisp_eval_int(void) {
    char const *source = "123";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result == obj);
}

static int test_builtin_var = 0;

struct hll_obj *
test_binding(struct hll_ctx *ctx, struct hll_obj *args) {
    (void)ctx;
    (void)args;
    test_builtin_var = 100;
    return hll_make_nil(ctx);
}

static void
test_lisp_eval_builtin_call(void) {
    char const *source = "(test)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    hll_add_binding(&ctx, test_binding, "test", 4);

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    test_builtin_var = 0;
    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(test_builtin_var == 100);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_NIL);
}

static void
test_builtin_print(void) {
    char const *source = "(print 1)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *f = tmpfile();
    ctx.file_out = f;
    hll_eval(&ctx, obj);

    fseek(f, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), f);
    TEST_ASSERT(strcmp(buffer, "1\n") == 0);
}

static void
test_find_builtins_work(void) {
    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_obj *obj = hll_find_symb(&ctx, "+", 1);
    TEST_ASSERT(obj != NULL);
    obj = hll_find_symb(&ctx, "-", 1);
    TEST_ASSERT(obj != NULL);
    obj = hll_find_symb(&ctx, "*", 1);
    TEST_ASSERT(obj != NULL);
    obj = hll_find_symb(&ctx, "/", 1);
    TEST_ASSERT(obj != NULL);
}

static void
test_add(void) {
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());
    char const *source = "(+ 1 2)";
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(eval_result)->value == 3);
}

static void
test_sub(void) {
    char buffer[4096];
    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    char const *source = "(- 1 2)";
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(eval_result)->value == -1);
}

static void
test_mul(void) {
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    char const *source = "(* 4 2)";
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(eval_result)->value == 8);
}

static void
test_div(void) {
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    char const *source = "(/ 100 10)";
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(eval_result)->value == 10);
}

static void
test_add_multiple_args(void) {
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    char const *source = "(+ 1 2 3 4)";
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(eval_result)->value == 10);
}

static void
test_sub_multiple_args(void) {
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    char const *source = "(- 1 2 -2)";
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(eval_result)->value == 1);
}

static void
test_mul_multiple_args(void) {
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    char const *source = "(* 4 2 -1)";
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(eval_result)->value == -8);
}

static void
test_div_multiple_args(void) {
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    char const *source = "(/ 100 10 -1)";
    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    hll_obj *eval_result = hll_eval(&ctx, obj);
    TEST_ASSERT(eval_result->kind == HLL_OBJ_INT);
    TEST_ASSERT(hll_unwrap_int(eval_result)->value == -10);
}

static void
test_lisp_eval_nested_function_calls(void) {
    char const *source = "(print (+ (* 3 2) (/ 8 1)))";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *f = tmpfile();
    ctx.file_out = f;
    hll_eval(&ctx, obj);

    fseek(f, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), f);
    TEST_ASSERT(strcmp(buffer, "14\n") == 0);
}

static void
test_lisp_prints_dotted_list(void) {
    char const *source = "(a b 1 . (c d 2))";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);
    TEST_ASSERT(obj != NULL);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *out = tmpfile();
    hll_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "(a b 1 c d 2)") == 0);
}

static void
test_lisp_prints_quote(void) {
    char const *source = "'(the magic quote hack)";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *out = tmpfile();
    hll_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "(quote (the magic quote hack))") == 0);
}

static void
test_lisp_prints_quote_without_evaling(void) {
    char const *source = "'(+ 1 (* 2 3))";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *out = tmpfile();
    hll_print(out, obj);

    fseek(out, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), out);
    TEST_ASSERT(strcmp(buffer, "(quote (+ 1 (* 2 3)))") == 0);
}

static void
test_lisp_evals_quote(void) {
    char const *source = "(print (eval '(+ 1 (* 2 3))))";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    hll_obj *temp;
    result = hll_read(&reader, &temp);
    TEST_ASSERT(result == HLL_READ_EOF);

    FILE *f = tmpfile();
    ctx.file_out = f;
    hll_eval(&ctx, obj);

    fseek(f, 0, SEEK_SET);
    fgets(buffer, sizeof(buffer), f);
    TEST_ASSERT(strcmp(buffer, "7\n") == 0);
}

static void
test_lisp_evals_true(void) {
    char const *source = "t";
    char buffer[4096];

    hll_ctx ctx = hll_create_ctx(hll_get_empty_reporter());

    hll_lexer lexer = hll_lexer_create(source, buffer, sizeof(buffer));
    hll_reader reader = hll_reader_create(&lexer, &ctx);

    hll_obj *obj = NULL;
    hll_read_result result = hll_read(&reader, &obj);
    TEST_ASSERT(result == HLL_READ_OK);

    obj = hll_eval(&ctx, obj);
    TEST_ASSERT(obj->kind == HLL_OBJ_TRUE);
}

#define TCASE(_name) \
    { #_name, _name }

TEST_LIST = { TCASE(test_lisp_print_nil),
              TCASE(test_lisp_print_number),
              TCASE(test_lisp_print_symbol),
              TCASE(test_lisp_print_single_element_list),
              TCASE(test_lisp_print_list),
              TCASE(test_lisp_eval_int),
              TCASE(test_lisp_eval_builtin_call),
              TCASE(test_builtin_print),
              TCASE(test_find_builtins_work),
              TCASE(test_add),
              TCASE(test_sub),
              TCASE(test_mul),
              TCASE(test_div),
              TCASE(test_add_multiple_args),
              TCASE(test_sub_multiple_args),
              TCASE(test_mul_multiple_args),
              TCASE(test_div_multiple_args),
              TCASE(test_lisp_eval_nested_function_calls),
              TCASE(test_lisp_prints_quote),
              TCASE(test_lisp_prints_dotted_list),
              TCASE(test_lisp_prints_quote_without_evaling),
              TCASE(test_lisp_evals_quote),
              TCASE(test_lisp_evals_true),
              { NULL, NULL } };
