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

#define HLL_ENUMERATE_CAR_CDR \
    HLL_CAR_CDR(a)            \
    HLL_CAR_CDR(d)            \
    HLL_CAR_CDR(aa)           \
    HLL_CAR_CDR(ad)           \
    HLL_CAR_CDR(da)           \
    HLL_CAR_CDR(dd)           \
    HLL_CAR_CDR(aaa)          \
    HLL_CAR_CDR(aad)          \
    HLL_CAR_CDR(ada)          \
    HLL_CAR_CDR(add)          \
    HLL_CAR_CDR(daa)          \
    HLL_CAR_CDR(dad)          \
    HLL_CAR_CDR(dda)          \
    HLL_CAR_CDR(ddd)          \
    HLL_CAR_CDR(aaaa)         \
    HLL_CAR_CDR(aaad)         \
    HLL_CAR_CDR(aada)         \
    HLL_CAR_CDR(aadd)         \
    HLL_CAR_CDR(adaa)         \
    HLL_CAR_CDR(adad)         \
    HLL_CAR_CDR(adda)         \
    HLL_CAR_CDR(addd)         \
    HLL_CAR_CDR(daaa)         \
    HLL_CAR_CDR(daad)         \
    HLL_CAR_CDR(dada)         \
    HLL_CAR_CDR(dadd)         \
    HLL_CAR_CDR(ddaa)         \
    HLL_CAR_CDR(ddad)         \
    HLL_CAR_CDR(ddda)         \
    HLL_CAR_CDR(dddd)

#define HLL_CAR_CDR(_letters)                                   \
    struct hll_obj *hll_std_c##_letters##r(struct hll_ctx *ctx, \
                                           struct hll_obj *args);
HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR

struct hll_obj *hll_std_if(struct hll_ctx *ctx, struct hll_obj *args);

#endif
