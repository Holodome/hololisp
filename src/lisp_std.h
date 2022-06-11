#ifndef __HLL_LISP_STD_H__
#define __HLL_LISP_STD_H__

#include <stdint.h>

struct hll_ctx;
struct hll_obj;

// (print <arg>)
struct hll_obj *hll_std_print(struct hll_ctx *ctx, struct hll_obj *args);

// (+ <summand: int>+)
struct hll_obj *hll_std_add(struct hll_ctx *ctx, struct hll_obj *args);

// (- <minuend: int> <subtrahend: int>*)
struct hll_obj *hll_std_sub(struct hll_ctx *ctx, struct hll_obj *args);

// (/ <dividend: int> <divisor: int>*)
struct hll_obj *hll_std_div(struct hll_ctx *ctx, struct hll_obj *args);

// (* <factor: int>+)
struct hll_obj *hll_std_mul(struct hll_ctx *ctx, struct hll_obj *args);

// (quote <arg>)
struct hll_obj *hll_std_quote(struct hll_ctx *ctx, struct hll_obj *args);

// (eval <arg>)
struct hll_obj *hll_std_eval(struct hll_ctx *ctx, struct hll_obj *args);

// (/= <num: int>+)
struct hll_obj *hll_std_int_ne(struct hll_ctx *ctx, struct hll_obj *args);

// (= <num: int>+)
struct hll_obj *hll_std_int_eq(struct hll_ctx *ctx, struct hll_obj *args);

// (< <num: int>+)
struct hll_obj *hll_std_int_lt(struct hll_ctx *ctx, struct hll_obj *args);

// (<= <num: int>+)
struct hll_obj *hll_std_int_le(struct hll_ctx *ctx, struct hll_obj *args);

// (> <num: int>+)
struct hll_obj *hll_std_int_gt(struct hll_ctx *ctx, struct hll_obj *args);

// (>= <num: int>+)
struct hll_obj *hll_std_int_ge(struct hll_ctx *ctx, struct hll_obj *args);

void hll_init_std(hll_ctx *ctx);

#endif
