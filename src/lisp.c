#include "lisp.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "error_reporter.h"
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

char const *
hll_get_obj_kind_str(hll_obj_kind kind) {
    char const *result = NULL;
    switch (kind) {
    default:
        break;
    case HLL_OBJ_CONS:
        result = "cons";
        break;
    case HLL_OBJ_SYMB:
        result = "symb";
        break;
    case HLL_OBJ_NIL:
        result = "nil";
        break;
    case HLL_OBJ_INT:
        result = "int";
        break;
    case HLL_OBJ_BIND:
        result = "c binding";
        break;
    case HLL_OBJ_ENV:
        result = "env";
        break;
    case HLL_OBJ_TRUE:
        result = "true";
        break;
    case HLL_OBJ_FUNC:
        result = "func";
        break;
    }

    return result;
}

hll_obj *
hll_make_nil(hll_ctx *ctx) {
    (void)ctx;
    return hll_nil;
}

hll_obj *
hll_make_true(hll_ctx *ctx) {
    (void)ctx;
    return hll_true;
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

hll_func *
hll_unwrap_func(hll_obj *head) {
    lisp_obj *obj = (void *)head;
    assert(head->kind == HLL_OBJ_FUNC);
    return &obj->body.func;
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
    hll_unwrap_env(env)->vars = hll_nil;

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

hll_obj *
hll_make_bind(hll_ctx *ctx, hll_bind_func *bind) {
    (void)ctx;
    hll_obj *binding = hll_alloc(sizeof(hll_bind), HLL_OBJ_BIND);
    hll_unwrap_bind(binding)->bind = bind;
    return binding;
}

hll_obj *
hll_make_func(hll_ctx *ctx, hll_obj *env, hll_obj *params, hll_obj *body) {
    (void)ctx;
    hll_obj *func = hll_alloc(sizeof(hll_func), HLL_OBJ_FUNC);

    hll_unwrap_func(func)->env = env;
    hll_unwrap_func(func)->params = params;
    hll_unwrap_func(func)->body = body;

    return func;
}

void
hll_add_binding(hll_ctx *ctx, hll_bind_func *bind_func, char const *symbol,
                size_t length) {
    hll_obj *bind = hll_make_bind(ctx, bind_func);
    hll_obj *symb = hll_find_symb(ctx, symbol, length);
    assert(ctx->env_stack != hll_nil);
    hll_unwrap_env(ctx->env_stack)->vars =
        hll_make_acons(ctx, symb, bind, hll_unwrap_env(ctx->env_stack)->vars);
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
    case HLL_OBJ_FUNC:
        fprintf(file, "<func>");
        break;
    }
}

hll_obj *
hll_call(hll_ctx *ctx, hll_obj *fn, hll_obj *args) {
    hll_obj *result = hll_nil;

    switch (fn->kind) {
    default:
        hll_report_error(ctx->reporter, "Unsupported callable type\n");
        break;
    case HLL_OBJ_BIND:
        result = hll_unwrap_bind(fn)->bind(ctx, args);
        break;
    case HLL_OBJ_FUNC: {
        hll_func *func = hll_unwrap_func(fn);
        hll_obj *cur_env = ctx->env_stack;
        hll_obj *env = hll_make_env(ctx, func->env);
        assert(hll_list_length(args) == hll_list_length(func->params));
        for (hll_obj *arg = args, *name = func->params; arg != hll_nil;
             arg = hll_unwrap_cdr(arg), name = hll_unwrap_cdr(name)) {
            hll_unwrap_env(env)->vars = hll_make_acons(
                ctx, hll_unwrap_car(name), hll_eval(ctx, hll_unwrap_car(arg)),
                hll_unwrap_env(env)->vars);
        }
        ctx->env_stack = env;
        result = hll_std_progn(ctx, func->body);
        ctx->env_stack = cur_env;
    } break;
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
            hll_report_error(ctx->reporter, "Undefined variable '%s'\n",
                             hll_unwrap_symb(obj)->symb);
        } else {
            result = hll_unwrap_cons(var)->cdr;
        }
    } break;
    case HLL_OBJ_CONS: {
        // Car should be executable
        hll_obj *fn = hll_eval(ctx, hll_unwrap_cons(obj)->car);
        hll_obj *args = hll_unwrap_cons(obj)->cdr;
        result = hll_call(ctx, fn, args);
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

void
hll_add_var(hll_ctx *ctx, hll_obj *symb, hll_obj *value) {
    hll_unwrap_env(ctx->env_stack)->vars =
        hll_make_acons(ctx, symb, value, hll_unwrap_env(ctx->env_stack)->vars);
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
#define _HLL_CAR_CDR _HLL_CAR_CDR_STD_FUNC
#define _HLL_STD_FUNC(_name, _str) BIND(hll_std_##_name, _str);
    _HLL_ENUMERATE_STD_FUNCS
#undef _HLL_STD_FUNC
    hll_unwrap_env(ctx.env_stack)->vars = hll_make_acons(
        &ctx, hll_find_symb(&ctx, STR_LEN("t")), hll_make_true(&ctx),
        hll_unwrap_env(ctx.env_stack)->vars);
#undef _HLL_CAR_CDR
#undef BIND
#undef STR_LEN

    return ctx;
}

#ifdef HLL_DELC
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
    case HLL_OBJ_FUNC:
        fprintf(file, "lobj<kind=Func>");
        break;
    }
}
#endif
