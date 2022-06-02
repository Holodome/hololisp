#include "lisp.h"

#include <assert.h>
#include <string.h>

static hll_lisp_obj_head *
cons_car_unwrap(hll_lisp_obj_head *obj) {
    assert(obj->kind == HLL_LOBJ_CONS);
    hll_lisp_cons *cons = obj->body;
    return cons->car;
}

static hll_lisp_obj_head *
cons_cdr_unwrap(hll_lisp_obj_head *obj) {
    assert(obj->kind == HLL_LOBJ_CONS);
    hll_lisp_cons *cons = obj->body;
    return cons->cdr;
}

hll_lisp_obj_head *
hll_make_cons(hll_lisp_ctx *ctx, hll_lisp_obj_head *car,
              hll_lisp_obj_head *cdr) {
    void *memory = ctx->alloc(sizeof(hll_lisp_cons), HLL_LOBJ_CONS);
    hll_lisp_obj_head *head = memory;

    hll_lisp_cons *cons = (void *)(head + 1);
    cons->car = car;
    cons->cdr = cdr;

    return head;
}

hll_lisp_obj_head *
hll_make_symb(hll_lisp_ctx *ctx, char const *data, size_t length) {
    void *memory =
        ctx->alloc(sizeof(hll_lisp_symb) + length + 1, HLL_LOBJ_SYMB);
    hll_lisp_obj_head *head = memory;

    hll_lisp_symb *symb = (void *)(head + 1);
    symb->symb = (void *)(symb + 1);
    strncpy((char *)symb->symb, data, length);

    return head;
}

hll_lisp_obj_head *
hll_make_int(hll_lisp_ctx *ctx, int64_t value) {
    void *memory = ctx->alloc(sizeof(hll_lisp_int), HLL_LOBJ_INT);
    hll_lisp_obj_head *head = memory;

    hll_lisp_int *obj = (void *)(head + 1);
    obj->value = value;

    return head;
}

hll_lisp_obj_head *
hll_reverse_list(hll_lisp_obj_head *obj) {
    hll_lisp_obj_head *result = hll_nil;

    while (obj != hll_nil) {
        assert(obj->kind == HLL_LOBJ_CONS);
        hll_lisp_obj_head *head = obj;
        hll_lisp_cons *cons = head->body;
        obj = cons->cdr;
        cons->cdr = result;
        result = head;
    }

    return result;
}

hll_lisp_obj_head *
hll_find_symb(hll_lisp_ctx *ctx, char const *data, size_t length) {
    hll_lisp_obj_head *found = NULL;

    for (hll_lisp_obj_head *obj = ctx->objects; obj != hll_nil && found == NULL;
         obj = cons_cdr_unwrap(obj)) {
        assert(obj->kind == HLL_LOBJ_CONS);
        hll_lisp_cons *cons = obj->body;
        hll_lisp_obj_head *symb_head = cons->car;
        assert(symb_head->kind == HLL_LOBJ_SYMB);
        hll_lisp_symb *symb = symb_head->body;

        if (strncmp(symb->symb, data, length) == 0) {
            found = obj;
        }
    }

    if (found == NULL) {
        found = hll_make_symb(ctx, data, length);
        ctx->objects = hll_make_cons(ctx, found, ctx->objects);
    } else {
        found = cons_car_unwrap(found);
    }

    return found;
}
