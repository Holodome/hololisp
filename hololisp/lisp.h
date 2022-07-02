#ifndef __HLL_LISP_H__
#define __HLL_LISP_H__

#include <stddef.h>
#include <stdint.h>

#include "ext.h"

struct hll_lexer;
struct hll_ctx;
struct hll_error_reporter;

typedef enum {
    HLL_OBJ_NONE = 0x0,
    HLL_OBJ_CONS = 0x1,
    HLL_OBJ_SYMB = 0x2,
    HLL_OBJ_NIL = 0x3,
    HLL_OBJ_INT = 0x4,
    HLL_OBJ_BIND = 0x5,
    HLL_OBJ_ENV = 0x6,
    HLL_OBJ_TRUE = 0x7,
    HLL_OBJ_FUNC = 0x8,
} hll_obj_kind;

/// Returns string of object kind.
/// If kind is invalid, NULL is returned.
char const *hll_get_obj_kind_str(hll_obj_kind kind);

/// All lisp objects are represented in abstract hierarchy, similar to
/// inheritance. Internally lisp object are located in memory as
/// *hll_obj* and body right after that, whilst total size of object
/// being sum of head and body sizes. Allocaing objects this way lets us manage
/// their memory more freely and not worrying about fragmentation too much
/// because that is a job of garbage collector. Because we have enumeration of
/// all possible kinds we are free to make our own memory-efficient allocator.
///
/// Object head contains information sufficient for identifying lisp object in
/// terms of language runtime. All private data is left unseen and is managed
/// internally via pointer arithmetic. If we want to lisp object according to
/// its kind we can use hll_unwrap_... functions.
typedef struct hll_obj {
    hll_obj_kind kind;
    size_t size;
} hll_obj;

typedef struct hll_cons {
    hll_obj *car;
    hll_obj *cdr;
} hll_cons;

typedef struct hll_symb {
    char const *symb;
    size_t length;
} hll_symb;

typedef struct hll_func {
    hll_obj *params;
    hll_obj *body;
    /// Environment in which function is defined and which is used when
    /// executing it.
    hll_obj *env;

    struct {
        char const *name;
    } meta;
} hll_func;

typedef struct hll_int {
    int64_t value;
} hll_int;

typedef struct hll_env {
    /// Pointer to parent scope frame.
    hll_obj *up;
    hll_obj *vars;

    struct {
        char const *name;
    } meta;
} hll_env;

typedef hll_obj *hll_bind_func(struct hll_ctx *ctx, hll_obj *args);

typedef struct hll_bind {
    hll_bind_func *bind;

    struct {
        char const *name;
    } meta;
} hll_bind;

typedef struct hll_ctx {
    void *file_in;
    void *file_out;
    void *file_outerr;
    /// Symbols linked list.
    hll_obj *symbols;
    /// Current executing frame stack.
    hll_obj *env;

    struct hll_error_reporter *reporter;
} hll_ctx;

HLL_DECL
hll_ctx hll_create_ctx(struct hll_error_reporter *reporter);

HLL_DECL
hll_obj *hll_find_symb(hll_ctx *ctx, char const *symb, size_t length);

HLL_DECL
hll_obj *hll_find_var(hll_ctx *ctx, hll_obj *car);

HLL_DECL
hll_obj *hll_add_binding(hll_ctx *ctx, hll_bind_func *bind, char const *symbol,
                         size_t length);

HLL_DECL
void hll_add_var(hll_ctx *ctx, hll_obj *env, hll_obj *symb, hll_obj *value);

HLL_DECL
hll_obj *hll_call(hll_ctx *ctx, hll_obj *fn, hll_obj *args);

HLL_DECL
hll_obj *hll_eval(hll_ctx *ctx, hll_obj *obj);

HLL_DECL
void hll_print(hll_ctx *ctx, void *file, hll_obj *obj);

HLL_DECL
size_t hll_list_length(hll_obj *obj);

HLL_DECL
void hll_print_error_stack_trace(hll_ctx *ctx);

#ifdef HLL_DEBUG
HLL_DECL
void hll_dump_object_desc(void *file, hll_obj *object);
#endif

#endif
