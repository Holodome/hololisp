#ifndef __HLL_LISP_H__
#define __HLL_LISP_H__

#include <stddef.h>
#include <stdint.h>

struct hll_lexer;
struct hll_lisp_ctx;

#define HLL_BUILTIN_QUOTE_SYMB_NAME "quote"

typedef enum {
    HLL_LOBJ_NONE = 0x0,
    HLL_LOBJ_CONS = 0x1,
    HLL_LOBJ_SYMB = 0x2,
    HLL_LOBJ_NIL = 0x3,
    HLL_LOBJ_INT = 0x4,
    HLL_LOBJ_BIND = 0x5
} hll_lisp_obj_kind;

typedef struct hll_lisp_obj_head {
    hll_lisp_obj_kind kind;
    size_t size;
} hll_lisp_obj_head;

typedef struct {
    hll_lisp_obj_head *car;
    hll_lisp_obj_head *cdr;
} hll_lisp_cons;

typedef struct {
    char const *symb;
    size_t length;
} hll_lisp_symb;

typedef struct {
    hll_lisp_obj_head *params;
    hll_lisp_obj_head *body;
    hll_lisp_obj_head *env;
} hll_lisp_func;

typedef struct {
    int64_t value;
} hll_lisp_int;

#define HLL_LISP_BIND(_name) \
    hll_lisp_obj_head *_name(struct hll_lisp_ctx *ctx, hll_lisp_obj_head *args)
typedef HLL_LISP_BIND(hll_lisp_bind_func);

typedef struct {
    hll_lisp_bind_func *bind;
} hll_lisp_bind;

#define HLL_LOBJ_ALLOC(_name) \
    hll_lisp_obj_head *_name(size_t body_size, hll_lisp_obj_kind kind)
typedef HLL_LOBJ_ALLOC(hll_lisp_obj_alloc);

#define HLL_LOBJ_FREE(_name) void _name(hll_lisp_obj_head *head)
typedef HLL_LOBJ_FREE(hll_lisp_obj_free);

typedef struct hll_lisp_ctx {
    hll_lisp_obj_alloc *alloc;
    hll_lisp_obj_free *free;
    void *state;

    hll_lisp_obj_head *symbols;
    hll_lisp_obj_head *bindings;
} hll_lisp_ctx;

extern hll_lisp_obj_head *hll_nil;

hll_lisp_obj_kind hll_obj_kind(hll_lisp_obj_head *obj);

hll_lisp_cons *hll_unwrap_cons(hll_lisp_obj_head *obj);

hll_lisp_symb *hll_unwrap_symb(hll_lisp_obj_head *obj);

hll_lisp_int *hll_unwrap_int(hll_lisp_obj_head *obj);

hll_lisp_bind *hll_unwrap_bind(hll_lisp_obj_head *obj);

hll_lisp_ctx hll_default_ctx(void);

void hll_add_builtins(hll_lisp_ctx *ctx);

hll_lisp_obj_head *hll_make_cons(hll_lisp_ctx *ctx, hll_lisp_obj_head *car,
                                 hll_lisp_obj_head *cdr);

hll_lisp_obj_head *hll_make_acons(hll_lisp_ctx *ctx, hll_lisp_obj_head *x,
                                  hll_lisp_obj_head *y, hll_lisp_obj_head *a);

hll_lisp_obj_head *hll_make_symb(hll_lisp_ctx *ctx, char const *symb,
                                 size_t length);

hll_lisp_obj_head *hll_make_int(hll_lisp_ctx *ctx, int64_t value);

void hll_add_binding(hll_lisp_ctx *ctx, hll_lisp_bind_func *bind,
                     char const *symbol, size_t length);

hll_lisp_obj_head *hll_reverse_list(hll_lisp_obj_head *obj);

hll_lisp_obj_head *hll_find_symb(hll_lisp_ctx *ctx, char const *symb,
                                 size_t length);

hll_lisp_obj_head *hll_eval(hll_lisp_ctx *ctx, hll_lisp_obj_head *obj);

void hll_lisp_print(void *file, hll_lisp_obj_head *obj);

#endif
