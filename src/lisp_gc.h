/// This file contains functions related to memory-related stuff of system.
/// Functions for allocating objects are defined here.
/// Function for accessing object contents like per-kind data are also defined
/// here because they are related to inner structure of objects which is only
/// visible to allocator.
#ifndef __LISP_GC_H__
#define __LISP_GC_H__

#include <stddef.h>
#include <stdint.h>

#include "ext.h"

struct hll_obj;
struct hll_ctx;

struct hll_obj *hll_alloc(size_t body_size, uint32_t kind);
void hll_free(struct hll_obj *head);

HLL_DECL
struct hll_obj *hll_make_nil(struct hll_ctx *ctx);

HLL_DECL
struct hll_obj *hll_make_true(struct hll_ctx *ctx);

HLL_DECL
struct hll_obj *hll_make_cons(struct hll_ctx *ctx, struct hll_obj *car,
                              struct hll_obj *cdr);

HLL_DECL
struct hll_obj *hll_make_acons(struct hll_ctx *ctx, struct hll_obj *x,
                               struct hll_obj *y, struct hll_obj *a);

HLL_DECL
struct hll_obj *hll_make_symb(struct hll_ctx *ctx, char const *symb,
                              size_t length);

HLL_DECL
struct hll_obj *hll_make_int(struct hll_ctx *ctx, int64_t value);

HLL_DECL
struct hll_obj *hll_make_bind(struct hll_ctx *ctx,
                              struct hll_obj *(*func)(struct hll_ctx *ctx,
                                                      struct hll_obj *args));

HLL_DECL
struct hll_obj *hll_make_func(struct hll_ctx *ctx, struct hll_obj *env,
                              struct hll_obj *params, struct hll_obj *body);

HLL_DECL
struct hll_obj *hll_make_env(struct hll_ctx *ctx, struct hll_obj *up);

HLL_DECL
struct hll_cons *hll_unwrap_cons(struct hll_obj *obj);

HLL_DECL
struct hll_obj *hll_unwrap_car(struct hll_obj *obj);

HLL_DECL
struct hll_obj *hll_unwrap_cdr(struct hll_obj *obj);

HLL_DECL
struct hll_symb *hll_unwrap_symb(struct hll_obj *obj);

HLL_DECL
struct hll_int *hll_unwrap_int(struct hll_obj *obj);

HLL_DECL
struct hll_bind *hll_unwrap_bind(struct hll_obj *obj);

HLL_DECL
struct hll_env *hll_unwrap_env(struct hll_obj *obj);

HLL_DECL
struct hll_func *hll_unwrap_func(struct hll_obj *obj);

#endif
