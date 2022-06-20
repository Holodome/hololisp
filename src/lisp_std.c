#include "lisp_std.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_reporter.h"
#include "lexer.h"
#include "lisp.h"
#include "reader.h"

#define STD_FUNC(_name) hll_obj *hll_std_##_name(hll_ctx *ctx, hll_obj *args)

hll_obj *
hll_std_print(hll_ctx *ctx, hll_obj *args) {
    FILE *f = ctx->file_out;
    hll_print(f, hll_eval(ctx, hll_unwrap_cons(args)->car));
    fprintf(f, "\n");
    return hll_make_nil(ctx);
}

hll_obj *
hll_std_add(hll_ctx *ctx, hll_obj *args) {
    int64_t result = 0;
    for (hll_obj *obj = args; obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        result += hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

hll_obj *
hll_std_sub(hll_ctx *ctx, hll_obj *args) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;
    for (hll_obj *obj = hll_unwrap_cons(args)->cdr; obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        result -= hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

hll_obj *
hll_std_div(hll_ctx *ctx, hll_obj *args) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;

    for (hll_obj *obj = hll_unwrap_cons(args)->cdr; obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        result /= hll_unwrap_int(hll_eval(ctx, hll_unwrap_car(obj)))->value;
    }
    return hll_make_int(ctx, result);
}

hll_obj *
hll_std_mul(hll_ctx *ctx, hll_obj *args) {
    int64_t result = 1;
    for (hll_obj *obj = args; obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
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
        hll_report_error("/= must have at least 1 argument");
        return hll_make_nil(ctx);
    }

    args = hll_std_list(ctx, args);

    for (hll_obj *obj1 = args; obj1->kind != HLL_OBJ_NIL;
         obj1 = hll_unwrap_cdr(obj1)) {
        hll_obj *num1 = hll_unwrap_car(obj1);
        if (num1->kind != HLL_OBJ_INT) {
            hll_report_error("/= arguments must be integers");
            return hll_make_nil(ctx);
        }

        for (hll_obj *obj2 = hll_unwrap_cdr(obj1); obj2->kind != HLL_OBJ_NIL;
             obj2 = hll_unwrap_cdr(obj2)) {
            if (obj1 == obj2) {
                continue;
            }

            hll_obj *num2 = hll_unwrap_car(obj2);
            if (num2->kind != HLL_OBJ_INT) {
                hll_report_error("/= arguments must be integers");
                return hll_make_nil(ctx);
            }

            if (hll_unwrap_int(num1)->value == hll_unwrap_int(num2)->value) {
                return hll_make_nil(ctx);
            }
        }
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_eq(hll_ctx *ctx, hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error("= must have at least 1 argument");
        return hll_make_nil(ctx);
    }

    args = hll_std_list(ctx, args);

    for (hll_obj *obj1 = args; obj1->kind != HLL_OBJ_NIL;
         obj1 = hll_unwrap_cdr(obj1)) {
        hll_obj *num1 = hll_unwrap_car(obj1);
        if (num1->kind != HLL_OBJ_INT) {
            hll_report_error("= arguments must be integers)");
            return hll_make_nil(ctx);
        }

        for (hll_obj *obj2 = hll_unwrap_cdr(obj1); obj2->kind != HLL_OBJ_NIL;
             obj2 = hll_unwrap_cdr(obj2)) {
            if (obj1 == obj2) {
                continue;
            }

            hll_obj *num2 = hll_unwrap_car(obj2);
            if (num2->kind != HLL_OBJ_INT) {
                hll_report_error("= arguments must be integers");
                return hll_make_nil(ctx);
            }

            if (hll_unwrap_int(num1)->value != hll_unwrap_int(num2)->value) {
                return hll_make_nil(ctx);
            }
        }
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_gt(hll_ctx *ctx, hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error("> must have at least 1 argument");
        return hll_make_nil(ctx);
    }

    args = hll_std_list(ctx, args);

    hll_obj *prev = hll_unwrap_car(args);
    if (prev->kind != HLL_OBJ_INT) {
        hll_report_error("> arguments must be integers");
        return hll_make_nil(ctx);
    }

    for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        if (num->kind != HLL_OBJ_INT) {
            hll_report_error("> arguments must be integers");
            return hll_make_nil(ctx);
        }

        if (hll_unwrap_int(prev)->value <= hll_unwrap_int(num)->value) {
            return hll_make_nil(ctx);
        }

        prev = num;
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_ge(hll_ctx *ctx, hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error(">= must have at least 1 argument");
        return hll_make_nil(ctx);
    }

    args = hll_std_list(ctx, args);

    hll_obj *prev = hll_unwrap_car(args);
    if (prev->kind != HLL_OBJ_INT) {
        hll_report_error(">= arguments must be integers");
        return hll_make_nil(ctx);
    }

    for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        if (num->kind != HLL_OBJ_INT) {
            hll_report_error(">= arguments must be integers");
            return hll_make_nil(ctx);
        }

        if (hll_unwrap_int(prev)->value < hll_unwrap_int(num)->value) {
            return hll_make_nil(ctx);
        }
        prev = num;
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_lt(hll_ctx *ctx, hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error("< must have at least 1 argument");
        return hll_make_nil(ctx);
    }

    args = hll_std_list(ctx, args);

    hll_obj *prev = hll_unwrap_car(args);
    if (prev->kind != HLL_OBJ_INT) {
        hll_report_error("< arguments must be integers");
        return hll_make_nil(ctx);
    }

    for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        if (num->kind != HLL_OBJ_INT) {
            hll_report_error("< arguments must be integers");
            return hll_make_nil(ctx);
        }

        if (hll_unwrap_int(prev)->value >= hll_unwrap_int(num)->value) {
            return hll_make_nil(ctx);
        }
        prev = num;
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_le(hll_ctx *ctx, hll_obj *args) {
    if (args->kind != HLL_OBJ_CONS) {
        hll_report_error("<= must have at least 1 argument");
        return hll_make_nil(ctx);
    }

    args = hll_std_list(ctx, args);

    hll_obj *prev = hll_unwrap_car(args);
    if (prev->kind != HLL_OBJ_INT) {
        hll_report_error("<= arguments must be integers");
        return hll_make_nil(ctx);
    }

    for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        if (num->kind != HLL_OBJ_INT) {
            hll_report_error("<= arguments must be integers");
            return hll_make_nil(ctx);
        }

        if (hll_unwrap_int(prev)->value > hll_unwrap_int(num)->value) {
            return hll_make_nil(ctx);
        }
        prev = num;
    }

    return hll_make_true(ctx);
}

static hll_obj *
uber_car_cdr(hll_ctx *ctx, hll_obj *args, char const *ops) {
    if (hll_list_length(args) != 1) {
        hll_report_error("c%sr must have exactly 1 argument", ops);
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_eval(ctx, hll_unwrap_car(args));

    char const *op = ops + strlen(ops) - 1;
    while (op >= ops && result->kind != HLL_OBJ_NIL) {
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

#define _HLL_CAR_CDR(_letters)                                     \
    hll_obj *hll_std_c##_letters##r(hll_ctx *ctx, hll_obj *args) { \
        return uber_car_cdr(ctx, args, #_letters);                 \
    }
_HLL_ENUMERATE_CAR_CDR
#undef _HLL_CAR_CDR

struct hll_obj *
hll_std_if(struct hll_ctx *ctx, struct hll_obj *args) {
    if (hll_list_length(args) < 2) {
        hll_report_error("If expects at least 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_make_nil(ctx);

    hll_obj *cond = hll_eval(ctx, hll_unwrap_car(args));
    if (cond->kind != HLL_OBJ_NIL) {
        hll_obj *then = hll_unwrap_car(hll_unwrap_cdr(args));
        result = hll_eval(ctx, then);
    } else {
        hll_obj *else_ = hll_unwrap_cdr(hll_unwrap_cdr(args));
        if (else_->kind != HLL_OBJ_NIL) {
            result = hll_eval(ctx, hll_unwrap_car(else_));
        }
    }

    return result;
}

struct hll_obj *
hll_std_cons(struct hll_ctx *ctx, struct hll_obj *args) {
    if (hll_list_length(args) != 2) {
        hll_report_error("cons expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *car = hll_unwrap_car(args);
    hll_obj *cdr = hll_unwrap_car(hll_unwrap_cdr(args));
    return hll_make_cons(ctx, hll_eval(ctx, car), hll_eval(ctx, cdr));
}

struct hll_obj *
hll_std_list(struct hll_ctx *ctx, struct hll_obj *args) {
    hll_obj *head = NULL;
    hll_obj *tail = NULL;

    for (hll_obj *src = args; src->kind != HLL_OBJ_NIL;
         src = hll_unwrap_cdr(src)) {
        hll_obj *tmp = hll_eval(ctx, hll_unwrap_car(src));

        if (head == NULL) {
            head = tail = hll_make_cons(ctx, tmp, hll_make_nil(ctx));
        } else {
            hll_unwrap_cons(tail)->cdr =
                hll_make_cons(ctx, tmp, hll_make_nil(ctx));
            tail = hll_unwrap_cdr(tail);
        }
    }

    if (head == NULL) {
        head = hll_make_nil(ctx);
    }
    return head;
}

struct hll_obj *
hll_std_defun(struct hll_ctx *ctx, struct hll_obj *args) {
    if (hll_list_length(args) < 3) {
        hll_report_error("defun expects at least 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *name = hll_unwrap_car(args);
    if (name->kind != HLL_OBJ_SYMB) {
        hll_report_error("defun name must be a symbol");
        return hll_make_nil(ctx);
    }

    hll_obj *param_list = hll_unwrap_car(hll_unwrap_cdr(args));
    if (param_list->kind != HLL_OBJ_CONS && param_list->kind != HLL_OBJ_NIL) {
        hll_report_error("defun parameter list must be a list");
        return hll_make_nil(ctx);
    }

    for (hll_obj *param = param_list; param->kind != HLL_OBJ_NIL;
         param = hll_unwrap_cdr(param)) {
        if (hll_unwrap_car(param)->kind != HLL_OBJ_SYMB) {
            hll_report_error("defun paramter must be a symbol");
            return hll_make_nil(ctx);
        }
    }

    hll_obj *body = hll_unwrap_cdr(hll_unwrap_cdr(args));
    hll_obj *func = hll_make_func(ctx, ctx->env_stack, param_list, body);

    hll_add_var(ctx, name, func);

    return func;
}

hll_obj *
hll_std_lambda(hll_ctx *ctx, hll_obj *args) {
    if (hll_list_length(args) < 2) {
        hll_report_error("lambda expects at least 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *param_list = hll_unwrap_car(args);
    if (param_list->kind != HLL_OBJ_CONS && param_list->kind != HLL_OBJ_NIL) {
        hll_report_error("lambda parameter list must be a list");
        return hll_make_nil(ctx);
    }

    for (hll_obj *param = param_list; param->kind != HLL_OBJ_NIL;
         param = hll_unwrap_cdr(param)) {
        if (hll_unwrap_car(param)->kind != HLL_OBJ_SYMB) {
            hll_report_error("lambda paramter must be a symbol");
            return hll_make_nil(ctx);
        }
    }

    hll_obj *body = hll_unwrap_cdr(args);
    return hll_make_func(ctx, ctx->env_stack, param_list, body);
}

hll_obj *
hll_std_progn(hll_ctx *ctx, hll_obj *args) {
    hll_obj *result = hll_make_nil(ctx);

    for (hll_obj *expr = args; expr->kind != HLL_OBJ_NIL;
         expr = hll_unwrap_cdr(expr)) {
        result = hll_eval(ctx, hll_unwrap_car(expr));
    }
    return result;
}

STD_FUNC(nth) {
    if (hll_list_length(args) != 2) {
        hll_report_error("nth expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *count_obj = hll_eval(ctx, hll_unwrap_car(args));
    if (count_obj->kind != HLL_OBJ_INT) {
        hll_report_error("nth expects integer count");
        return hll_make_nil(ctx);
    }

    int64_t count = hll_unwrap_int(count_obj)->value;
    if (count < 0) {
        hll_report_error("nth expects nonnegative count");
        return hll_make_nil(ctx);
    }

    hll_obj *list = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    for (; list->kind != HLL_OBJ_NIL && count;
         --count, list = hll_unwrap_cdr(list))
        ;

    hll_obj *result = hll_make_nil(ctx);
    if (list->kind != HLL_OBJ_NIL && count == 0) {
        result = hll_unwrap_car(list);
    }

    return result;
}

STD_FUNC(nthcdr) {
    if (hll_list_length(args) != 2) {
        hll_report_error("nthcdr expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    int64_t count = hll_unwrap_int(hll_unwrap_car(args))->value;
    if (count < 0) {
        hll_report_error("nthcdr expects nonnegative count");
        return hll_make_nil(ctx);
    }

    hll_obj *list = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));
    for (; list->kind != HLL_OBJ_NIL && count;
         --count, list = hll_unwrap_cdr(list))
        ;

    hll_obj *result = hll_make_nil(ctx);
    if (count == 0) {
        result = list;
    }

    return result;
}

STD_FUNC(setcar) {
    if (hll_list_length(args) != 2) {
        hll_report_error("setcar expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *cons = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *expr = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    hll_unwrap_cons(cons)->car = expr;

    return cons;
}

STD_FUNC(setcdr) {
    if (hll_list_length(args) != 2) {
        hll_report_error("setcar expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *cons = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *expr = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    hll_unwrap_cons(cons)->cdr = expr;

    return cons;
}

STD_FUNC(any) {
    if (hll_list_length(args) != 2) {
        hll_report_error("any expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *predicate = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *list = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    for (; list->kind != HLL_OBJ_NIL; list = hll_unwrap_cdr(list)) {
        hll_obj *cdr = hll_unwrap_cdr(list);
        hll_unwrap_cons(list)->cdr = hll_make_nil(ctx);
        hll_obj *test = hll_call(ctx, predicate, list);
        hll_unwrap_cons(list)->cdr = cdr;
        if (test->kind != HLL_OBJ_NIL) {
            return hll_make_true(ctx);
        }
    }

    return hll_make_nil(ctx);
}

STD_FUNC(map) {
    if (hll_list_length(args) != 2) {
        hll_report_error("map expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *fn = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *list = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    hll_obj *head = NULL;
    hll_obj *tail = NULL;

    while (list->kind == HLL_OBJ_CONS) {
        hll_obj *cdr = hll_unwrap_cdr(list);
        hll_unwrap_cons(list)->cdr = hll_make_nil(ctx);
        hll_obj *it = hll_call(ctx, fn, list);
        hll_unwrap_cons(list)->cdr = cdr;
        if (head == NULL) {
            head = tail = hll_make_cons(ctx, it, hll_make_nil(ctx));
        } else {
            hll_unwrap_cons(tail)->cdr =
                hll_make_cons(ctx, it, hll_make_nil(ctx));
            tail = hll_unwrap_cdr(tail);
        }

        list = hll_unwrap_cdr(list);
    }

    if (head == NULL) {
        head = hll_make_nil(ctx);
    }
    return head;
}

STD_FUNC(when) {
    if (hll_list_length(args) < 1) {
        hll_report_error("when expects at least 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_make_nil(ctx);
    hll_obj *predicate = hll_unwrap_car(args);
    if (hll_eval(ctx, predicate)->kind != HLL_OBJ_NIL) {
        result = hll_std_progn(ctx, hll_unwrap_cdr(args));
    }

    return result;
}

STD_FUNC(unless) {
    if (hll_list_length(args) < 1) {
        hll_report_error("unless expects at least 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_make_nil(ctx);
    hll_obj *predicate = hll_unwrap_car(args);
    if (hll_eval(ctx, predicate)->kind == HLL_OBJ_NIL) {
        result = hll_std_progn(ctx, hll_unwrap_cdr(args));
    }

    return result;
}

STD_FUNC(or) {
    hll_obj *list = hll_std_list(ctx, args);
    hll_obj *result = hll_make_nil(ctx);

    for (; list->kind != HLL_OBJ_NIL && result->kind == HLL_OBJ_NIL;
         list = hll_unwrap_cdr(list)) {
        if (hll_unwrap_car(list)->kind != HLL_OBJ_NIL) {
            result = hll_make_true(ctx);
        }
    }

    return result;
}

STD_FUNC(not ) {
    if (hll_list_length(args) != 1) {
        hll_report_error("not expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_make_nil(ctx);
    if (hll_eval(ctx, hll_unwrap_car(args))->kind == HLL_OBJ_NIL) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(and) {
    hll_obj *result = hll_make_true(ctx);

    for (hll_obj *obj = args;
         obj->kind != HLL_OBJ_NIL && result == hll_make_true(ctx);
         obj = hll_unwrap_cdr(obj)) {
        if (hll_eval(ctx, hll_unwrap_car(obj))->kind == HLL_OBJ_NIL) {
            result = hll_make_nil(ctx);
        }
    }

    return result;
}

STD_FUNC(listp) {
    if (hll_list_length(args) != 1) {
        hll_report_error("listp expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *result = hll_make_nil(ctx);

    if (obj->kind == HLL_OBJ_CONS || obj->kind == HLL_OBJ_NIL) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(null) {
    (void)ctx;
    if (hll_list_length(args) != 1) {
        hll_report_error("null expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *obj = hll_unwrap_car(args);
    hll_obj *result = hll_make_nil(ctx);

    if (obj->kind == HLL_OBJ_NIL) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(minusp) {
    if (hll_list_length(args) != 1) {
        hll_report_error("minusp expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    if (obj->kind != HLL_OBJ_INT) {
        hll_report_error("minusp expects integer as argument");
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_make_nil(ctx);

    if (hll_unwrap_int(obj)->value < 0) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(zerop) {
    if (hll_list_length(args) != 1) {
        hll_report_error("zerop expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    if (obj->kind != HLL_OBJ_INT) {
        hll_report_error("zerp expects integer as argument");
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_make_nil(ctx);

    if (hll_unwrap_int(obj)->value == 0) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(plusp) {
    if (hll_list_length(args) != 1) {
        hll_report_error("plusp expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    if (obj->kind != HLL_OBJ_INT) {
        hll_report_error("plusp expects integer as argument");
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_make_nil(ctx);

    if (hll_unwrap_int(obj)->value > 0) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(numberp) {
    if (hll_list_length(args) != 1) {
        hll_report_error("numberp expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *result = hll_make_nil(ctx);

    if (obj->kind == HLL_OBJ_INT) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(append) {
    if (hll_list_length(args) != 2) {
        hll_report_error("append expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *list1 = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *list2 = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    hll_obj *tail = list1;
    for (; hll_unwrap_cdr(tail)->kind != HLL_OBJ_NIL;
         tail = hll_unwrap_cdr(tail))
        ;

    hll_unwrap_cons(tail)->cdr = list2;
    return list1;
}

STD_FUNC(reverse) {
    if (hll_list_length(args) != 1) {
        hll_report_error("reverse expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *result = hll_make_nil(ctx);

    while (obj->kind != HLL_OBJ_NIL) {
        assert(obj->kind == HLL_OBJ_CONS);
        hll_obj *head = obj;
        obj = hll_unwrap_cons(obj)->cdr;
        hll_unwrap_cons(head)->cdr = result;
        result = head;
    }

    return result;
}

STD_FUNC(min) {
    hll_obj *result = hll_make_nil(ctx);
    for (hll_obj *obj = args; obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *test = hll_eval(ctx, hll_unwrap_car(obj));
        if (test->kind != HLL_OBJ_INT) {
            hll_report_error("min expects integer arguments");
            return hll_make_nil(ctx);
        }

        if (result->kind == HLL_OBJ_NIL) {
            result = test;
        } else if (hll_unwrap_int(result)->value >
                   hll_unwrap_int(test)->value) {
            result = test;
        }
    }

    return result;
}

STD_FUNC(max) {
    hll_obj *result = hll_make_nil(ctx);
    for (hll_obj *obj = args; obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *test = hll_eval(ctx, hll_unwrap_car(obj));
        if (test->kind != HLL_OBJ_INT) {
            hll_report_error("max expects integer arguments");
            return hll_make_nil(ctx);
        }

        if (result->kind == HLL_OBJ_NIL) {
            result = test;
        } else if (hll_unwrap_int(result)->value <
                   hll_unwrap_int(test)->value) {
            result = test;
        }
    }

    return result;
}

STD_FUNC(abs) {
    if (hll_list_length(args) != 1) {
        hll_report_error("abs expects exactly 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *obj = hll_unwrap_car(args);
    if (obj->kind != HLL_OBJ_INT) {
        hll_report_error("abs expects integer as argument");
        return hll_make_nil(ctx);
    }

    int64_t value = hll_unwrap_int(obj)->value;
    if (value < 0) {
        value = -value;
    }

    return hll_make_int(ctx, value);
}

STD_FUNC(prin1) {
    FILE *f = ctx->file_out;
    hll_print(f, hll_eval(ctx, hll_unwrap_cons(args)->car));
    return hll_make_nil(ctx);
}

STD_FUNC(read) {
    (void)args;

    enum { BUFFER_SIZE = 4096 };
    char buffer[BUFFER_SIZE];
    char line_buffer[BUFFER_SIZE];

    // TODO: Overflow
    if (fgets(line_buffer, BUFFER_SIZE, stdin) == NULL) {
        hll_report_error("read failed to read from stdin");
        return hll_make_nil(ctx);
    }

    hll_lexer lexer = hll_lexer_create(line_buffer, buffer, BUFFER_SIZE);
    hll_reader reader = hll_reader_create(&lexer, ctx);
    hll_obj *obj = NULL;
    hll_read_result read_result = hll_read(&reader, &obj);

    if (read_result != HLL_READ_OK) {
        obj = hll_make_nil(ctx);
    }

    return obj;
}

STD_FUNC(while) {
    if (hll_list_length(args) < 2) {
        hll_report_error("while expects at least 2 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *condition = hll_unwrap_car(args);
    hll_obj *body = hll_unwrap_cdr(args);
    while (hll_eval(ctx, condition)->kind != HLL_OBJ_NIL) {
        hll_std_list(ctx, body);
    }

    return hll_make_nil(ctx);
}

STD_FUNC(let) {
    if (hll_list_length(args) < 1) {
        hll_report_error("let* expects at least 1 argument");
        return hll_make_nil(ctx);
    }

    hll_obj *vars = hll_make_nil(ctx);
    hll_obj *lets = hll_unwrap_car(args);
    for (hll_obj *let = lets; let->kind != HLL_OBJ_NIL;
         let = hll_unwrap_cdr(let)) {
        hll_obj *pair = hll_unwrap_car(let);
        hll_obj *name = hll_unwrap_car(pair);
        assert(name->kind == HLL_OBJ_SYMB);
        hll_obj *value = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(pair)));

        vars = hll_make_acons(ctx, name, value, vars);
    }

    hll_obj *body = hll_unwrap_cdr(args);

    hll_obj *env = hll_make_env(ctx, ctx->env_stack);
    ctx->env_stack = env;
    hll_unwrap_env(env)->vars = vars;
    hll_obj *result = hll_std_progn(ctx, body);
    ctx->env_stack = hll_unwrap_env(ctx->env_stack)->up;

    return result;
}

STD_FUNC(defvar) {
    if (hll_list_length(args) != 2) {
        hll_report_error("defvar expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *name = hll_unwrap_car(args);
    assert(name->kind == HLL_OBJ_SYMB);
    hll_obj *value = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    for (hll_obj *cons = hll_unwrap_env(ctx->env_stack)->vars;
         cons->kind != HLL_OBJ_NIL; cons = hll_unwrap_cdr(cons)) {
        hll_obj *test = hll_unwrap_cons(cons)->car;
        if (hll_unwrap_cons(test)->car == name) {
            hll_report_error("defvar failed (variable is already defined");
            return hll_make_nil(ctx);
        }
    }

    hll_unwrap_env(ctx->env_stack)->vars =
        hll_make_acons(ctx, name, value, hll_unwrap_env(ctx->env_stack)->vars);

    return name;
}

STD_FUNC(setq) {
    if (hll_list_length(args) != 2) {
        hll_report_error("setq expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }

    hll_obj *name = hll_unwrap_car(args);
    assert(name->kind == HLL_OBJ_SYMB);
    hll_obj *value = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    hll_obj *var = hll_find_var(ctx, name);
    if (var->kind == HLL_OBJ_NIL) {
        hll_report_error("setq failed: there is no such variable");
        return hll_make_nil(ctx);
    }

    hll_unwrap_cons(var)->cdr = value;
    return value;
}

STD_FUNC(rand) {
    (void)args;
    int value = rand();
    return hll_make_int(ctx, value);
}

STD_FUNC(rem) {
    if (hll_list_length(args) != 2) {
        hll_report_error("rem expects exactly 2 arguments");
        return hll_make_nil(ctx);
    }
    hll_obj *x = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *y = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));
    if (x->kind != HLL_OBJ_INT || y->kind != HLL_OBJ_INT) {
        hll_report_error("rem expects integer arguments");
        return hll_make_nil(ctx);
    }
    return hll_make_int(ctx,
                        hll_unwrap_int(x)->value % hll_unwrap_int(y)->value);
}

STD_FUNC(clrscr) {
    (void)ctx;
    (void)args;
    printf("\033[2J");
    return hll_make_nil(ctx);
}

