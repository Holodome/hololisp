#include "lisp.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "error_reporter.h"
#include "lisp_gc.h"
#include "lisp_std.h"

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
hll_add_binding(hll_ctx *ctx, hll_bind_func *bind_func, char const *symbol,
                size_t length) {
    hll_obj *bind = hll_make_bind(ctx, bind_func);
    hll_obj *symb = hll_find_symb(ctx, symbol, length);
    assert(ctx->env->kind != HLL_OBJ_NIL);
    hll_unwrap_env(ctx->env)->vars =
        hll_make_acons(ctx, symb, bind, hll_unwrap_env(ctx->env)->vars);
    return bind;
}

hll_obj *
hll_find_symb(hll_ctx *ctx, char const *data, size_t length) {
    hll_obj *found = NULL;

    for (hll_obj *obj = ctx->symbols; obj->kind != HLL_OBJ_NIL && found == NULL;
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

    for (hll_obj *env = ctx->env; env->kind != HLL_OBJ_NIL && result == NULL;
         env = hll_unwrap_env(env)->up) {
        for (hll_obj *cons = hll_unwrap_env(env)->vars;
             cons->kind != HLL_OBJ_NIL && result == NULL;
             cons = hll_unwrap_cdr(cons)) {
            hll_obj *test = hll_unwrap_cons(cons)->car;
            if (hll_unwrap_car(test) == car) {
                result = test;
            }
        }
    }

    return result;
}

void
hll_print(hll_ctx *ctx, void *file_, hll_obj *obj) {
    FILE *file = file_;

    switch (obj->kind) {
    default:
        assert(0);
        break;
    case HLL_OBJ_CONS:
        fprintf(file, "(");
        while (obj->kind != HLL_OBJ_NIL) {
            assert(obj->kind == HLL_OBJ_CONS);
            hll_print(ctx, file, hll_unwrap_car(obj));

            hll_obj *cdr = hll_unwrap_cdr(obj);
            if (cdr->kind != HLL_OBJ_NIL && cdr->kind != HLL_OBJ_CONS) {
                fprintf(file, " . ");
                hll_print(ctx, file, cdr);
                cdr = hll_make_nil(ctx);
            } else if (cdr != hll_make_nil(ctx)) {
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
    hll_obj *result = NULL;

    switch (fn->kind) {
    default:
        hll_report_error(ctx->reporter, "Unsupported callable type %s",
                         hll_get_obj_kind_str(fn->kind));
        break;
    case HLL_OBJ_BIND: {
        hll_obj *cur_env = ctx->env;
        hll_obj *env = hll_make_env(ctx, cur_env);
        hll_unwrap_env(env)->meta.name = hll_unwrap_bind(fn)->meta.name;

        ctx->env = env;
        result = hll_unwrap_bind(fn)->bind(ctx, args);
        ctx->env = cur_env;
    } break;
    case HLL_OBJ_FUNC: {
        hll_func *func = hll_unwrap_func(fn);
        hll_obj *cur_env = ctx->env;
        hll_obj *env = hll_make_env(ctx, func->env);
        assert(hll_list_length(args) == hll_list_length(func->params));
        for (hll_obj *arg = args, *name = func->params;
             arg->kind != HLL_OBJ_NIL;
             arg = hll_unwrap_cdr(arg), name = hll_unwrap_cdr(name)) {
            hll_unwrap_env(env)->vars = hll_make_acons(
                ctx, hll_unwrap_car(name), hll_eval(ctx, hll_unwrap_car(arg)),
                hll_unwrap_env(env)->vars);
        }
        hll_unwrap_env(env)->meta.name = func->meta.name;

        ctx->env = env;
        result = hll_std_progn(ctx, func->body);
        ctx->env = cur_env;
    } break;
    }

    if (result == NULL) {
        result = hll_make_nil(ctx);
    }

    return result;
}

hll_obj *
hll_eval(hll_ctx *ctx, hll_obj *obj) {
    hll_obj *result = NULL;

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
            hll_report_error(ctx->reporter, "Undefined variable '%s'",
                             hll_unwrap_symb(obj)->symb);
#if HLL_DEBUG
            char buffer[16384];
            size_t len = 0;
            for (hll_obj *env = ctx->env;
                 env->kind != HLL_OBJ_NIL && result == NULL;
                 env = hll_unwrap_env(env)->up) {
                for (hll_obj *cons = hll_unwrap_env(env)->vars;
                     cons->kind != HLL_OBJ_NIL && result == NULL;
                     cons = hll_unwrap_cdr(cons)) {
                    hll_obj *test = hll_unwrap_cons(cons)->car;
                    size_t written =
                        snprintf(buffer + len, sizeof(buffer) - len, "%s ",
                                 hll_unwrap_symb(hll_unwrap_car(test))->symb);
                    len += written;
                }
            }
            hll_report_note(ctx->reporter, "Possible variants are: %s", buffer);
#endif
        } else {
            result = hll_unwrap_cons(var)->cdr;
        }
    } break;
    case HLL_OBJ_CONS: {
        // Car should be executable
        hll_obj *fn = hll_eval(ctx, hll_unwrap_car(obj));
        hll_obj *args = hll_unwrap_cons(obj)->cdr;
        result = hll_call(ctx, fn, args);
    } break;
    }

    if (result == NULL) {
        result = hll_make_nil(ctx);
    }

    return result;
}

size_t
hll_list_length(hll_obj *obj) {
    size_t result = 0;

    while (obj->kind == HLL_OBJ_CONS) {
        ++result;
        obj = hll_unwrap_cdr(obj);
    }

    return result;
}

void
hll_add_var(hll_ctx *ctx, hll_obj *env, hll_obj *symb, hll_obj *value) {
    hll_unwrap_env(env)->vars =
        hll_make_acons(ctx, symb, value, hll_unwrap_env(env)->vars);
}

void
hll_print_error_stack_trace(hll_ctx *ctx) {
    size_t idx = 0;
    for (hll_obj *env = ctx->env; env->kind != HLL_OBJ_NIL;
         env = hll_unwrap_env(env)->up) {
        hll_report_note(ctx->reporter, "%zu: In %s", idx,
                        hll_unwrap_env(env)->meta.name);

        ++idx;
    }
}

hll_ctx
hll_create_ctx(hll_error_reporter *reporter) {
    hll_ctx ctx = { 0 };

    ctx.reporter = reporter;
    ctx.symbols = hll_make_nil(&ctx);
    ctx.file_out = stdout;
    ctx.file_outerr = stderr;
    ctx.env = hll_make_env(&ctx, hll_make_nil(&ctx));
    hll_unwrap_env(ctx.env)->meta.name = "global";

#define STR_LEN(_str) _str, (sizeof(_str) - 1)
#define BIND(_func, _symb)                                              \
    do {                                                                \
        hll_obj *__temp = hll_add_binding(&ctx, _func, STR_LEN(_symb)); \
        hll_unwrap_bind(__temp)->meta.name = _symb;                     \
    } while (0);
#define _HLL_CAR_CDR _HLL_CAR_CDR_STD_FUNC
#define _HLL_STD_FUNC(_name, _str) BIND(hll_std_##_name, _str);
    _HLL_ENUMERATE_STD_FUNCS
#undef _HLL_STD_FUNC
    hll_unwrap_env(ctx.env)->vars =
        hll_make_acons(&ctx, hll_find_symb(&ctx, STR_LEN("t")),
                       hll_make_true(&ctx), hll_unwrap_env(ctx.env)->vars);
#undef _HLL_CAR_CDR
#undef BIND
#undef STR_LEN

    return ctx;
}

#ifdef HLL_DEBUG
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
