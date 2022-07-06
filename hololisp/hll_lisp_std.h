#ifndef __HLL_LISP_STD_H__
#define __HLL_LISP_STD_H__

#include <stdint.h>

#include "hll_ext.h"

struct hll_ctx;
struct hll_obj;

#define _HLL_ENUMERATE_STD_FUNCS      \
    _HLL_STD_FUNC(print, "print")     \
    _HLL_STD_FUNC(add, "+")           \
    _HLL_STD_FUNC(sub, "-")           \
    _HLL_STD_FUNC(div, "/")           \
    _HLL_STD_FUNC(mul, "*")           \
    _HLL_STD_FUNC(int_eq, "=")        \
    _HLL_STD_FUNC(int_ne, "/=")       \
    _HLL_STD_FUNC(int_le, "<=")       \
    _HLL_STD_FUNC(int_lt, "<")        \
    _HLL_STD_FUNC(int_ge, ">=")       \
    _HLL_STD_FUNC(int_gt, ">")        \
    _HLL_STD_FUNC(quote, "quote")     \
    _HLL_STD_FUNC(eval, "eval")       \
    _HLL_STD_FUNC(if, "if")           \
    _HLL_STD_FUNC(cons, "cons")       \
    _HLL_STD_FUNC(list, "list")       \
    _HLL_STD_FUNC(defun, "defun")     \
    _HLL_STD_FUNC(lambda, "lambda")   \
    _HLL_STD_FUNC(progn, "progn")     \
    _HLL_STD_FUNC(nth, "nth")         \
    _HLL_STD_FUNC(nthcdr, "nthcdr")   \
    _HLL_STD_FUNC(setcar, "setcar")   \
    _HLL_STD_FUNC(setcdr, "setcdr")   \
    _HLL_STD_FUNC(any, "any")         \
    _HLL_STD_FUNC(map, "map")         \
    _HLL_STD_FUNC(when, "when")       \
    _HLL_STD_FUNC(unless, "unless")   \
    _HLL_STD_FUNC(not, "not")         \
    _HLL_STD_FUNC(or, "or")           \
    _HLL_STD_FUNC(and, "and")         \
    _HLL_STD_FUNC(listp, "listp")     \
    _HLL_STD_FUNC(null, "null")       \
    _HLL_STD_FUNC(minusp, "minusp")   \
    _HLL_STD_FUNC(zerop, "zerop")     \
    _HLL_STD_FUNC(plusp, "plusp")     \
    _HLL_STD_FUNC(numberp, "numberp") \
    _HLL_STD_FUNC(append, "append")   \
    _HLL_STD_FUNC(reverse, "reverse") \
    _HLL_STD_FUNC(min, "min")         \
    _HLL_STD_FUNC(max, "max")         \
    _HLL_STD_FUNC(abs, "abs")         \
    _HLL_STD_FUNC(prin1, "prin1")     \
    _HLL_STD_FUNC(read, "read")       \
    _HLL_STD_FUNC(while, "while")     \
    _HLL_STD_FUNC(let, "let")         \
    _HLL_STD_FUNC(defvar, "defvar")   \
    _HLL_STD_FUNC(setq, "setq")       \
    _HLL_STD_FUNC(rand, "rand")       \
    _HLL_STD_FUNC(clrscr, "clrscr")   \
    _HLL_STD_FUNC(car, "car")         \
    _HLL_STD_FUNC(cdr, "cdr")

#define _HLL_STD_FUNC(_name, ...)                                 \
    HLL_DECL struct hll_obj *hll_std_##_name(struct hll_ctx *ctx, \
                                             struct hll_obj *args);
_HLL_ENUMERATE_STD_FUNCS
#undef _HLL_CAR_CDR
#undef _HLL_STD_FUNC

#endif
