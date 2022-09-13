#ifndef HLL_OBJ_H
#define HLL_OBJ_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hll_hololisp.h"

typedef uint8_t hll_object_kind;
enum {
  // Singleton values
  HLL_OBJ_NUM = 0x0,
  HLL_OBJ_NIL = 0x1,
  HLL_OBJ_TRUE = 0x2,
  // Heap-allocated garbage-collected values
  HLL_OBJ_CONS = 0x3,
  HLL_OBJ_SYMB = 0x4,
  HLL_OBJ_BIND = 0x5,
  HLL_OBJ_ENV = 0x6,
  HLL_OBJ_FUNC = 0x7,
  HLL_OBJ_MACRO = 0x8,
};

struct hll_obj {
  hll_object_kind kind;
  bool is_dark;
  struct hll_obj *next_gc;
  char as[];
};

struct hll_obj_cons {
  hll_value car;
  hll_value cdr;
};

struct hll_obj_func {
  const char *name;
  struct hll_bytecode *bytecode;
  hll_value param_names;
  hll_value var_list;
  hll_value env;
};

struct hll_obj_env {
  hll_value vars;
  hll_value up;
};

struct hll_obj_bind {
  hll_value (*bind)(struct hll_vm *vm, hll_value args);
};

struct hll_obj_symb {
  size_t length;
  char symb[];
};

HLL_PUB
const char *hll_get_object_kind_str(hll_object_kind kind);

HLL_PUB
void hll_free_object(struct hll_vm *vm, struct hll_obj *obj);

HLL_PUB
hll_value hll_copy_obj(struct hll_vm *vm, hll_value src);

HLL_PUB hll_value hll_nil(void);

HLL_PUB hll_value hll_true(void);

HLL_PUB hll_value hll_num(double value);

HLL_PUB
hll_value hll_new_symbol(struct hll_vm *vm, const char *symbol, size_t length);

HLL_PUB
hll_value hll_new_symbolz(struct hll_vm *vm, const char *symbol);

HLL_PUB
hll_value hll_new_cons(struct hll_vm *vm, hll_value car, hll_value cdr);
HLL_PUB
hll_value hll_new_env(struct hll_vm *vm, hll_value up, hll_value vars);
HLL_PUB
hll_value hll_new_bind(struct hll_vm *vm,
                       hll_value (*bind)(struct hll_vm *vm, hll_value args));
HLL_PUB
hll_value hll_new_func(struct hll_vm *vm, hll_value params,
                       struct hll_bytecode *bytecode, const char *name);

HLL_PUB
hll_value hll_new_macro(struct hll_vm *vm, hll_value params,
                        struct hll_bytecode *bytecode, const char *name);

HLL_PUB
struct hll_obj_cons *hll_unwrap_cons(hll_value value);

HLL_PUB
hll_value hll_unwrap_cdr(hll_value value);

HLL_PUB
hll_value hll_unwrap_car(hll_value value);

HLL_PUB
struct hll_obj_env *hll_unwrap_env(hll_value value);

HLL_PUB
const char *hll_unwrap_zsymb(hll_value value);

HLL_PUB
struct hll_obj_symb *hll_unwrap_symb(hll_value value);
HLL_PUB
struct hll_obj_bind *hll_unwrap_bind(hll_value value);
HLL_PUB
struct hll_obj_func *hll_unwrap_func(hll_value value);
HLL_PUB
struct hll_obj_func *hll_unwrap_macro(hll_value value);
HLL_PUB
double hll_unwrap_num(hll_value value);

HLL_PUB
size_t hll_list_length(hll_value value);

HLL_PUB
void hll_gray_obj(struct hll_vm *vm, hll_value value);
HLL_PUB
void hll_blacken_obj(struct hll_vm *vm, hll_value value);

HLL_PUB
hll_object_kind hll_get_value_kind(hll_value value);

HLL_PUB bool hll_is_nil(hll_value value);
HLL_PUB bool hll_is_num(hll_value value);
HLL_PUB bool hll_is_cons(hll_value value);
HLL_PUB bool hll_is_symb(hll_value value);
HLL_PUB bool hll_is_list(hll_value value);

HLL_PUB hll_value hll_car(hll_value lis);
HLL_PUB hll_value hll_cdr(hll_value lis);

#endif
