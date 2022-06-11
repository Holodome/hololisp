#include <stdio.h>

#include "lisp.h"

#define CAR(_obj) (hll_unwrap_cons(_obj)->car)
#define CDR(_obj) (hll_unwrap_cons(_obj)->cdr)

static HLL_LISP_BIND(builtin_print) {
    FILE *f = ctx->file_out;
    hll_print(f, hll_eval(ctx, hll_unwrap_cons(args)->car));
    fprintf(f, "\n");
    return hll_nil;
}

static HLL_LISP_BIND(builtin_add) {
    int64_t result = 0;
    for (hll_obj *obj = args; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr) {
        result +=
            hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(obj)->car))->value;
    }
    return hll_make_int(ctx, result);
}

static HLL_LISP_BIND(builtin_sub) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;
    for (hll_obj *obj = hll_unwrap_cons(args)->cdr; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr) {
        result -=
            hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(obj)->car))->value;
    }
    return hll_make_int(ctx, result);
}

static HLL_LISP_BIND(builtin_div) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;

    for (hll_obj *obj = hll_unwrap_cons(args)->cdr; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr) {
        result /=
            hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(obj)->car))->value;
    }
    return hll_make_int(ctx, result);
}

static HLL_LISP_BIND(builtin_mul) {
    int64_t result = 1;
    for (hll_obj *obj = args; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr) {
        result *=
            hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(obj)->car))->value;
    }
    return hll_make_int(ctx, result);
}

static HLL_LISP_BIND(builtin_quote) {
    (void)ctx;
    return hll_unwrap_cons(args)->car;
}

static HLL_LISP_BIND(builtin_eval) {
    return hll_eval(ctx, hll_eval(ctx, hll_unwrap_cons(args)->car));
}

static HLL_LISP_BIND(builtin_num_ne) {
    if (args->kind != HLL_LOBJ_CONS) {
        hll_report_error(ctx, "/= must have at least 1 argument");
        return hll_nil;
    }

    for (hll_obj *obj1 = args; obj1 != hll_nil; obj1 = CDR(obj1)) {
        hll_obj *num1 = CAR(obj1);
        if (num1->kind != HLL_LOBJ_INT) {
            hll_report_error(ctx, "/= arguments must be integers");
            return hll_nil;
        }

        for (hll_obj *obj2 = CDR(obj1); obj2 != hll_nil;
             obj2 = CDR(obj2)) {
            if (obj1 == obj2) {
                continue;
            }

            hll_obj *num2 = CAR(obj2);
            if (num2->kind != HLL_LOBJ_INT) {
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

static HLL_LISP_BIND(builtin_num_eq) {
    if (args->kind != HLL_LOBJ_CONS) {
        hll_report_error(ctx, "= must have at least 1 argument");
        return hll_nil;
    }

    for (hll_obj *obj1 = args; obj1 != hll_nil; obj1 = CDR(obj1)) {
        hll_obj *num1 = CAR(obj1);
        if (num1->kind != HLL_LOBJ_INT) {
            hll_report_error(ctx, "= arguments must be integers");
            return hll_nil;
        }

        for (hll_obj *obj2 = CDR(obj1); obj2 != hll_nil;
             obj2 = CDR(obj2)) {
            if (obj1 == obj2) {
                continue;
            }

            hll_obj *num2 = CAR(obj2);
            if (num2->kind != HLL_LOBJ_INT) {
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

static HLL_LISP_BIND(builtin_num_gt) {
    if (args->kind != HLL_LOBJ_CONS) {
        hll_report_error(ctx, "> must have at least 1 argument");
        return hll_nil;
    }

    hll_obj *prev = CAR(args);
    if (prev->kind != HLL_LOBJ_INT) {
        hll_report_error(ctx, "> arguments must be integers");
        return hll_nil;
    }

    for (hll_obj *obj = CDR(args); obj != hll_nil; obj = CDR(obj)) {
        hll_obj *num = CAR(obj);
        if (num->kind != HLL_LOBJ_INT) {
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

static HLL_LISP_BIND(builtin_num_ge) {
    if (args->kind != HLL_LOBJ_CONS) {
        hll_report_error(ctx, ">= must have at least 1 argument");
        return hll_nil;
    }

    hll_obj *prev = CAR(args);
    if (prev->kind != HLL_LOBJ_INT) {
        hll_report_error(ctx, ">= arguments must be integers");
        return hll_nil;
    }

    for (hll_obj *obj = CDR(args); obj != hll_nil; obj = CDR(obj)) {
        hll_obj *num = CAR(obj);
        if (num->kind != HLL_LOBJ_INT) {
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

static HLL_LISP_BIND(builtin_num_lt) {
    if (args->kind != HLL_LOBJ_CONS) {
        hll_report_error(ctx, "< must have at least 1 argument");
        return hll_nil;
    }

    hll_obj *prev = CAR(args);
    if (prev->kind != HLL_LOBJ_INT) {
        hll_report_error(ctx, "< arguments must be integers");
        return hll_nil;
    }

    for (hll_obj *obj = CDR(args); obj != hll_nil; obj = CDR(obj)) {
        hll_obj *num = CAR(obj);
        if (num->kind != HLL_LOBJ_INT) {
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

static HLL_LISP_BIND(builtin_num_le) {
    if (args->kind != HLL_LOBJ_CONS) {
        hll_report_error(ctx, "<= must have at least 1 argument");
        return hll_nil;
    }

    hll_obj *prev = CAR(args);
    if (prev->kind != HLL_LOBJ_INT) {
        hll_report_error(ctx, "<= arguments must be integers");
        return hll_nil;
    }

    for (hll_obj *obj = CDR(args); obj != hll_nil; obj = CDR(obj)) {
        hll_obj *num = CAR(obj);
        if (num->kind != HLL_LOBJ_INT) {
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
hll_add_builtins(hll_ctx *ctx) {
#define STR_LEN(_str) _str, sizeof(_str) - 1
#define BIND(_func, _symb) hll_add_binding(ctx, _func, STR_LEN(_symb))
    BIND(builtin_print, "print");
    BIND(builtin_add, "+");
    BIND(builtin_sub, "-");
    BIND(builtin_div, "/");
    BIND(builtin_mul, "*");
    BIND(builtin_quote, "quote");
    BIND(builtin_eval, "eval");
    BIND(builtin_num_eq, "=");
    BIND(builtin_num_ne, "/=");
    BIND(builtin_num_lt, "<");
    BIND(builtin_num_le, "<=");
    BIND(builtin_num_gt, ">");
    BIND(builtin_num_ge, ">=");
    hll_unwrap_env(ctx->env_stack)->vars =
        hll_make_acons(ctx, hll_find_symb(ctx, STR_LEN("t")), hll_true,
                       hll_unwrap_env(ctx->env_stack)->vars);
#undef BIND
#undef STR_LEN
}
