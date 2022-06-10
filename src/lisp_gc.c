#include <stdlib.h>

#include "lisp.h"
#include "lisp_gc.h"

struct hll_lisp_obj_head *
hll_alloc(size_t body_size, uint32_t kind) {
    hll_lisp_obj_head *head = malloc(sizeof(hll_lisp_obj_head) + body_size);
    head->kind = kind;
    head->size = body_size;

    return head;
}

void
hll_free(hll_lisp_obj_head *head) {
    free(head);
}

