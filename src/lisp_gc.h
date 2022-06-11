#ifndef __LISP_GC_H__
#define __LISP_GC_H__

#include <stdint.h>
#include <stddef.h>

struct hll_obj *hll_alloc(size_t body_size, uint32_t kind);
void hll_free(hll_obj *head);

#endif
