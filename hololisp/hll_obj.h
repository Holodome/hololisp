#ifndef HLL_OBJ_H
#define HLL_OBJ_H

#include <stddef.h>
#include <stdint.h>

#include "hll_hololisp.h"

struct hll_vm;

typedef enum {
    HLL_OBJ_CONS = 0x1,
    HLL_OBJ_SYMB,
    HLL_OBJ_NIL,
    HLL_OBJ_NUM,
    HLL_OBJ_BIND,
    HLL_OBJ_ENV,
    HLL_OBJ_TRUE,
    HLL_OBJ_FUNC,
} hll_object_kind;

typedef struct hll_obj {
    hll_object_kind kind;
    union {
        hll_num num;
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
    size_t length;
    char symb[];
} hll_obj_symb;

hll_obj *hll_new_nil(struct hll_vm *vm);
hll_obj *hll_new_true(struct hll_vm *vm);

hll_obj *hll_new_num(struct hll_vm *vm, hll_num num);
hll_obj *hll_new_symbol(struct hll_vm *vm, char const *symbol, size_t length);
hll_obj *hll_new_cons(struct hll_vm *vm, hll_obj *car, hll_obj *cdr);

#endif
