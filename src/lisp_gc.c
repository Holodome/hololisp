#include "lisp_gc.h"

#include <stdlib.h>

#include "lisp.h"

struct hll_obj *
hll_alloc(size_t body_size, uint32_t kind) {
    hll_obj *head = malloc(sizeof(hll_obj) + body_size);
    head->kind = kind;
    head->size = body_size;

    return head;
}

void
hll_free(hll_obj *head) {
    free(head);
}
