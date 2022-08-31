#ifndef HLL_OBJ_H
#define HLL_OBJ_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hll_hololisp.h"

enum hll_object_kind {
  HLL_OBJ_NUM = 0x0,
  HLL_OBJ_CONS = 0x1,
  HLL_OBJ_SYMB = 0x2,
  HLL_OBJ_NIL = 0x3,
  HLL_OBJ_BIND = 0x4,
  HLL_OBJ_ENV = 0x5,
  HLL_OBJ_TRUE = 0x6,
  HLL_OBJ_FUNC = 0x7,
  HLL_OBJ_MACRO = 0x8,

  HLL_OBJ_FREED = 0x100
};

struct hll_obj_cons {
  struct hll_obj *car;
  struct hll_obj *cdr;
};

struct hll_obj {
  enum hll_object_kind kind;
  bool is_dark;
  struct hll_obj *next_gc;
  union {
    struct hll_obj_cons cons;
    double num;
    void *body;
  } as;
};

struct hll_obj_func {
  const char *name;
  struct hll_bytecode *bytecode;
  struct hll_obj *param_names;
  struct hll_obj *var_list;
  struct hll_obj *env;
};

struct hll_obj_env {
  struct hll_obj *vars;
  struct hll_obj *up;
};

struct hll_obj_bind {
  struct hll_obj *(*bind)(struct hll_vm *vm, struct hll_obj *args);
};

struct hll_obj_symb {
  size_t length;
  char symb[];
};

HLL_PUB
const char *hll_get_object_kind_str(enum hll_object_kind kind);

HLL_PUB
void hll_free_object(struct hll_vm *vm, struct hll_obj *obj);

HLL_PUB
struct hll_obj *hll_copy_obj(struct hll_vm *vm, struct hll_obj *src);
HLL_PUB
struct hll_obj *hll_new_nil(struct hll_vm *vm);

HLL_PUB
struct hll_obj *hll_new_true(struct hll_vm *vm);

HLL_PUB
struct hll_obj *hll_new_num(struct hll_vm *vm, double num);
HLL_PUB
struct hll_obj *hll_new_symbol(struct hll_vm *vm, const char *symbol,
                               size_t length);
HLL_PUB
struct hll_obj *hll_new_symbolz(struct hll_vm *vm, const char *symbol);

HLL_PUB
struct hll_obj *hll_new_cons(struct hll_vm *vm, struct hll_obj *car,
                             struct hll_obj *cdr);
HLL_PUB
struct hll_obj *hll_new_env(struct hll_vm *vm, struct hll_obj *up,
                            struct hll_obj *vars);
HLL_PUB
struct hll_obj *hll_new_bind(struct hll_vm *vm,
                             struct hll_obj *(*bind)(struct hll_vm *vm,
                                                     struct hll_obj *args));
HLL_PUB
struct hll_obj *hll_new_func(struct hll_vm *vm, struct hll_obj *params,
                             struct hll_bytecode *bytecode, const char *name);

HLL_PUB
struct hll_obj *hll_new_macro(struct hll_vm *vm, struct hll_obj *params,
                              struct hll_bytecode *bytecode, const char *name);

HLL_PUB
struct hll_obj_cons *hll_unwrap_cons(struct hll_obj *obj);
HLL_PUB
struct hll_obj *hll_unwrap_cdr(struct hll_obj *obj);
HLL_PUB
struct hll_obj *hll_unwrap_car(struct hll_obj *obj);
HLL_PUB
struct hll_obj_env *hll_unwrap_env(struct hll_obj *obj);
HLL_PUB
const char *hll_unwrap_zsymb(struct hll_obj *obj);
HLL_PUB
struct hll_obj_symb *hll_unwrap_symb(struct hll_obj *obj);
HLL_PUB
struct hll_obj_bind *hll_unwrap_bind(struct hll_obj *obj);
HLL_PUB
struct hll_obj_func *hll_unwrap_func(struct hll_obj *obj);
HLL_PUB
struct hll_obj_func *hll_unwrap_macro(struct hll_obj *obj);
HLL_PUB
double hll_unwrap_num(struct hll_obj *obj);

HLL_PUB
size_t hll_list_length(struct hll_obj *obj);

HLL_PUB
void hll_gray_obj(struct hll_vm *vm, struct hll_obj *obj);
HLL_PUB
void hll_blacken_obj(struct hll_vm *vm, struct hll_obj *obj);

#endif
