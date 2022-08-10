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
    double num;
    void *body;
  } as;
} hll_obj;

typedef struct {
  const char *name;
  struct hll_bytecode *bytecode;
  struct hll_obj *param_names;
  struct hll_obj *var_list;
} hll_obj_func;

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

const char *hll_get_object_kind_str(hll_object_kind kind);
void free_object(struct hll_vm *vm, struct hll_obj *obj);

hll_obj *hll_copy_obj(struct hll_vm *vm, struct hll_obj *src);

hll_obj *hll_new_nil(struct hll_vm *vm);
hll_obj *hll_new_true(struct hll_vm *vm);

hll_obj *hll_new_num(struct hll_vm *vm, double num);
hll_obj *hll_new_symbol(struct hll_vm *vm, const char *symbol, size_t length);
hll_obj *hll_new_symbolz(struct hll_vm *vm, const char *symbol);

hll_obj *hll_new_cons(struct hll_vm *vm, hll_obj *car, hll_obj *cdr);
hll_obj *hll_new_env(struct hll_vm *vm, hll_obj *up, hll_obj *vars);
hll_obj *hll_new_bind(struct hll_vm *vm,
                      struct hll_obj *(*bind)(struct hll_vm *vm,
                                              struct hll_obj *args));
hll_obj *hll_new_func(struct hll_vm *vm, struct hll_obj *params,
                      struct hll_bytecode *bytecode, const char *name);

hll_obj_cons *hll_unwrap_cons(const struct hll_obj *obj);
hll_obj *hll_unwrap_cdr(const struct hll_obj *obj);
hll_obj *hll_unwrap_car(const struct hll_obj *obj);
hll_obj_env *hll_unwrap_env(const struct hll_obj *obj);
const char *hll_unwrap_zsymb(const struct hll_obj *obj);
hll_obj_symb *hll_unwrap_symb(const struct hll_obj *obj);
hll_obj_bind *hll_unwrap_bind(const struct hll_obj *obj);
hll_obj_func *hll_unwrap_func(const struct hll_obj *obj);

size_t hll_list_length(const struct hll_obj *obj);

#endif
