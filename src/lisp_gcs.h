#ifndef __LISP_GCS_H__
#define __LISP_GCS_H__

#include <stdint.h>

struct hll_lisp_ctx;

void hll_init_libc_no_gc(struct hll_lisp_ctx *ctx);

#endif
