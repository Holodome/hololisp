#include "lisp_gc.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hll_lisp.h"

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

struct hll_obj *
hll_alloc(size_t body_size, uint32_t kind) {
    hll_obj *head = calloc(1, sizeof(hll_obj) + body_size);
    head->kind = kind;
    head->size = body_size;

    return head;
}

void
hll_free(hll_obj *head) {
    free(head);
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
    hll_unwrap_env(env)->vars = hll_make_nil(ctx);

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
