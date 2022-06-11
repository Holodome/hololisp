#include "../src/lisp.h"
#include "acutest.h"

static void
test_hll_make_int_works(void) {
    hll_ctx ctx = hll_default_ctx();

    hll_obj *obj = hll_make_int(&ctx, 100);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_LOBJ_INT);
    TEST_ASSERT(obj->size == sizeof(hll_int));

    hll_int *val = hll_unwrap_int(obj);
    TEST_ASSERT(val->value == 100);
}

static void
test_hll_make_symb_works(void) {
    hll_ctx ctx = hll_default_ctx();

    char const symb[] = "hello";
    size_t length = sizeof(symb) - 1;

    hll_obj *obj = hll_make_symb(&ctx, symb, length);
    TEST_ASSERT(obj != NULL);
    TEST_ASSERT(obj->kind == HLL_LOBJ_SYMB);
    TEST_ASSERT(obj->size == sizeof(hll_symb) + length + 1);

    hll_symb *val = hll_unwrap_symb(obj);
    TEST_ASSERT(strcmp(val->symb, symb) == 0);
    TEST_ASSERT(val->length == length);
}

static void
test_hll_make_cons_works(void) {
    hll_ctx ctx = hll_default_ctx();

    char const symb_data[] = "hello";
    size_t length = sizeof(symb_data) - 1;
    int64_t int_value = 100;

    hll_obj *symb = hll_make_symb(&ctx, symb_data, length);
    hll_obj *integer = hll_make_int(&ctx, int_value);

    hll_obj *obj = hll_make_cons(&ctx, symb, integer);
    hll_cons *cons = hll_unwrap_cons(obj);
    TEST_ASSERT(cons->car == symb);
    TEST_ASSERT(cons->cdr == integer);
}

static void
test_hll_reverse_list_works(void) {
    hll_ctx ctx = hll_default_ctx();

    char const symb_data[] = "hello";
    size_t length = sizeof(symb_data) - 1;
    int64_t int_value1 = 100;
    int64_t int_value2 = -100;

    hll_obj *symb = hll_make_symb(&ctx, symb_data, length);
    hll_obj *integer1 = hll_make_int(&ctx, int_value1);
    hll_obj *integer2 = hll_make_int(&ctx, int_value2);

    hll_obj *list = hll_make_cons(
        &ctx, symb,
        hll_make_cons(&ctx, integer1, hll_make_cons(&ctx, integer2, hll_nil)));

    size_t idx = 0;
    for (hll_obj *obj = list; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr, ++idx) {
        switch (idx) {
        default:
            TEST_ASSERT(0);
            break;
        case 0:
            TEST_ASSERT(hll_unwrap_cons(obj)->car == symb);
            break;
        case 1:
            TEST_ASSERT(hll_unwrap_cons(obj)->car == integer1);
            break;
        case 2:
            TEST_ASSERT(hll_unwrap_cons(obj)->car == integer2);
            break;
        }
    }

    list = hll_reverse_list(list);
    idx = 0;
    for (hll_obj *obj = list; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr, ++idx) {
        switch (idx) {
        default:
            TEST_ASSERT(0);
            break;
        case 2:
            TEST_ASSERT(hll_unwrap_cons(obj)->car == symb);
            break;
        case 1:
            TEST_ASSERT(hll_unwrap_cons(obj)->car == integer1);
            break;
        case 0:
            TEST_ASSERT(hll_unwrap_cons(obj)->car == integer2);
            break;
        }
    }
}

static void
test_hll_find_symb_works_single_item(void) {
    hll_ctx ctx = hll_default_ctx();

    char const symb[] = "hello";
    size_t length = sizeof(symb) - 1;

    hll_obj *initial = hll_find_symb(&ctx, symb, length);
    hll_obj *other = hll_find_symb(&ctx, symb, length);

    TEST_ASSERT(initial == other);
}

static void
test_hll_find_symb_works(void) {
    hll_ctx ctx = hll_default_ctx();

    char const symb1[] = "hello";
    size_t length1 = sizeof(symb1) - 1;
    char const symb2[] = "world";
    size_t length2 = sizeof(symb2) - 1;
    char const symb3[] = "holodome";
    size_t length3 = sizeof(symb3) - 1;

    hll_obj *obj1 = hll_find_symb(&ctx, symb1, length1);
    hll_obj *obj2 = hll_find_symb(&ctx, symb2, length2);
    hll_obj *obj3 = hll_find_symb(&ctx, symb3, length3);

    hll_obj *test1 = hll_find_symb(&ctx, symb1, length1);
    hll_obj *test2 = hll_find_symb(&ctx, symb2, length2);
    hll_obj *test3 = hll_find_symb(&ctx, symb3, length3);

    TEST_ASSERT(test1 == obj1);
    TEST_ASSERT(test2 == obj2);
    TEST_ASSERT(test3 == obj3);
}

#define TCASE(_name) \
    { #_name, _name }

TEST_LIST = { TCASE(test_hll_make_int_works),
              TCASE(test_hll_make_symb_works),
              TCASE(test_hll_make_cons_works),
              TCASE(test_hll_reverse_list_works),
              TCASE(test_hll_find_symb_works),
              TCASE(test_hll_find_symb_works_single_item),
              { NULL, NULL } };
