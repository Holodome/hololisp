#include "lisp.h"

#include <string.h>

hll_lisp_obj_head *
hll_make_cons(hll_lisp_ctx *ctx, hll_lisp_obj_head *car,
              hll_lisp_obj_head *cdr) {
    void *memory = ctx->alloc(sizeof(hll_lisp_obj_cons), HLL_LOBJ_CONS);
    hll_lisp_obj_head *head = memory;

    hll_lisp_obj_cons *cons = (void *)(head + 1);
    cons->car               = car;
    cons->cdr               = cdr;

    return head;
}

hll_lisp_obj_head *
hll_make_symb(hll_lisp_ctx *ctx, char const *data, size_t length) {
    void *memory =
        ctx->alloc(sizeof(hll_lisp_obj_symb) + length + 1, HLL_LOBJ_SYMB);
    hll_lisp_obj_head *head = memory;

    hll_lisp_obj_symb *symb = (void *)(head + 1);
    symb->symb              = (void *)(symb + 1);
    strncpy((char *)symb->symb, data, length);

    return head;
}

hll_lisp_obj_head *
hll_make_int(hll_lisp_ctx *ctx, int64_t value) {
    void *memory = ctx->alloc(sizeof(hll_lisp_obj_int), HLL_LOBJ_INT);
    hll_lisp_obj_head *head = memory;

    hll_lisp_obj_int *obj = (void *)(head + 1);
    obj->value            = value;

    return head;
}

