#ifndef __LISP_GC_H__
#define __LISP_GC_H__

#include <stdint.h>
#include <stddef.h>

struct hll_lisp_obj_head *hll_alloc(size_t body_size, uint32_t kind);
void hll_free(hll_lisp_obj_head *head);

#endif
