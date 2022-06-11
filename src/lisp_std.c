#include "lisp_std.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "lisp.h"

hll_obj *
hll_std_print(hll_ctx *ctx, hll_obj *args) {
    FILE *f = ctx->file_out;
    hll_print(f, hll_eval(ctx, hll_unwrap_cons(args)->car));
    fprintf(f, "\n");
    return hll_nil;
}

hll_obj *
hll_std_add(hll_ctx *ctx, hll_obj *args) {
    int64_t result = 0;
    for (hll_obj *obj = args; obj != hll_nil; obj = hll_unwrap_cdr(obj)) {
        result += hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

hll_obj *
hll_std_sub(hll_ctx *ctx, hll_obj *args) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;
    for (hll_obj *obj = hll_unwrap_cons(args)->cdr; obj != hll_nil;
         obj = hll_unwrap_cdr(obj)) {
        result -= hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

hll_obj *
hll_std_div(hll_ctx *ctx, hll_obj *args) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;

    for (hll_obj *obj = hll_unwrap_cons(args)->cdr; obj != hll_nil;
         obj = hll_unwrap_cdr(obj)) {
        result /= hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

hll_obj *
hll_std_mul(hll_ctx *ctx, hll_obj *args) {
    int64_t result = 1;
    for (hll_obj *obj = args; obj != hll_nil; obj = hll_unwrap_cdr(obj)) {
        result *= hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

hll_obj *
hll_std_quote(hll_ctx *ctx, hll_obj *args) {
    (void)ctx;
    return hll_unwrap_cons(args)->car;
}

hll_obj *
hll_std_eval(hll_ctx *ctx, hll_obj *args) {
    return hll_eval(ctx, hll_eval(ctx, hll_unwrap_cons(args)->car));
}

hll_obj *
hll_std_int_ne(hll_ctx *ctx, hll_obj *args) {
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

hll_obj *
hll_std_int_eq(hll_ctx *ctx, hll_obj *args) {
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

hll_obj *
hll_std_int_gt(hll_ctx *ctx, hll_obj *args) {
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

hll_obj *
hll_std_int_ge(hll_ctx *ctx, hll_obj *args) {
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

hll_obj *
hll_std_int_lt(hll_ctx *ctx, hll_obj *args) {
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

hll_obj *
hll_std_int_le(hll_ctx *ctx, hll_obj *args) {
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

static hll_obj *
uber_car_cdr(hll_ctx *ctx, hll_obj *args, char const *ops) {
    if (hll_list_length(args) != 1) {
        hll_report_error(ctx, "c%sr must have exactly 1 argument", ops);
        return hll_nil;
    }

    hll_obj *result = hll_eval(ctx, hll_unwrap_car(args));

    char const *op = ops + strlen(ops) - 1;
    while (op >= ops && result != hll_nil) {
        if (*op == 'a') {
            // TODO: Error checking here
            result = hll_unwrap_car(result);
        } else if (*op == 'd') {
            // TODO: Error checking here
            result = hll_unwrap_cdr(result);
        } else {
            assert(0);
        }

        --op;
    }

    return result;
}

#define HLL_CAR_CDR(_letters)                                      \
    hll_obj *hll_std_c##_letters##r(hll_ctx *ctx, hll_obj *args) { \
        return uber_car_cdr(ctx, args, #_letters);                 \
    }
HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR

struct hll_obj *
hll_std_if(struct hll_ctx *ctx, struct hll_obj *args) {
    if (hll_list_length(args) < 2) {
        hll_report_error(ctx, "If expects at least 2 arguments");
        return hll_nil;
    }

    hll_obj *result = hll_nil;

    hll_obj *cond = hll_eval(ctx, hll_unwrap_car(args));
    if (cond != hll_nil) {
        hll_obj *then = hll_unwrap_car(hll_unwrap_cdr(args));
        result = hll_eval(ctx, then);
    } else {
        hll_obj *else_ = hll_unwrap_cdr(hll_unwrap_cdr(args));
        if (else_ != hll_nil) {
            result = hll_eval(ctx, hll_unwrap_car(else_));
        }
    }

    return result;
}

struct hll_obj *
hll_std_cons(struct hll_ctx *ctx, struct hll_obj *args) {
    if (hll_list_length(args) != 2) {
        hll_report_error(ctx, "cons expects exactly 2 arguments");
        return hll_nil;
    }

    hll_obj *car = hll_unwrap_car(args);
    hll_obj *cdr = hll_unwrap_car(hll_unwrap_cdr(args));
    return hll_make_cons(ctx, hll_eval(ctx, car), hll_eval(ctx, cdr));
}

struct hll_obj *
hll_std_list(struct hll_ctx *ctx, struct hll_obj *args) {
    hll_obj *head = NULL;
    hll_obj *tail = NULL;

    for (hll_obj *src = args; src != hll_nil; src = hll_unwrap_cdr(src)) {
        hll_obj *tmp = hll_eval(ctx, hll_unwrap_car(src));

        if (head == NULL) {
            head = tail = hll_make_cons(ctx, tmp, hll_nil);
        } else {
            hll_unwrap_cons(tail)->cdr = hll_make_cons(ctx, tmp, hll_nil);
            tail = hll_unwrap_cdr(tail);
        }
    }

    if (head == NULL) {
        head = hll_nil;
    }
    return head;
}
