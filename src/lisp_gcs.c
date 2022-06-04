#include "lisp_gcs.h"

#include <stdlib.h>

#include "lisp.h"

static HLL_LOBJ_ALLOC(libc_no_gc_alloc) {
    hll_lisp_obj_head *head = malloc(sizeof(hll_lisp_obj_head) + body_size);
    head->kind = kind;
    head->size = body_size;

    return head;
}

static HLL_LOBJ_FREE(libc_no_gc_free) {
    free(head);
}

void
hll_init_libc_no_gc(hll_lisp_ctx *ctx) {
    ctx->alloc = libc_no_gc_alloc;
    ctx->free = libc_no_gc_free;
    ctx->state = NULL;
}
