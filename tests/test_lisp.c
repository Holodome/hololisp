#include "../src/lisp.h"
#include "../src/lisp_gcs.h"
#include "acutest.h"

static void
test_hll_make_int_works(void) {
    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    hll_lisp_obj_head *obj = hll_make_int(&ctx, 100);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_LOBJ_INT);
    TEST_ASSERT(obj->size == sizeof(hll_lisp_int));

    hll_lisp_int *val = hll_obj_unwrap_int(obj);
    TEST_ASSERT(val->value == 100);
}

static void
test_hll_make_symb_works(void) {
    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    char const symb[] = "hello";
    size_t length = sizeof(symb) - 1;

    hll_lisp_obj_head *obj = hll_make_symb(&ctx, symb, length);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_LOBJ_SYMB);
    TEST_ASSERT(obj->size == sizeof(hll_lisp_symb) + length + 1);

    hll_lisp_symb *val = hll_obj_unwrap_symb(obj);
    TEST_ASSERT(strcmp(val->symb, symb) == 0);
    TEST_ASSERT(val->length == length);
}

static void
test_hll_make_cons_works(void) {
    hll_lisp_ctx ctx = hll_default_ctx();
    hll_init_libc_no_gc(&ctx);

    char const symb_data[] = "hello";
    size_t length = sizeof(symb_data) - 1;
    int64_t int_value = 100;

    hll_lisp_obj_head *symb = hll_make_symb(&ctx, symb_data, length);
    hll_lisp_obj_head *integer = hll_make_int(&ctx, int_value);

    hll_lisp_obj_head *obj = hll_make_cons(&ctx, symb, integer);
    hll_lisp_cons *cons = hll_obj_unwrap_cons(obj);
    TEST_ASSERT(cons->car == symb);
    TEST_ASSERT(cons->cdr == integer);
}

#define TCASE(_name) \
    { #_name, _name }

TEST_LIST = { TCASE(test_hll_make_int_works),
              TCASE(test_hll_make_symb_works),
              TCASE(test_hll_make_cons_works),
              { NULL, NULL } };
