#include "lisp.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "lisp_gc.h"
#include "lisp_std.h"

typedef struct {
    hll_obj head;
    union {
        hll_cons cons;
        hll_symb symb;
        hll_func func;
        hll_int integer;
        hll_bind bind;
        hll_env env;
    } body;
} lisp_obj;

static lisp_obj hll_nil_ = { .head = {
                                 .kind = HLL_OBJ_NIL,
                                 .size = 0,
                             } };
hll_obj *hll_nil = &hll_nil_.head;

static lisp_obj hll_true_ = { .head = { .kind = HLL_OBJ_TRUE, .size = 0 } };
hll_obj *hll_true = &hll_true_.head;

void
hll_report_error(hll_ctx *ctx, char const *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(ctx->file_outerr, format, args);
    fprintf(ctx->file_outerr, "\n");
}

hll_cons *
hll_unwrap_cons(hll_obj *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_OBJ_CONS);
    return &obj->body.cons;
}

hll_obj *
hll_unwrap_car(hll_obj *obj) {
    return hll_unwrap_cons(obj)->car;
}

hll_obj *
hll_unwrap_cdr(hll_obj *obj) {
    return hll_unwrap_cons(obj)->cdr;
}

hll_symb *
hll_unwrap_symb(hll_obj *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_OBJ_SYMB);
    return &obj->body.symb;
}

hll_int *
hll_unwrap_int(hll_obj *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_OBJ_INT);
    return &obj->body.integer;
}

hll_bind *
hll_unwrap_bind(hll_obj *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_OBJ_BIND);
    return &obj->body.bind;
}

hll_env *
hll_unwrap_env(hll_obj *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_OBJ_ENV);
    return &obj->body.env;
}

hll_obj *
hll_make_cons(hll_ctx *ctx, hll_obj *car, hll_obj *cdr) {
    (void)ctx;
    hll_obj *cons = hll_alloc(sizeof(hll_cons), HLL_OBJ_CONS);

    hll_unwrap_cons(cons)->car = car;
    hll_unwrap_cons(cons)->cdr = cdr;

    return cons;
}

hll_obj *
hll_make_env(hll_ctx *ctx, hll_obj *up) {
    (void)ctx;
    hll_obj *env = hll_alloc(sizeof(hll_env), HLL_OBJ_ENV);

    hll_unwrap_env(env)->up = up;

    return env;
}

hll_obj *
hll_make_acons(hll_ctx *ctx, hll_obj *x, hll_obj *y, hll_obj *a) {
    return hll_make_cons(ctx, hll_make_cons(ctx, x, y), a);
}

hll_obj *
hll_make_symb(hll_ctx *ctx, char const *data, size_t length) {
    (void)ctx;
    hll_obj *symb = hll_alloc(sizeof(hll_symb) + length + 1, HLL_OBJ_SYMB);

    char *string = (char *)symb + sizeof(hll_obj) + sizeof(hll_symb);
    strncpy(string, data, length);
    hll_unwrap_symb(symb)->symb = string;
    hll_unwrap_symb(symb)->length = length;

    return symb;
}

hll_obj *
hll_make_int(hll_ctx *ctx, int64_t value) {
    (void)ctx;
    hll_obj *integer = hll_alloc(sizeof(hll_int), HLL_OBJ_INT);
    hll_unwrap_int(integer)->value = value;
    return integer;
}

static hll_obj *
hll_make_binding(hll_ctx *ctx, hll_bind_func *bind) {
    (void)ctx;
    hll_obj *binding = hll_alloc(sizeof(hll_obj), HLL_OBJ_BIND);
    hll_unwrap_bind(binding)->bind = bind;
    return binding;
}

void
hll_add_binding(hll_ctx *ctx, hll_bind_func *bind, char const *symbol,
                size_t length) {
    hll_obj *binding = hll_make_binding(ctx, bind);
    hll_obj *symb = hll_find_symb(ctx, symbol, length);
    assert(ctx->env_stack != hll_nil);
    hll_unwrap_env(ctx->env_stack)->vars = hll_make_acons(
        ctx, symb, binding, hll_unwrap_env(ctx->env_stack)->vars);
}

hll_obj *
hll_reverse_list(hll_obj *obj) {
    hll_obj *result = hll_nil;

    while (obj != hll_nil) {
        assert(obj->kind == HLL_OBJ_CONS);
        hll_obj *head = obj;
        obj = hll_unwrap_cons(obj)->cdr;
        hll_unwrap_cons(head)->cdr = result;
        result = head;
    }

    return result;
}

hll_obj *
hll_find_symb(hll_ctx *ctx, char const *data, size_t length) {
    hll_obj *found = NULL;

    for (hll_obj *obj = ctx->symbols; obj != hll_nil && found == NULL;
         obj = hll_unwrap_cons(obj)->cdr) {
        assert(obj->kind == HLL_OBJ_CONS);
        hll_obj *symb = hll_unwrap_cons(obj)->car;

        if (strcmp(hll_unwrap_symb(symb)->symb, data) == 0) {
            found = obj;
        }
    }

    if (found == NULL) {
        found = hll_make_symb(ctx, data, length);
        ctx->symbols = hll_make_cons(ctx, found, ctx->symbols);
    } else {
        found = hll_unwrap_cons(found)->car;
    }

    return found;
}

hll_obj *
hll_find_var(hll_ctx *ctx, hll_obj *car) {
    hll_obj *result = NULL;

    for (hll_obj *env_frame = ctx->env_stack;
         env_frame != hll_nil && result == NULL;
         env_frame = hll_unwrap_env(env_frame)->up) {
        for (hll_obj *cons = hll_unwrap_env(env_frame)->vars;
             cons != hll_nil && result == NULL;
             cons = hll_unwrap_cons(cons)->cdr) {
            hll_obj *test = hll_unwrap_cons(cons)->car;
            if (hll_unwrap_cons(test)->car == car) {
                result = test;
            }
        }
    }

    return result;
}

void
hll_print(void *file_, hll_obj *obj) {
    FILE *file = file_;

    switch (obj->kind) {
    default:
        assert(0);
        break;
    case HLL_OBJ_CONS:
        fprintf(file, "(");
        while (obj != hll_nil) {
            assert(obj->kind == HLL_OBJ_CONS);
            hll_print(file, hll_unwrap_cons(obj)->car);

            hll_obj *cdr = hll_unwrap_cons(obj)->cdr;
            if (cdr != hll_nil && cdr->kind != HLL_OBJ_CONS) {
                fprintf(file, " . ");
                hll_print(file, cdr);
                cdr = hll_nil;
            } else if (cdr != hll_nil) {
                fprintf(file, " ");
            }

            obj = cdr;
        }
        fprintf(file, ")");
        break;
    case HLL_OBJ_SYMB: {
        fprintf(file, "%s", hll_unwrap_symb(obj)->symb);
    } break;
    case HLL_OBJ_INT:
        fprintf(file, "%" PRId64 "", hll_unwrap_int(obj)->value);
        break;
    case HLL_OBJ_BIND:
        fprintf(file, "<c-binding>");
        break;
    case HLL_OBJ_NIL:
        fprintf(file, "()");
        break;
    case HLL_OBJ_TRUE:
        fprintf(file, "t");
        break;
    }
}

static hll_obj *
call(hll_ctx *ctx, hll_obj *fn, hll_obj *args) {
    hll_obj *result = hll_nil;

    switch (fn->kind) {
    default:
        fprintf(stderr, "Unsupported callable type\n");
        break;
    case HLL_OBJ_BIND:
        result = hll_unwrap_bind(fn)->bind(ctx, args);
        break;
    }

    return result;
}

hll_obj *
hll_eval(hll_ctx *ctx, hll_obj *obj) {
    hll_obj *result = hll_nil;

    switch (obj->kind) {
    default:
        assert(0);
        break;
    case HLL_OBJ_BIND:
    case HLL_OBJ_NIL:
    case HLL_OBJ_INT:
        result = obj;
        break;
    case HLL_OBJ_SYMB: {
        hll_obj *var = hll_find_var(ctx, obj);
        if (var == NULL) {
            fprintf(stderr, "Undefined variable '%s'\n",
                    hll_unwrap_symb(obj)->symb);
        } else {
            result = hll_unwrap_cons(var)->cdr;
        }
    } break;
    case HLL_OBJ_CONS: {
        // Car should be function
        hll_obj *fn = hll_eval(ctx, hll_unwrap_cons(obj)->car);
        hll_obj *args = hll_unwrap_cons(obj)->cdr;
        result = call(ctx, fn, args);
    } break;
    }

    return result;
}

size_t
hll_list_length(hll_obj *obj) {
    size_t result = 0;

    while (obj->kind == HLL_OBJ_CONS) {
        ++result;
        obj = hll_unwrap_cons(obj)->cdr;
    }

    return result;
}

hll_ctx
hll_create_ctx(void) {
    hll_ctx ctx = { 0 };

    ctx.symbols = hll_nil;
    ctx.file_out = stdout;
    ctx.file_outerr = stderr;
    ctx.env_stack = hll_make_env(&ctx, hll_nil);

#define STR_LEN(_str) _str, (sizeof(_str) - 1)
#define BIND(_func, _symb) hll_add_binding(&ctx, _func, STR_LEN(_symb))
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
    hll_unwrap_env(ctx.env_stack)->vars =
        hll_make_acons(&ctx, hll_find_symb(&ctx, STR_LEN("t")), hll_true,
                       hll_unwrap_env(ctx.env_stack)->vars);
#undef BIND
#undef STR_LEN

    return ctx;
}

void
hll_dump_object_desc(void *file, hll_obj *obj) {
    switch (obj->kind) {
    case HLL_OBJ_NONE:
        fprintf(file, "lobj<kind=None>");
        break;
    case HLL_OBJ_TRUE:
        fprintf(file, "lobj<kind=True>");
        break;
    case HLL_OBJ_CONS:
        fprintf(file, "lobj<kind=Cons car=");
        hll_dump_object_desc(file, hll_unwrap_cons(obj)->car);
        fprintf(file, " cdr=");
        hll_dump_object_desc(file, hll_unwrap_cons(obj)->cdr);
        fprintf(file, ">");
        break;
    case HLL_OBJ_SYMB:
        fprintf(file, "lobj<kind=Symb symb='%s' len=%zu>",
                hll_unwrap_symb(obj)->symb, hll_unwrap_symb(obj)->length);
        break;
    case HLL_OBJ_INT:
        fprintf(file, "lobj<kind=Int value=%" PRIi64 ">",
                hll_unwrap_int(obj)->value);
        break;
    case HLL_OBJ_BIND:
        fprintf(file, "lobj<kind=Bind ptr=%zu>",
                (uintptr_t)hll_unwrap_bind(obj)->bind);
        break;
    case HLL_OBJ_ENV:
        fprintf(file, "lobj<kind=Env vars=");
        hll_dump_object_desc(file, hll_unwrap_env(obj)->vars);
        fprintf(file, " up=");
        hll_dump_object_desc(file, hll_unwrap_env(obj)->up);
        fprintf(file, ">");
        break;
    case HLL_OBJ_NIL:
        fprintf(file, "lobj<kind=Nil>");
        break;
    }
}
