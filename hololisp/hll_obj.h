#ifndef HLL_OBJ_H
#define HLL_OBJ_H

#include <stddef.h>
#include <stdint.h>

struct hll_vm;

typedef enum {
    HLL_OBJ_NONE,
    HLL_OBJ_CONS,
    HLL_OBJ_SYMB,
    HLL_OBJ_NIL,
    HLL_OBJ_INT,
    HLL_OBJ_BIND,
    HLL_OBJ_ENV,
    HLL_OBJ_TRUE,
    HLL_OBJ_FUNC,
} hll_object_kind;

typedef struct hll_obj {
    hll_object_kind kind;
    union {
        int64_t num;
        void *body;
    } as;
} hll_obj;

typedef struct {
    struct hll_obj *vars;
    struct hll_obj *up;
} hll_obj_env;

typedef struct {
    struct hll_obj *car;
    struct hll_obj *cdr;
} hll_obj_cons;

typedef struct {
    struct hll_obj *(*bind)(struct hll_vm *vm, struct hll_obj *args);
} hll_obj_bind;

typedef struct {
    char const *symb;
    size_t length;
} hll_obj_symb;

#endif
