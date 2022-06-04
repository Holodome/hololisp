#include "lisp.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    hll_lisp_obj_head head;
    union {
        hll_lisp_cons cons;
        hll_lisp_symb symb;
        hll_lisp_func func;
        hll_lisp_int integer;
        hll_lisp_bind bind;
    } body;
} lisp_obj;

static lisp_obj hll_nil_ = { .head = {
                                 .kind = HLL_LOBJ_NIL,
                                 .size = 0,
                             } };
hll_lisp_obj_head *hll_nil = &hll_nil_.head;

hll_lisp_ctx
hll_default_ctx(void) {
    hll_lisp_ctx ctx = { 0 };

    ctx.symbols = hll_nil;
    ctx.bindings = hll_nil;

    return ctx;
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

hll_lisp_obj_head *
hll_make_cons(hll_lisp_ctx *ctx, hll_lisp_obj_head *car,
              hll_lisp_obj_head *cdr) {
    hll_lisp_obj_head *cons = ctx->alloc(sizeof(hll_lisp_cons), HLL_LOBJ_CONS);

    hll_unwrap_cons(cons)->car = car;
    hll_unwrap_cons(cons)->cdr = cdr;

    return cons;
}

hll_lisp_obj_head *
hll_make_acons(hll_lisp_ctx *ctx, hll_lisp_obj_head *x, hll_lisp_obj_head *y,
               hll_lisp_obj_head *a) {
    return hll_make_cons(ctx, hll_make_cons(ctx, x, y), a);
}

hll_lisp_obj_head *
hll_make_symb(hll_lisp_ctx *ctx, char const *data, size_t length) {
    hll_lisp_obj_head *symb =
        ctx->alloc(sizeof(hll_lisp_symb) + length + 1, HLL_LOBJ_SYMB);

    char *string =
        (char *)symb + sizeof(hll_lisp_obj_head) + sizeof(hll_lisp_symb);
    strncpy(string, data, length);
    hll_unwrap_symb(symb)->symb = string;
    hll_unwrap_symb(symb)->length = length;

    return symb;
}

hll_lisp_obj_head *
hll_make_int(hll_lisp_ctx *ctx, int64_t value) {
    hll_lisp_obj_head *integer = ctx->alloc(sizeof(hll_lisp_int), HLL_LOBJ_INT);
    hll_unwrap_int(integer)->value = value;
    return integer;
}

static hll_lisp_obj_head *
hll_make_binding(hll_lisp_ctx *ctx, hll_lisp_bind_func *bind) {
    hll_lisp_obj_head *binding =
        ctx->alloc(sizeof(hll_lisp_obj_head), HLL_LOBJ_BIND);
    hll_unwrap_bind(binding)->bind = bind;
    return binding;
}

void
hll_add_binding(hll_lisp_ctx *ctx, hll_lisp_bind_func *bind, char const *symbol,
                size_t length) {
    hll_lisp_obj_head *binding = hll_make_binding(ctx, bind);
    hll_lisp_obj_head *symb = hll_find_symb(ctx, symbol, length);
    ctx->bindings = hll_make_acons(ctx, symb, binding, ctx->bindings);
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

void
lisp_print(void *file_, hll_lisp_obj_head *obj) {
    FILE *file = file_;

    switch (obj->kind) {
    default:
        assert(0);
        break;
    case HLL_LOBJ_CONS:
        fprintf(file, "(");
        while (obj != hll_nil) {
            assert(obj->kind == HLL_LOBJ_CONS);
            lisp_print(file, hll_unwrap_cons(obj)->car);

            hll_lisp_obj_head *cdr = hll_unwrap_cons(obj)->cdr;
            if (cdr != hll_nil && cdr->kind != HLL_LOBJ_CONS) {
                fprintf(file, " . ");
                lisp_print(file, cdr);
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
    }
}

hll_lisp_obj_head *
hll_eval(hll_lisp_ctx *ctx, hll_lisp_obj_head *obj) {
    (void)ctx;
    // TODO:
    return obj;
}

static HLL_LISP_BIND(builtin_print) {
    lisp_print(stdout, hll_eval(ctx, hll_unwrap_cons(args)->car));
    printf("\n");
    return hll_nil;
}

static HLL_LISP_BIND(builtin_add) {
    int64_t result = 0;
    for (hll_lisp_obj_head *obj = args; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr) {
        result += hll_unwrap_int(hll_eval(ctx, obj))->value;
    }
    return hll_make_int(ctx, result);
}

static HLL_LISP_BIND(builtin_sub) {
    int64_t result = 0;
    for (hll_lisp_obj_head *obj = args; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr) {
        result -= hll_unwrap_int(hll_eval(ctx, obj))->value;
    }
    return hll_make_int(ctx, result);
}

static HLL_LISP_BIND(builtin_div) {
    int64_t result =
        hll_unwrap_int(hll_eval(ctx, hll_unwrap_cons(args)->car))->value;

    for (hll_lisp_obj_head *obj = hll_unwrap_cons(args)->cdr; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr) {
        result /= hll_unwrap_int(hll_eval(ctx, obj))->value;
    }
    return hll_make_int(ctx, result);
}

static HLL_LISP_BIND(builtin_mul) {
    int64_t result = 1;
    for (hll_lisp_obj_head *obj = args; obj != hll_nil;
         obj = hll_unwrap_cons(obj)->cdr) {
        result *= hll_unwrap_int(hll_eval(ctx, obj))->value;
    }
    return hll_make_int(ctx, result);
}

void
hll_add_builtins(hll_lisp_ctx *ctx) {
    hll_add_binding(ctx, builtin_print, "print", sizeof("print") - 1);
    hll_add_binding(ctx, builtin_add, "+", 1);
    hll_add_binding(ctx, builtin_sub, "-", 1);
    hll_add_binding(ctx, builtin_div, "/", 1);
    hll_add_binding(ctx, builtin_mul, "*", 1);
}

