#include "lisp_std.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error_reporter.h"
#include "lexer.h"
#include "lisp.h"
#include "lisp_gc.h"
#include "reader.h"

static char const *
get_lisp_function_name(char const *name) {
    char const *result = NULL;
    static struct {
        char const *c_name;
        char const *lisp_name;
    } const functions[] = {
#define _HLL_CAR_CDR _HLL_CAR_CDR_STD_FUNC
#define _HLL_STD_FUNC_(_c, _lisp) { #_c, _lisp },
#define _HLL_STD_FUNC(_name, _lisp) _HLL_STD_FUNC_(hll_std_##_name, _lisp)
        _HLL_ENUMERATE_STD_FUNCS
#undef _HLL_STD_FUNC_
#undef _HLL_STD_FUNC
#undef _HLL_CAR_CDR
    };

    size_t count = sizeof(functions) / sizeof(functions[0]);
    for (size_t i = 0; i < count && result == NULL; ++i) {
        if (0 == strcmp(functions[i].c_name, name)) {
            result = functions[i].lisp_name;
        }
    }

#if HLL_DEBUG
    if (result == NULL) {
        fprintf(stderr,
                "Internal error: failed to get builtin function name: %s\n",
                name);
    }
#endif

    assert(result != NULL);
    return result;
}

#define REPORT(...)                           \
    do {                                      \
        if (!ctx->reporter->has_error) {      \
            hll_report_error(__VA_ARGS__);    \
            hll_print_error_stack_trace(ctx); \
        }                                     \
    } while (0);

#define STD_FUNC(_name) hll_obj *hll_std_##_name(hll_ctx *ctx, hll_obj *args)

#define ITER(_obj, _head)                                  \
    for (hll_obj *_obj = _head; _obj->kind != HLL_OBJ_NIL; \
         _obj = hll_unwrap_cdr(_obj))

#define CHECK_TYPE(_obj, _kind, _msg)                                        \
    do {                                                                     \
        hll_obj *__temp = (_obj);                                            \
        if (__temp->kind != (_kind)) {                                       \
            REPORT(ctx->reporter, "%s " _msg " must be of type %s (got %s)", \
                   get_lisp_function_name(__func__),                         \
                   hll_get_obj_kind_str(_kind),                              \
                   hll_get_obj_kind_str(__temp->kind));                      \
            return hll_make_nil(ctx);                                        \
        }                                                                    \
    } while (0)

#define CHECK_TYPE_LIST(_obj, _msg)                                            \
    do {                                                                       \
        hll_obj *__temp = (_obj);                                              \
        if (__temp->kind != HLL_OBJ_CONS && __temp->kind != HLL_OBJ_NIL) {     \
            REPORT(ctx->reporter, "%s " _msg " must be of list type (got %s)", \
                   get_lisp_function_name(__func__),                           \
                   hll_get_obj_kind_str((_obj)->kind));                        \
            return hll_make_nil(ctx);                                          \
        }                                                                      \
    } while (0)

#define CHECK_HAS_N_ARGS(_n)                                                   \
    do {                                                                       \
        if (hll_list_length(args) != (_n)) {                                   \
            REPORT(ctx->reporter,                                              \
                   "%s must have exactly " #_n " argument%s (got %zu)",        \
                   get_lisp_function_name(__func__), (((_n) == 1) ? "" : "s"), \
                   hll_list_length(args));                                     \
            return hll_make_nil(ctx);                                          \
        }                                                                      \
    } while (0)

#define CHECK_HAS_ATLEAST_N_ARGS(_n)                                          \
    do {                                                                      \
        if (hll_list_length(args) < (_n)) {                                   \
            REPORT(ctx->reporter, "%s must have at least " #_n " argument%s", \
                   get_lisp_function_name(__func__),                          \
                   (((_n) == 1) ? "" : "s"));                                 \
            return hll_make_nil(ctx);                                         \
        }                                                                     \
    } while (0)

STD_FUNC(print) {
    FILE *f = ctx->file_out;
    hll_print(ctx, f, hll_eval(ctx, hll_unwrap_cons(args)->car));
    fprintf(f, "\n");
    return hll_make_nil(ctx);
}

STD_FUNC(add) {
    int64_t result = 0;
    ITER(obj, args) {
        hll_obj *value = hll_eval(ctx, hll_unwrap_car(obj));
        CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
        result += hll_unwrap_int(value)->value;
    }
    return hll_make_int(ctx, result);
}

STD_FUNC(sub) {
    CHECK_HAS_ATLEAST_N_ARGS(1);
    hll_obj *first = hll_eval(ctx, hll_unwrap_car(args));
    CHECK_TYPE(first, HLL_OBJ_INT, "arguments");
    int64_t result = hll_unwrap_int(first)->value;
    ITER(obj, hll_unwrap_cdr(args)) {
        hll_obj *value = hll_eval(ctx, hll_unwrap_car(obj));
        CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
        result -= hll_unwrap_int(value)->value;
    }
    return hll_make_int(ctx, result);
}

STD_FUNC(div) {
    CHECK_HAS_ATLEAST_N_ARGS(1);
    hll_obj *first = hll_eval(ctx, hll_unwrap_car(args));
    CHECK_TYPE(first, HLL_OBJ_INT, "arguments");
    int64_t result = hll_unwrap_int(first)->value;
    ITER(obj, hll_unwrap_cdr(args)) {
        hll_obj *value = hll_eval(ctx, hll_unwrap_car(obj));
        CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
        result /= hll_unwrap_int(value)->value;
    }
    return hll_make_int(ctx, result);
}

STD_FUNC(mul) {
    int64_t result = 1;
    ITER(obj, args) {
        hll_obj *value = hll_eval(ctx, hll_unwrap_car(obj));
        CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
        result *= hll_unwrap_int(value)->value;
    }
    return hll_make_int(ctx, result);
}

STD_FUNC(quote) {
    CHECK_HAS_N_ARGS(1);
    return hll_unwrap_cons(args)->car;
}

STD_FUNC(eval) {
    CHECK_HAS_N_ARGS(1);
    return hll_eval(ctx, hll_eval(ctx, hll_unwrap_car(args)));
}

STD_FUNC(int_ne) {
    CHECK_HAS_ATLEAST_N_ARGS(1);

    args = hll_std_list(ctx, args);
    ITER(obj1, args) {
        hll_obj *num1 = hll_unwrap_car(obj1);
        CHECK_TYPE(num1, HLL_OBJ_INT, "arguments");

        ITER(obj2, hll_unwrap_cdr(obj1)) {
            if (obj1 == obj2) {
                continue;
            }

            hll_obj *num2 = hll_unwrap_car(obj2);
            CHECK_TYPE(num2, HLL_OBJ_INT, "arguments");

            if (hll_unwrap_int(num1)->value == hll_unwrap_int(num2)->value) {
                return hll_make_nil(ctx);
            }
        }
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_eq(hll_ctx *ctx, hll_obj *args) {
    CHECK_HAS_ATLEAST_N_ARGS(1);

    args = hll_std_list(ctx, args);
    ITER(obj1, args) {
        hll_obj *num1 = hll_unwrap_car(obj1);
        CHECK_TYPE(num1, HLL_OBJ_INT, "arguments");

        ITER(obj2, hll_unwrap_cdr(obj1)) {
            if (obj1 == obj2) {
                continue;
            }

            hll_obj *num2 = hll_unwrap_car(obj2);
            CHECK_TYPE(num2, HLL_OBJ_INT, "arguments");

            if (hll_unwrap_int(num1)->value != hll_unwrap_int(num2)->value) {
                return hll_make_nil(ctx);
            }
        }
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_gt(hll_ctx *ctx, hll_obj *args) {
    CHECK_HAS_ATLEAST_N_ARGS(1);

    args = hll_std_list(ctx, args);

    hll_obj *prev = hll_unwrap_car(args);
    CHECK_TYPE(prev, HLL_OBJ_INT, "arguments");

    ITER(obj, hll_unwrap_cdr(args)) {
        hll_obj *num = hll_unwrap_car(obj);
        CHECK_TYPE(num, HLL_OBJ_INT, "arguments");

        if (hll_unwrap_int(prev)->value <= hll_unwrap_int(num)->value) {
            return hll_make_nil(ctx);
        }

        prev = num;
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_ge(hll_ctx *ctx, hll_obj *args) {
    CHECK_HAS_ATLEAST_N_ARGS(1);

    args = hll_std_list(ctx, args);

    hll_obj *prev = hll_unwrap_car(args);
    CHECK_TYPE(prev, HLL_OBJ_INT, "arguments");

    for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        CHECK_TYPE(num, HLL_OBJ_INT, "arguments");

        if (hll_unwrap_int(prev)->value < hll_unwrap_int(num)->value) {
            return hll_make_nil(ctx);
        }
        prev = num;
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_lt(hll_ctx *ctx, hll_obj *args) {
    CHECK_HAS_ATLEAST_N_ARGS(1);

    args = hll_std_list(ctx, args);

    hll_obj *prev = hll_unwrap_car(args);
    CHECK_TYPE(prev, HLL_OBJ_INT, "arguments");

    for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        CHECK_TYPE(num, HLL_OBJ_INT, "arguments");

        if (hll_unwrap_int(prev)->value >= hll_unwrap_int(num)->value) {
            return hll_make_nil(ctx);
        }
        prev = num;
    }

    return hll_make_true(ctx);
}

hll_obj *
hll_std_int_le(hll_ctx *ctx, hll_obj *args) {
    CHECK_HAS_ATLEAST_N_ARGS(1);

    args = hll_std_list(ctx, args);

    hll_obj *prev = hll_unwrap_car(args);
    CHECK_TYPE(prev, HLL_OBJ_INT, "arguments");

    for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
         obj = hll_unwrap_cdr(obj)) {
        hll_obj *num = hll_unwrap_car(obj);
        CHECK_TYPE(num, HLL_OBJ_INT, "arguments");

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
        hll_report_error(ctx->reporter, "c%sr must have exactly 1 argument",
                         ops);
        return hll_make_nil(ctx);
    }

    hll_obj *result = hll_eval(ctx, hll_unwrap_car(args));

    char const *op = ops + strlen(ops) - 1;
    while (op >= ops && result->kind != HLL_OBJ_NIL) {
        if (result->kind != HLL_OBJ_CONS) {
            hll_report_error(ctx->reporter,
                             "c%sr argument must be well-formed list", ops);
            return hll_make_nil(ctx);
        }

        if (*op == 'a') {
            result = hll_unwrap_car(result);
        } else if (*op == 'd') {
            result = hll_unwrap_cdr(result);
        } else {
            HLL_UNREACHABLE;
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
    CHECK_HAS_ATLEAST_N_ARGS(2);

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
    CHECK_HAS_N_ARGS(2);

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
    CHECK_HAS_ATLEAST_N_ARGS(3);

    hll_obj *name = hll_unwrap_car(args);
    CHECK_TYPE(name, HLL_OBJ_SYMB, "name");

    hll_obj *param_list = hll_unwrap_car(hll_unwrap_cdr(args));
    CHECK_TYPE_LIST(param_list, "parameter list");

    ITER(param, param_list) {
        CHECK_TYPE(hll_unwrap_car(param), HLL_OBJ_SYMB, "parameters");
    }

    // This has to be done because we enclose each builtin function in its own
    // scope.
    hll_obj *env = hll_unwrap_env(ctx->env)->up;

    hll_obj *body = hll_unwrap_cdr(hll_unwrap_cdr(args));
    hll_obj *func = hll_make_func(ctx, env, param_list, body);
    hll_unwrap_func(func)->meta.name = hll_unwrap_symb(name)->symb;
    hll_add_var(ctx, env, name, func);

    return func;
}

hll_obj *
hll_std_lambda(hll_ctx *ctx, hll_obj *args) {
    CHECK_HAS_ATLEAST_N_ARGS(2);

    hll_obj *param_list = hll_unwrap_car(args);
    CHECK_TYPE_LIST(param_list, "parameter list");

    ITER(param, param_list) {
        CHECK_TYPE(hll_unwrap_car(param), HLL_OBJ_SYMB, "parameters");
    }

    // This has to be done because we enclose each builtin function in its own
    // scope.
    hll_obj *env = hll_unwrap_env(ctx->env)->up;

    hll_obj *body = hll_unwrap_cdr(args);
    hll_obj *result = hll_make_func(ctx, env, param_list, body);
    hll_unwrap_func(result)->meta.name = "lambda";
    return result;
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
    CHECK_HAS_N_ARGS(2);

    hll_obj *count_obj = hll_eval(ctx, hll_unwrap_car(args));
    CHECK_TYPE(count_obj, HLL_OBJ_INT, "count");

    int64_t count = hll_unwrap_int(count_obj)->value;
    if (count < 0) {
        hll_report_error(ctx->reporter, "nth expects nonnegative count");
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
    CHECK_HAS_N_ARGS(2);

    int64_t count = hll_unwrap_int(hll_unwrap_car(args))->value;
    if (count < 0) {
        hll_report_error(ctx->reporter, "nthcdr expects nonnegative count");
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
    CHECK_HAS_N_ARGS(2);

    hll_obj *cons = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *expr = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    hll_unwrap_cons(cons)->car = expr;

    return cons;
}

STD_FUNC(setcdr) {
    CHECK_HAS_N_ARGS(2);

    hll_obj *cons = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *expr = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    hll_unwrap_cons(cons)->cdr = expr;

    return cons;
}

STD_FUNC(any) {
    CHECK_HAS_N_ARGS(2);

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
    CHECK_HAS_N_ARGS(2);

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
    CHECK_HAS_ATLEAST_N_ARGS(1);

    hll_obj *result = hll_make_nil(ctx);
    hll_obj *predicate = hll_unwrap_car(args);
    if (hll_eval(ctx, predicate)->kind != HLL_OBJ_NIL) {
        result = hll_std_progn(ctx, hll_unwrap_cdr(args));
    }

    return result;
}

STD_FUNC(unless) {
    CHECK_HAS_ATLEAST_N_ARGS(1);

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
    CHECK_HAS_N_ARGS(1);

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
    CHECK_HAS_N_ARGS(1);

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *result = hll_make_nil(ctx);

    if (obj->kind == HLL_OBJ_CONS || obj->kind == HLL_OBJ_NIL) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(null) {
    CHECK_HAS_N_ARGS(1);

    hll_obj *obj = hll_unwrap_car(args);
    hll_obj *result = hll_make_nil(ctx);

    if (obj->kind == HLL_OBJ_NIL) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(minusp) {
    CHECK_HAS_N_ARGS(1);

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    CHECK_TYPE(obj, HLL_OBJ_INT, "argument");

    hll_obj *result = hll_make_nil(ctx);

    if (hll_unwrap_int(obj)->value < 0) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(zerop) {
    CHECK_HAS_N_ARGS(1);

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    CHECK_TYPE(obj, HLL_OBJ_INT, "argument");

    hll_obj *result = hll_make_nil(ctx);

    if (hll_unwrap_int(obj)->value == 0) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(plusp) {
    CHECK_HAS_N_ARGS(1);

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    CHECK_TYPE(obj, HLL_OBJ_INT, "argument");

    hll_obj *result = hll_make_nil(ctx);

    if (hll_unwrap_int(obj)->value > 0) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(numberp) {
    CHECK_HAS_N_ARGS(1);

    hll_obj *obj = hll_eval(ctx, hll_unwrap_car(args));
    hll_obj *result = hll_make_nil(ctx);

    if (obj->kind == HLL_OBJ_INT) {
        result = hll_make_true(ctx);
    }

    return result;
}

STD_FUNC(append) {
    CHECK_HAS_N_ARGS(2);

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
    CHECK_HAS_N_ARGS(1);

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
        CHECK_TYPE(test, HLL_OBJ_INT, "arguments");

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
        CHECK_TYPE(test, HLL_OBJ_INT, "arguments");

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
    CHECK_HAS_N_ARGS(1);

    hll_obj *obj = hll_unwrap_car(args);
    CHECK_TYPE(obj, HLL_OBJ_INT, "argument");

    int64_t value = hll_unwrap_int(obj)->value;
    if (value < 0) {
        value = -value;
    }

    return hll_make_int(ctx, value);
}

STD_FUNC(prin1) {
    CHECK_HAS_N_ARGS(1);

    FILE *f = ctx->file_out;
    hll_print(ctx, f, hll_eval(ctx, hll_unwrap_cons(args)->car));
    return hll_make_nil(ctx);
}

STD_FUNC(read) {
    CHECK_HAS_N_ARGS(0);

    enum { BUFFER_SIZE = 4096 };
    char buffer[BUFFER_SIZE];
    char line_buffer[BUFFER_SIZE];

    // TODO: Overflow
    if (fgets(line_buffer, BUFFER_SIZE, stdin) == NULL) {
        hll_report_error(ctx->reporter, "read failed to read from stdin");
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
    CHECK_HAS_ATLEAST_N_ARGS(2);

    hll_obj *condition = hll_unwrap_car(args);
    hll_obj *body = hll_unwrap_cdr(args);
    while (hll_eval(ctx, condition)->kind != HLL_OBJ_NIL) {
        hll_std_list(ctx, body);
    }

    return hll_make_nil(ctx);
}

STD_FUNC(let) {
    CHECK_HAS_ATLEAST_N_ARGS(1);

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

    hll_obj *env = hll_make_env(ctx, ctx->env);
    ctx->env = env;
    hll_unwrap_env(env)->vars = vars;
    hll_obj *result = hll_std_progn(ctx, body);
    ctx->env = hll_unwrap_env(ctx->env)->up;

    return result;
}

STD_FUNC(defvar) {
    CHECK_HAS_N_ARGS(2);

    hll_obj *name = hll_unwrap_car(args);
    assert(name->kind == HLL_OBJ_SYMB);
    hll_obj *value = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    for (hll_obj *cons = hll_unwrap_env(ctx->env)->vars;
         cons->kind != HLL_OBJ_NIL; cons = hll_unwrap_cdr(cons)) {
        hll_obj *test = hll_unwrap_cons(cons)->car;
        if (hll_unwrap_cons(test)->car == name) {
            hll_report_error(ctx->reporter,
                             "defvar failed (variable is already defined");
            return hll_make_nil(ctx);
        }
    }

    // This has to be done because we enclose each builtin function in its own
    // scope.
    hll_obj *env = hll_unwrap_env(ctx->env)->up;
    hll_add_var(ctx, env, name, value);

    return name;
}

STD_FUNC(setq) {
    CHECK_HAS_N_ARGS(2);

    hll_obj *name = hll_unwrap_car(args);
    assert(name->kind == HLL_OBJ_SYMB);
    hll_obj *value = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));

    hll_obj *var = hll_find_var(ctx, name);
    if (var->kind == HLL_OBJ_NIL) {
        hll_report_error(ctx->reporter,
                         "setq failed: there is no such variable");
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
    CHECK_HAS_N_ARGS(2);

    hll_obj *x = hll_eval(ctx, hll_unwrap_car(args));
    CHECK_TYPE(x, HLL_OBJ_INT, "dividend");
    hll_obj *y = hll_eval(ctx, hll_unwrap_car(hll_unwrap_cdr(args)));
    CHECK_TYPE(y, HLL_OBJ_INT, "divisor");

    return hll_make_int(ctx,
                        hll_unwrap_int(x)->value % hll_unwrap_int(y)->value);
}

STD_FUNC(clrscr) {
    (void)ctx;
    (void)args;
    printf("\033[2J");
    return hll_make_nil(ctx);
}

