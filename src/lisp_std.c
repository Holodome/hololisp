#include <stdio.h>

#include "lisp.h"

struct hll_obj *
hll_std_print(struct hll_ctx *ctx, struct hll_obj *args) {
    FILE *f = ctx->file_out;
    hll_print(f, hll_eval(ctx, hll_unwrap_cons(args)->car));
    fprintf(f, "\n");
    return hll_nil;
}

struct hll_obj *
hll_std_add(struct hll_ctx *ctx, struct hll_obj *args) {
    int64_t result = 0;
    for (hll_obj *obj = args; obj != hll_nil; obj = hll_unwrap_car(obj)) {
        result += hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

struct hll_obj *
hll_std_sub(struct hll_ctx *ctx, struct hll_obj *args) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;
    for (hll_obj *obj = hll_unwrap_cons(args)->cdr; obj != hll_nil;
         obj = hll_unwrap_car(obj)) {
        result -= hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

struct hll_obj *
hll_std_div(struct hll_ctx *ctx, struct hll_obj *args) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;

    for (hll_obj *obj = hll_unwrap_cons(args)->cdr; obj != hll_nil;
         obj = hll_unwrap_car(obj)) {
        result /= hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

struct hll_obj *
hll_std_mul(struct hll_ctx *ctx, struct hll_obj *args) {
    int64_t result = 1;
    for (hll_obj *obj = args; obj != hll_nil; obj = hll_unwrap_car(obj)) {
        result *= hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

struct hll_obj *
hll_std_quote(struct hll_ctx *ctx, struct hll_obj *args) {
    (void)ctx;
    return hll_unwrap_cons(args)->car;
}

struct hll_obj *
hll_std_eval(struct hll_ctx *ctx, struct hll_obj *args) {
    return hll_eval(ctx, hll_eval(ctx, hll_unwrap_cons(args)->car));
}

struct hll_obj *
hll_std_int_ne(struct hll_ctx *ctx, struct hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error(ctx, "/= must have at least 1 argument");
        return hll_nil;
    }

    for (hll_obj *obj1 = args; obj1 != hll_nil; obj1 = hll_unwrap_cdr(obj1)) {
        hll_obj *num1 = hll_unwrap_car(obj1);
        if (num1->kind != HLL_OBJ_INT) {
            hll_report_error(ctx, "/= arguments must be integers");
            return hll_nil;
        }

        for (hll_obj *obj2 = hll_unwrap_cdr(obj1); obj2 != hll_nil;
             obj2 = hll_unwrap_cdr(obj2)) {
            if (obj1 == obj2) {
                continue;
            }

            hll_obj *num2 = hll_unwrap_car(obj2);
            if (num2->kind != HLL_OBJ_INT) {
                hll_report_error(ctx, "/= arguments must be integers");
                return hll_nil;
            }

            if (hll_unwrap_int(num1)->value == hll_unwrap_int(num2)->value) {
                return hll_nil;
            }
        }
    }

    return hll_true;
}

struct hll_obj *
hll_std_int_eq(struct hll_ctx *ctx, struct hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error(ctx, "= must have at least 1 argument");
        return hll_nil;
    }

    for (hll_obj *obj1 = args; obj1 != hll_nil; obj1 = hll_unwrap_cdr(obj1)) {
        hll_obj *num1 = hll_unwrap_car(obj1);
        if (num1->kind != HLL_OBJ_INT) {
            hll_report_error(ctx, "= arguments must be integers");
            return hll_nil;
        }

        for (hll_obj *obj2 = hll_unwrap_cdr(obj1); obj2 != hll_nil;
             obj2 = hll_unwrap_cdr(obj2)) {
            if (obj1 == obj2) {
                continue;
            }

            hll_obj *num2 = hll_unwrap_car(obj2);
            if (num2->kind != HLL_OBJ_INT) {
                hll_report_error(ctx, "= arguments must be integers");
                return hll_nil;
            }

            if (hll_unwrap_int(num1)->value != hll_unwrap_int(num2)->value) {
                return hll_nil;
            }
        }
    }

    return hll_true;
}

struct hll_obj *
hll_std_int_gt(struct hll_ctx *ctx, struct hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error(ctx, "> must have at least 1 argument");
        return hll_nil;
    }

    hll_obj *prev = hll_unwrap_car(args);
    if (prev->kind != HLL_OBJ_INT) {
        hll_report_error(ctx, "> arguments must be integers");
        return hll_nil;
    }

    for (hll_obj *obj = hll_unwrap_cdr(args); obj != hll_nil;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        if (num->kind != HLL_OBJ_INT) {
            hll_report_error(ctx, "> arguments must be integers");
            return hll_nil;
        }

        if (hll_unwrap_int(prev)->value <= hll_unwrap_int(num)->value) {
            return hll_nil;
        }

        prev = num;
    }

    return hll_true;
}

struct hll_obj *
hll_std_int_ge(struct hll_ctx *ctx, struct hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error(ctx, ">= must have at least 1 argument");
        return hll_nil;
    }

    hll_obj *prev = hll_unwrap_car(args);
    if (prev->kind != HLL_OBJ_INT) {
        hll_report_error(ctx, ">= arguments must be integers");
        return hll_nil;
    }

    for (hll_obj *obj = hll_unwrap_cdr(args); obj != hll_nil;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        if (num->kind != HLL_OBJ_INT) {
            hll_report_error(ctx, ">= arguments must be integers");
            return hll_nil;
        }

        if (hll_unwrap_int(prev)->value < hll_unwrap_int(num)->value) {
            return hll_nil;
        }
        prev = num;
    }

    return hll_true;
}

struct hll_obj *
hll_std_int_lt(struct hll_ctx *ctx, struct hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error(ctx, "< must have at least 1 argument");
        return hll_nil;
    }

    hll_obj *prev = hll_unwrap_car(args);
    if (prev->kind != HLL_OBJ_INT) {
        hll_report_error(ctx, "< arguments must be integers");
        return hll_nil;
    }

    for (hll_obj *obj = hll_unwrap_cdr(args); obj != hll_nil;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        if (num->kind != HLL_OBJ_INT) {
            hll_report_error(ctx, "< arguments must be integers");
            return hll_nil;
        }

        if (hll_unwrap_int(prev)->value >= hll_unwrap_int(num)->value) {
            return hll_nil;
        }
        prev = num;
    }

    return hll_true;
}

struct hll_obj *
hll_std_int_le(struct hll_ctx *ctx, struct hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error(ctx, "<= must have at least 1 argument");
        return hll_nil;
    }

    hll_obj *prev = hll_unwrap_car(args);
    if (prev->kind != HLL_OBJ_INT) {
        hll_report_error(ctx, "<= arguments must be integers");
        return hll_nil;
    }

    for (hll_obj *obj = hll_unwrap_cdr(args); obj != hll_nil;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        if (num->kind != HLL_OBJ_INT) {
            hll_report_error(ctx, "<= arguments must be integers");
            return hll_nil;
        }

        if (hll_unwrap_int(prev)->value > hll_unwrap_int(num)->value) {
            return hll_nil;
        }
        prev = num;
    }

    return hll_true;
}

void
hll_init_std(hll_ctx *ctx) {
#define STR_LEN(_str) _str, (sizeof(_str) - 1)
#define BIND(_func, _symb) hll_add_binding(ctx, _func, STR_LEN(_symb))
    BIND(hll_std_print, "print");
    BIND(hll_std_add, "+");
    BIND(hll_std_sub, "-");
    BIND(hll_std_div, "/");
    BIND(hll_std_mul, "*");
    BIND(hll_std_quote, "quote");
    BIND(hll_std_eval, "eval");
    BIND(hll_std_int_eq, "=");
    BIND(hll_std_int_ne, "/=");
    BIND(hll_std_int_lt, "<");
    BIND(hll_std_int_le, "<=");
    BIND(hll_std_int_gt, ">");
    BIND(hll_std_int_ge, ">=");
    hll_unwrap_env(ctx->env_stack)->vars =
        hll_make_acons(ctx, hll_find_symb(ctx, STR_LEN("t")), hll_true,
                       hll_unwrap_env(ctx->env_stack)->vars);
#undef BIND
#undef STR_LEN
}
