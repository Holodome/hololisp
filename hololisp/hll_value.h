//
// hll_value.h
//
// Defines common methods for manipulating hololisp values.
#ifndef HLL_VALUE_H
#define HLL_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "hll_hololisp.h"

enum {
  // Singleton values. These are not heap-allocated.
  HLL_VALUE_NUM = 0x0,
  HLL_VALUE_NIL = 0x1,
  HLL_VALUE_TRUE = 0x2,
  // Heap-allocated garbage-collected values
  HLL_VALUE_CONS = 0x3,
  HLL_VALUE_SYMB = 0x4,
  HLL_VALUE_BIND = 0x5,
  HLL_VALUE_ENV = 0x6,
  HLL_VALUE_FUNC = 0x7,
};

HLL_PUB const char *hll_get_value_kind_str(uint8_t kind);

typedef struct hll_obj {
  uint8_t kind;
  bool is_dark;
  struct hll_obj *next_gc;
  char as[];
} hll_obj;

typedef struct hll_obj_cons {
  hll_value car;
  hll_value cdr;
} hll_obj_cons;

typedef struct hll_obj_func {
  struct hll_bytecode *bytecode;
  // List if function parameter names
  hll_value param_names;
  hll_value env;
} hll_obj_func;

typedef struct hll_obj_env {
  hll_value vars;
  hll_value up;
} hll_obj_env;

typedef struct hll_obj_bind {
  hll_value (*bind)(struct hll_vm *vm, hll_value args);
} hll_obj_bind;

typedef struct hll_obj_symb {
  size_t length;
  uint32_t hash;
  char symb[];
} hll_obj_symb;

//
// Functions that create new values.
//

HLL_PUB hll_value hll_new_symbol(struct hll_vm *vm, const char *symbol,
                                 size_t length);
HLL_PUB hll_value hll_new_symbolz(struct hll_vm *vm, const char *symbol);
HLL_PUB hll_value hll_new_cons(struct hll_vm *vm, hll_value car, hll_value cdr);
HLL_PUB hll_value hll_new_env(struct hll_vm *vm, hll_value up, hll_value vars);
HLL_PUB hll_value hll_new_bind(struct hll_vm *vm,
                               hll_value (*bind)(struct hll_vm *vm,
                                                 hll_value args));
HLL_PUB hll_value hll_new_func(struct hll_vm *vm, hll_value params,
                               struct hll_bytecode *bytecode);

void hll_free_obj(struct hll_vm *vm, hll_obj *obj);

HLL_PUB size_t hll_list_length(hll_value value);

#define HLL_SIGN_BIT ((uint64_t)1 << 63)
#define HLL_QNAN ((uint64_t)0x7ffc000000000000)

static inline hll_value nan_box_singleton(uint8_t kind) {
  return HLL_QNAN | kind;
}

static inline uint8_t nan_unbox_singleton(hll_value value) {
  return value & ~(HLL_QNAN);
}

static inline hll_value nan_box_ptr(void *ptr) {
  return ((uintptr_t)ptr) | (HLL_SIGN_BIT | HLL_QNAN);
}

static inline hll_obj *nan_unbox_ptr(hll_value value) {
  return (hll_obj *)(uintptr_t)(value & ~(HLL_SIGN_BIT | HLL_QNAN));
}

static inline hll_value hll_nil(void) { return nan_box_singleton(HLL_VALUE_NIL); }
static inline hll_value hll_true(void) { return nan_box_singleton(HLL_VALUE_TRUE); }
static inline hll_value hll_num(double num) {
  hll_value value;
  memcpy(&value, &num, sizeof(hll_value));
  return value;
}

static inline bool hll_is_num(hll_value value) { return (value & HLL_QNAN) != HLL_QNAN; }
static inline bool hll_is_obj(hll_value value) {
  return (((value) & (HLL_QNAN | HLL_SIGN_BIT)) == (HLL_QNAN | HLL_SIGN_BIT));
}
static inline uint8_t hll_get_value_kind(hll_value value) {
  return hll_is_obj(value) ? nan_unbox_ptr(value)->kind
                           : nan_unbox_singleton(value);
}

static inline bool hll_is_nil(hll_value value) { return value == hll_nil(); }

static inline bool hll_is_cons(hll_value value) {
  return hll_is_obj(value) && nan_unbox_ptr(value)->kind == HLL_VALUE_CONS;
}

static inline bool hll_is_symb(hll_value value) {
  return hll_is_obj(value) && nan_unbox_ptr(value)->kind == HLL_VALUE_SYMB;
}

static inline bool hll_is_list(hll_value value) {
  return hll_is_cons(value) || hll_is_nil(value);
}

static inline hll_obj_cons *hll_unwrap_cons(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_CONS);
  return (hll_obj_cons *)obj->as;
}

static inline const char *hll_unwrap_zsymb(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_SYMB);
  return ((hll_obj_symb *)obj->as)->symb;
}

static inline hll_obj_symb *hll_unwrap_symb(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_SYMB);
  return (hll_obj_symb *)obj->as;
}

static inline hll_value hll_unwrap_cdr(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_CONS);
  return ((hll_obj_cons *)obj->as)->cdr;
}

static inline hll_value hll_unwrap_car(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_CONS);
  return ((hll_obj_cons *)obj->as)->car;
}

static inline hll_obj_bind *hll_unwrap_bind(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_BIND);
  return (hll_obj_bind *)obj->as;
}

static inline hll_obj_env *hll_unwrap_env(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_ENV);
  return (hll_obj_env *)obj->as;
}

static inline hll_obj_func *hll_unwrap_func(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_FUNC);
  return (hll_obj_func *)obj->as;
}

static inline double hll_unwrap_num(hll_value value) {
  assert(hll_is_num(value));
  double result;
  memcpy(&result, &value, sizeof(hll_value));
  return result;
}

static inline hll_obj *hll_unwrap_obj(hll_value value) {
  assert(hll_is_obj(value));
  return nan_unbox_ptr(value);
}

static inline void hll_setcar(hll_value cons, hll_value car) {
  hll_unwrap_cons(cons)->car = car;
}

static inline void hll_setcdr(hll_value cons, hll_value cdr) {
  hll_unwrap_cons(cons)->cdr = cdr;
}

#endif
