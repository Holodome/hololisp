#include "lisp.h"

#include <assert.h>
#include <string.h>

typedef struct {
    hll_lisp_obj_head head;
    union {
        hll_lisp_cons cons;
        hll_lisp_symb symb;
        hll_lisp_func func;
        hll_lisp_int integer;
    } body;
} lisp_obj;

static lisp_obj hll_nil_ = { .head = {
                                 .kind = HLL_LOBJ_NIL,
                                 .size = 0,
                             } };
hll_lisp_obj_head *hll_nil = &hll_nil_.head;

hll_lisp_ctx
hll_default_ctx(void) {
    hll_lisp_ctx ctx = { 0 };

    ctx.objects = hll_nil;

    return ctx;
}

hll_lisp_cons *
hll_unwrap_cons(hll_lisp_obj_head *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_LOBJ_CONS);
    return &obj->body.cons;
}

hll_lisp_symb *
hll_unwrap_symb(hll_lisp_obj_head *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_LOBJ_SYMB);
    return &obj->body.symb;
}

hll_lisp_int *
hll_unwrap_int(hll_lisp_obj_head *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_LOBJ_INT);
    return &obj->body.integer;
}

hll_lisp_obj_head *
hll_make_cons(hll_lisp_ctx *ctx, hll_lisp_obj_head *car,
              hll_lisp_obj_head *cdr) {
    hll_lisp_obj_head *cons = ctx->alloc(sizeof(hll_lisp_cons), HLL_LOBJ_CONS);

    hll_unwrap_cons(cons)->car = car;
    hll_unwrap_cons(cons)->cdr = cdr;

    return cons;
}

hll_lisp_obj_head *
hll_make_acons(hll_lisp_ctx *ctx, hll_lisp_obj_head *x, hll_lisp_obj_head *y,
               hll_lisp_obj_head *a) {
    return hll_make_cons(ctx, hll_make_cons(ctx, x, y), a);
}

hll_lisp_obj_head *
hll_make_symb(hll_lisp_ctx *ctx, char const *data, size_t length) {
    hll_lisp_obj_head *symb =
        ctx->alloc(sizeof(hll_lisp_symb) + length + 1, HLL_LOBJ_SYMB);

    char *string =
        (char *)symb + sizeof(hll_lisp_obj_head) + sizeof(hll_lisp_symb);
    strncpy(string, data, length);
    hll_unwrap_symb(symb)->symb = string;
    hll_unwrap_symb(symb)->length = length;

    return symb;
}

hll_lisp_obj_head *
hll_make_int(hll_lisp_ctx *ctx, int64_t value) {
    hll_lisp_obj_head *integer = ctx->alloc(sizeof(hll_lisp_int), HLL_LOBJ_INT);
    hll_unwrap_int(integer)->value = value;
    return integer;
}

hll_lisp_obj_head *
hll_reverse_list(hll_lisp_obj_head *obj) {
    hll_lisp_obj_head *result = hll_nil;

    while (obj != hll_nil) {
        assert(obj->kind == HLL_LOBJ_CONS);
        hll_lisp_obj_head *head = obj;
        obj = hll_unwrap_cons(obj)->cdr;
        hll_unwrap_cons(head)->cdr = result;
        result = head;
    }

    return result;
}

hll_lisp_obj_head *
hll_find_symb(hll_lisp_ctx *ctx, char const *data, size_t length) {
    hll_lisp_obj_head *found = NULL;

    for (hll_lisp_obj_head *obj = ctx->objects; obj != hll_nil && found == NULL;
         obj = hll_unwrap_cons(obj)->cdr) {
        assert(obj->kind == HLL_LOBJ_CONS);
        hll_lisp_obj_head *symb = hll_unwrap_cons(obj)->car;

        if (strcmp(hll_unwrap_symb(symb)->symb, data) == 0) {
            found = obj;
        }
    }

    if (found == NULL) {
        found = hll_make_symb(ctx, data, length);
        ctx->objects = hll_make_cons(ctx, found, ctx->objects);
    } else {
        found = hll_unwrap_cons(found)->car;
    }

    return found;
}
