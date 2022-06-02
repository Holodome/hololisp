#ifndef __HLL_LISP_H__
#define __HLL_LISP_H__

#include <stddef.h>
#include <stdint.h>

struct hll_lexer;

typedef enum {
    HLL_LOBJ_NONE = 0x0,
    HLL_LOBJ_CONS = 0x1,
    HLL_LOBJ_SYMB = 0x2,
    /* HLL_LOBJ_FUNC  = 0x3, */
    /* HLL_LOBJ_MACRO = 0x4, */
    /* HLL_LOBJ_TRUE  = 0x5, */
    /* HLL_LOBJ_NIL   = 0x6, */
    HLL_LOBJ_INT = 0x7
} hll_lisp_obj_kind;

typedef struct hll_lisp_obj_head {
    hll_lisp_obj_kind kind;
    size_t            size;
    void             *body;
} hll_lisp_obj_head;

typedef struct {
    hll_lisp_obj_head *car;
    hll_lisp_obj_head *cdr;
} hll_lisp_obj_cons;

typedef struct {
    char const *symb;
} hll_lisp_obj_symb;

typedef struct {
    hll_lisp_obj_head *params;
    hll_lisp_obj_head *body;
    hll_lisp_obj_head *env;
} hll_lisp_obj_func;

typedef struct {
    int64_t value;
} hll_lisp_obj_int;

/* extern hll_lisp_obj_head *hll_true; */
extern hll_lisp_obj_head *hll_nil;

#define HLL_LOBJ_ALLOC(_name) \
    hll_lisp_obj_head *_name(size_t body_size, hll_lisp_obj_kind kind)
typedef HLL_LOBJ_ALLOC(hll_lisp_obj_alloc);

#define HLL_LOBJ_FREE(_name) void *_name(hll_lisp_obj_head *head)
typedef HLL_LOBJ_FREE(hll_lisp_obj_free);

typedef struct {
    hll_lisp_obj_alloc *alloc;
    hll_lisp_obj_free  *free;
    void               *state;
} hll_lisp_ctx;

hll_lisp_obj_head *hll_make_cons(hll_lisp_ctx *ctx, hll_lisp_obj_head *car,
                                 hll_lisp_obj_head *cdr);

hll_lisp_obj_head *hll_make_symb(hll_lisp_ctx *ctx, char const *symb,
                                 size_t length);

hll_lisp_obj_head *hll_make_int(hll_lisp_ctx *ctx, int64_t value);

#endif
