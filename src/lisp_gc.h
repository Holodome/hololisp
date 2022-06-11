#ifndef __LISP_GC_H__
#define __LISP_GC_H__

#include <stddef.h>
#include <stdint.h>

struct hll_obj;

struct hll_obj *hll_alloc(size_t body_size, uint32_t kind);
void hll_free(struct hll_obj *head);

#endif
