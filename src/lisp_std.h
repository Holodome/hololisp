#ifndef __HLL_LISP_STD_H__
#define __HLL_LISP_STD_H__

#include <stdint.h>

struct hll_ctx;
struct hll_obj;

#define _HLL_ENUMERATE_CAR_CDR \
    _HLL_CAR_CDR(a)            \
    _HLL_CAR_CDR(d)            \
    _HLL_CAR_CDR(aa)           \
    _HLL_CAR_CDR(ad)           \
    _HLL_CAR_CDR(da)           \
    _HLL_CAR_CDR(dd)           \
    _HLL_CAR_CDR(aaa)          \
    _HLL_CAR_CDR(aad)          \
    _HLL_CAR_CDR(ada)          \
    _HLL_CAR_CDR(add)          \
    _HLL_CAR_CDR(daa)          \
    _HLL_CAR_CDR(dad)          \
    _HLL_CAR_CDR(dda)          \
    _HLL_CAR_CDR(ddd)          \
    _HLL_CAR_CDR(aaaa)         \
    _HLL_CAR_CDR(aaad)         \
    _HLL_CAR_CDR(aada)         \
    _HLL_CAR_CDR(aadd)         \
    _HLL_CAR_CDR(adaa)         \
    _HLL_CAR_CDR(adad)         \
    _HLL_CAR_CDR(adda)         \
    _HLL_CAR_CDR(addd)         \
    _HLL_CAR_CDR(daaa)         \
    _HLL_CAR_CDR(daad)         \
    _HLL_CAR_CDR(dada)         \
    _HLL_CAR_CDR(dadd)         \
    _HLL_CAR_CDR(ddaa)         \
    _HLL_CAR_CDR(ddad)         \
    _HLL_CAR_CDR(ddda)         \
    _HLL_CAR_CDR(dddd)

#define _HLL_CAR_CDR_STD_FUNC(_letters) \
    _HLL_STD_FUNC(c##_letters##r, "c" #_letters "r")
#define _HLL_CAR_CDR _HLL_CAR_CDR_STD_FUNC

#define _HLL_ENUMERATE_STD_FUNCS    \
    _HLL_STD_FUNC(print, "print")   \
    _HLL_STD_FUNC(add, "+")         \
    _HLL_STD_FUNC(sub, "-")         \
    _HLL_STD_FUNC(div, "/")         \
    _HLL_STD_FUNC(mul, "*")         \
    _HLL_STD_FUNC(int_eq, "=")      \
    _HLL_STD_FUNC(int_ne, "/=")     \
    _HLL_STD_FUNC(int_le, "<=")     \
    _HLL_STD_FUNC(int_lt, "<")      \
    _HLL_STD_FUNC(int_ge, ">=")     \
    _HLL_STD_FUNC(int_gt, ">")      \
    _HLL_STD_FUNC(quote, "quote")   \
    _HLL_STD_FUNC(eval, "eval")     \
    _HLL_STD_FUNC(if, "if")         \
    _HLL_STD_FUNC(cons, "cons")     \
    _HLL_STD_FUNC(list, "list")     \
    _HLL_STD_FUNC(defun, "defun")   \
    _HLL_STD_FUNC(lambda, "lambda") \
    _HLL_STD_FUNC(progn, "progn")   \
    _HLL_ENUMERATE_CAR_CDR

#define _HLL_STD_FUNC(_name, ...) \
    struct hll_obj *hll_std_##_name(struct hll_ctx *ctx, struct hll_obj *args);
_HLL_ENUMERATE_STD_FUNCS
#undef _HLL_CAR_CDR
#undef _HLL_STD_FUNC

#endif
