#ifndef __HLL_LISP_H__
#define __HLL_LISP_H__

#include <stddef.h>
#include <stdint.h>

struct hll_lexer;
struct hll_ctx;

#define HLL_BUILTIN_QUOTE_SYMB_NAME "quote"
#define HLL_BUILTIN_QUOTE_SYMB_LEN (sizeof(HLL_BUILTIN_QUOTE_SYMB_NAME) - 1)

/// Enumeration describing different lisp object kinds.
typedef enum {
    /// Reserve 0 to make sure we don't forget to initialize kind.
    HLL_LOBJ_NONE = 0x0,
    /// Cons
    HLL_LOBJ_CONS = 0x1,
    /// Symbol
    HLL_LOBJ_SYMB = 0x2,
    /// NIL
    HLL_LOBJ_NIL = 0x3,
    /// Integer
    HLL_LOBJ_INT = 0x4,
    /// C binding function
    HLL_LOBJ_BIND = 0x5,
    /// Env frame
    HLL_LOBJ_ENV = 0x6,
    /// True
    HLL_LOBJ_TRUE = 0x7,
} hll_obj_kind;

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
    /// Kind of object for identifying it.
    hll_obj_kind kind;
    /// Size of body.
    /// TODO: Do we need it here?
    size_t size;
} hll_obj;

/// Cons object.
typedef struct {
    /// A car.
    hll_obj *car;
    /// A cdr.
    hll_obj *cdr;
} hll_cons;

/// Symbol object.
typedef struct {
    /// Symbol pointer.
    char const *symb;
    /// Length of symbol.
    size_t length;
} hll_symb;

/// Function object.
typedef struct {
    /// Parameter list (their names).
    hll_obj *params;
    /// Body statement list.
    hll_obj *body;
    /// Environment in which function is defined and which is used when
    /// executing it.
    hll_obj *env;
} hll_func;

/// Integer object.
typedef struct {
    /// Value.
    int64_t value;
} hll_int;

/// Environment frame contains information about current lexical scope.
typedef struct {
    /// Pointer to parent scope frame.
    hll_obj *up;
    /// Variable linked list.
    hll_obj *vars;
} hll_env;

/// Signature of C binding function.
#define HLL_LISP_BIND(_name) hll_obj *_name(struct hll_ctx *ctx, hll_obj *args)

/// Type of c binding function.
typedef HLL_LISP_BIND(hll_bind_func);

/// C binding object.
typedef struct {
    /// Bind function.
    hll_bind_func *bind;
} hll_bind;

/// Lisp context that is used when executing lisp expressions.
typedef struct hll_ctx {
    /// Control where stdout goes.
    void *file_out;
    /// Control where stderr goes.
    void *file_outerr;
    /// Symbols linked list.
    /// TODO: Make this a hash map?
    hll_obj *symbols;
    /// Current executing frame stack.
    hll_obj *env_stack;
} hll_ctx;

/// Global nil object.
extern hll_obj *hll_nil;
// Global true object.
extern hll_obj *hll_true;

hll_cons *hll_unwrap_cons(hll_obj *obj);

hll_symb *hll_unwrap_symb(hll_obj *obj);

hll_int *hll_unwrap_int(hll_obj *obj);

hll_bind *hll_unwrap_bind(hll_obj *obj);

hll_env *hll_unwrap_env(hll_obj *obj);

hll_ctx hll_default_ctx(void);

void hll_add_builtins(hll_ctx *ctx);

hll_obj *hll_make_cons(hll_ctx *ctx, hll_obj *car, hll_obj *cdr);

hll_obj *hll_make_acons(hll_ctx *ctx, hll_obj *x, hll_obj *y, hll_obj *a);

hll_obj *hll_make_symb(hll_ctx *ctx, char const *symb, size_t length);

hll_obj *hll_make_int(hll_ctx *ctx, int64_t value);

void hll_add_binding(hll_ctx *ctx, hll_bind_func *bind, char const *symbol,
                     size_t length);

hll_obj *hll_reverse_list(hll_obj *obj);

size_t hll_list_length(hll_obj *obj);

hll_obj *hll_find_symb(hll_ctx *ctx, char const *symb, size_t length);

hll_obj *hll_find_var(hll_ctx *ctx, hll_obj *car);

hll_obj *hll_eval(hll_ctx *ctx, hll_obj *obj);

void hll_print(void *file, hll_obj *obj);

void hll_dump_object_desc(void *file, hll_obj *object);

void hll_report_error(hll_ctx *ctx, char const *format, ...);

#endif
