#include "lisp.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "lisp_gc.h"

typedef struct {
    hll_lisp_obj_head head;
    union {
        hll_lisp_cons cons;
        hll_lisp_symb symb;
        hll_lisp_func func;
        hll_lisp_int integer;
        hll_lisp_bind bind;
        hll_lisp_env env;
    } body;
} lisp_obj;

static lisp_obj hll_nil_ = { .head = {
                                 .kind = HLL_LOBJ_NIL,
                                 .size = 0,
                             } };
hll_lisp_obj_head *hll_nil = &hll_nil_.head;

static lisp_obj hll_true_ = { .head = { .kind = HLL_LOBJ_TRUE, .size = 0 } };
hll_lisp_obj_head *hll_true = &hll_true_.head;

void
hll_report_error(hll_lisp_ctx *ctx, char const *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(ctx->file_outerr, format, args);
    fprintf(ctx->file_outerr, "\n");
}

hll_lisp_cons *
hll_unwrap_cons(hll_lisp_obj_head *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_LOBJ_CONS);
    return &obj->body.cons;
}

hll_lisp_symb *
hll_unwrap_symb(hll_lisp_obj_head *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_LOBJ_SYMB);
    return &obj->body.symb;
}

hll_lisp_int *
hll_unwrap_int(hll_lisp_obj_head *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_LOBJ_INT);
    return &obj->body.integer;
}

hll_lisp_bind *
hll_unwrap_bind(hll_lisp_obj_head *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_LOBJ_BIND);
    return &obj->body.bind;
}

hll_lisp_env *
hll_unwrap_env(hll_lisp_obj_head *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_LOBJ_ENV);
    return &obj->body.env;
}

hll_lisp_obj_head *
hll_make_cons(hll_lisp_ctx *ctx, hll_lisp_obj_head *car,
              hll_lisp_obj_head *cdr) {
    (void)ctx;
    hll_lisp_obj_head *cons = hll_alloc(sizeof(hll_lisp_cons), HLL_LOBJ_CONS);

    hll_unwrap_cons(cons)->car = car;
    hll_unwrap_cons(cons)->cdr = cdr;

    return cons;
}

hll_lisp_obj_head *
hll_make_env(hll_lisp_ctx *ctx, hll_lisp_obj_head *up) {
    (void)ctx;
    hll_lisp_obj_head *env = hll_alloc(sizeof(hll_lisp_env), HLL_LOBJ_ENV);

    hll_unwrap_env(env)->up = up;

    return env;
}

hll_lisp_obj_head *
hll_make_acons(hll_lisp_ctx *ctx, hll_lisp_obj_head *x, hll_lisp_obj_head *y,
               hll_lisp_obj_head *a) {
    return hll_make_cons(ctx, hll_make_cons(ctx, x, y), a);
}

hll_lisp_obj_head *
hll_make_symb(hll_lisp_ctx *ctx, char const *data, size_t length) {
    (void)ctx;
    hll_lisp_obj_head *symb =
        hll_alloc(sizeof(hll_lisp_symb) + length + 1, HLL_LOBJ_SYMB);

    char *string =
        (char *)symb + sizeof(hll_lisp_obj_head) + sizeof(hll_lisp_symb);
    strncpy(string, data, length);
    hll_unwrap_symb(symb)->symb = string;
    hll_unwrap_symb(symb)->length = length;

    return symb;
}

hll_lisp_obj_head *
hll_make_int(hll_lisp_ctx *ctx, int64_t value) {
    (void)ctx;
    hll_lisp_obj_head *integer = hll_alloc(sizeof(hll_lisp_int), HLL_LOBJ_INT);
    hll_unwrap_int(integer)->value = value;
    return integer;
}

static hll_lisp_obj_head *
hll_make_binding(hll_lisp_ctx *ctx, hll_lisp_bind_func *bind) {
    (void)ctx;
    hll_lisp_obj_head *binding =
        hll_alloc(sizeof(hll_lisp_obj_head), HLL_LOBJ_BIND);
    hll_unwrap_bind(binding)->bind = bind;
    return binding;
}

void
hll_add_binding(hll_lisp_ctx *ctx, hll_lisp_bind_func *bind, char const *symbol,
                size_t length) {
    hll_lisp_obj_head *binding = hll_make_binding(ctx, bind);
    hll_lisp_obj_head *symb = hll_find_symb(ctx, symbol, length);
    assert(ctx->env_stack != hll_nil);
    hll_unwrap_env(ctx->env_stack)->vars = hll_make_acons(
        ctx, symb, binding, hll_unwrap_env(ctx->env_stack)->vars);
}

hll_lisp_obj_head *
hll_reverse_list(hll_lisp_obj_head *obj) {
    hll_lisp_obj_head *result = hll_nil;

    while (obj != hll_nil) {
        assert(obj->kind == HLL_LOBJ_CONS);
        hll_lisp_obj_head *head = obj;
        obj = hll_unwrap_cons(obj)->cdr;
        hll_unwrap_cons(head)->cdr = result;
        result = head;
    }

    return result;
}

hll_lisp_obj_head *
hll_find_symb(hll_lisp_ctx *ctx, char const *data, size_t length) {
    hll_lisp_obj_head *found = NULL;

    for (hll_lisp_obj_head *obj = ctx->symbols; obj != hll_nil && found == NULL;
         obj = hll_unwrap_cons(obj)->cdr) {
        assert(obj->kind == HLL_LOBJ_CONS);
        hll_lisp_obj_head *symb = hll_unwrap_cons(obj)->car;

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

hll_lisp_obj_head *
hll_find_var(hll_lisp_ctx *ctx, hll_lisp_obj_head *car) {
    hll_lisp_obj_head *result = NULL;

    for (hll_lisp_obj_head *env_frame = ctx->env_stack;
         env_frame != hll_nil && result == NULL;
         env_frame = hll_unwrap_env(env_frame)->up) {
        for (hll_lisp_obj_head *cons = hll_unwrap_env(env_frame)->vars;
             cons != hll_nil && result == NULL;
             cons = hll_unwrap_cons(cons)->cdr) {
            hll_lisp_obj_head *test = hll_unwrap_cons(cons)->car;
            if (hll_unwrap_cons(test)->car == car) {
                result = test;
            }
        }
    }

    return result;
}

void
hll_lisp_print(void *file_, hll_lisp_obj_head *obj) {
    FILE *file = file_;

    switch (obj->kind) {
    default:
        assert(0);
        break;
    case HLL_LOBJ_CONS:
        fprintf(file, "(");
        while (obj != hll_nil) {
            assert(obj->kind == HLL_LOBJ_CONS);
            hll_lisp_print(file, hll_unwrap_cons(obj)->car);

            hll_lisp_obj_head *cdr = hll_unwrap_cons(obj)->cdr;
            if (cdr != hll_nil && cdr->kind != HLL_LOBJ_CONS) {
                fprintf(file, " . ");
                hll_lisp_print(file, cdr);
                cdr = hll_nil;
            } else if (cdr != hll_nil) {
                fprintf(file, " ");
            }

            obj = cdr;
        }
        fprintf(file, ")");
        break;
    case HLL_LOBJ_SYMB: {
        fprintf(file, "%s", hll_unwrap_symb(obj)->symb);
    } break;
    case HLL_LOBJ_INT:
        fprintf(file, "%" PRId64 "", hll_unwrap_int(obj)->value);
        break;
    case HLL_LOBJ_BIND:
        fprintf(file, "<c-binding>");
        break;
    case HLL_LOBJ_NIL:
        fprintf(file, "()");
        break;
    case HLL_LOBJ_TRUE:
        fprintf(file, "t");
        break;
    }
}

static hll_lisp_obj_head *
call(hll_lisp_ctx *ctx, hll_lisp_obj_head *fn, hll_lisp_obj_head *args) {
    hll_lisp_obj_head *result = hll_nil;

    switch (fn->kind) {
    default:
        fprintf(stderr, "Unsupported callable type\n");
        break;
    case HLL_LOBJ_BIND:
        result = hll_unwrap_bind(fn)->bind(ctx, args);
        break;
    }

    return result;
}

hll_lisp_obj_head *
hll_eval(hll_lisp_ctx *ctx, hll_lisp_obj_head *obj) {
    hll_lisp_obj_head *result = hll_nil;

    switch (obj->kind) {
    default:
        assert(0);
        break;
    case HLL_LOBJ_BIND:
    case HLL_LOBJ_NIL:
    case HLL_LOBJ_INT:
        result = obj;
        break;
    case HLL_LOBJ_SYMB: {
        hll_lisp_obj_head *var = hll_find_var(ctx, obj);
        if (var == NULL) {
            fprintf(stderr, "Undefined variable '%s'\n",
                    hll_unwrap_symb(obj)->symb);
        } else {
            result = hll_unwrap_cons(var)->cdr;
        }
    } break;
    case HLL_LOBJ_CONS: {
        // Car should be function
        hll_lisp_obj_head *fn = hll_eval(ctx, hll_unwrap_cons(obj)->car);
        hll_lisp_obj_head *args = hll_unwrap_cons(obj)->cdr;
        result = call(ctx, fn, args);
    } break;
    }

    return result;
}

size_t
hll_list_length(hll_lisp_obj_head *obj) {
    size_t result = 0;

    while (obj->kind == HLL_LOBJ_CONS) {
        ++result;
        obj = hll_unwrap_cons(obj)->cdr;
    }

    return result;
}


hll_lisp_ctx
hll_default_ctx(void) {
    hll_lisp_ctx ctx = { 0 };

    ctx.symbols = hll_nil;
    ctx.file_out = stdout;
    ctx.file_outerr = stderr;
    ctx.env_stack = hll_make_env(&ctx, hll_nil);

    return ctx;
}

void
hll_dump_object_desc(void *file, hll_lisp_obj_head *obj) {
    switch (obj->kind) {
    case HLL_LOBJ_NONE:
        fprintf(file, "lobj<kind=None>");
        break;
    case HLL_LOBJ_TRUE:
        fprintf(file, "lobj<kind=True>");
        break;
    case HLL_LOBJ_CONS:
        fprintf(file, "lobj<kind=Cons car=");
        hll_dump_object_desc(file, hll_unwrap_cons(obj)->car);
        fprintf(file, " cdr=");
        hll_dump_object_desc(file, hll_unwrap_cons(obj)->cdr);
        fprintf(file, ">");
        break;
    case HLL_LOBJ_SYMB:
        fprintf(file, "lobj<kind=Symb symb='%s' len=%zu>",
                hll_unwrap_symb(obj)->symb, hll_unwrap_symb(obj)->length);
        break;
    case HLL_LOBJ_INT:
        fprintf(file, "lobj<kind=Int value=%" PRIi64 ">",
                hll_unwrap_int(obj)->value);
        break;
    case HLL_LOBJ_BIND:
        fprintf(file, "lobj<kind=Bind ptr=%zu>",
                (uintptr_t)hll_unwrap_bind(obj)->bind);
        break;
    case HLL_LOBJ_ENV:
        fprintf(file, "lobj<kind=Env vars=");
        hll_dump_object_desc(file, hll_unwrap_env(obj)->vars);
        fprintf(file, " up=");
        hll_dump_object_desc(file, hll_unwrap_env(obj)->up);
        fprintf(file, ">");
        break;
    case HLL_LOBJ_NIL:
        fprintf(file, "lobj<kind=Nil>");
        break;
    }
}
