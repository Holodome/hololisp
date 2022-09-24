//
// hll_value.h
//
// Defines common methods for manipulating hololisp values.
#ifndef HLL_VALUE_H
#define HLL_VALUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hll_hololisp.h"

#define HLL_MAX_SYMB_LENGTH 128

typedef uint8_t hll_value_kind;
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
  HLL_VALUE_MACRO = 0x8,
};

HLL_PUB const char *hll_get_value_kind_str(hll_value_kind kind);

typedef struct hll_obj {
  hll_value_kind kind;
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

HLL_PUB hll_value hll_nil(void);
HLL_PUB hll_value hll_true(void);
HLL_PUB hll_value hll_num(double value);
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
HLL_PUB hll_value hll_new_macro(struct hll_vm *vm, hll_value params,
                                struct hll_bytecode *bytecode);

void hll_free_obj(struct hll_vm *vm, hll_obj *obj);

//
// Unwrapper functions.
// Unwrappers are used to get internal contents of value of expected type.
// If value type is not equal to expected, panic.
//

HLL_PUB double hll_unwrap_num(hll_value value);
HLL_PUB hll_value hll_unwrap_cdr(hll_value value);
HLL_PUB hll_value hll_unwrap_car(hll_value value);
HLL_PUB hll_obj_cons *hll_unwrap_cons(hll_value value)
    __attribute__((returns_nonnull));
HLL_PUB hll_obj_env *hll_unwrap_env(hll_value value)
    __attribute__((returns_nonnull));
HLL_PUB const char *hll_unwrap_zsymb(hll_value value)
    __attribute__((returns_nonnull));
HLL_PUB hll_obj_symb *hll_unwrap_symb(hll_value value)
    __attribute__((returns_nonnull));
HLL_PUB hll_obj_bind *hll_unwrap_bind(hll_value value)
    __attribute__((returns_nonnull));
HLL_PUB hll_obj_func *hll_unwrap_func(hll_value value)
    __attribute__((returns_nonnull));
HLL_PUB hll_obj_func *hll_unwrap_macro(hll_value value)
    __attribute__((returns_nonnull));

hll_obj *hll_unwrap_obj(hll_value value);

//
// Type-checking functions. Generally specific function 'hll_is_nil'
// is preferred to hll_get_value_kind() == HLL_VALUE_NIL because it makes
// compiler optimizations easier.
//

HLL_PUB hll_value_kind hll_get_value_kind(hll_value value);
HLL_PUB bool hll_is_nil(hll_value value);
HLL_PUB bool hll_is_num(hll_value value);
HLL_PUB bool hll_is_cons(hll_value value);
HLL_PUB bool hll_is_symb(hll_value value);
HLL_PUB bool hll_is_list(hll_value value);

bool hll_is_obj(hll_value value);

//
// Accessor overriding functions. By lisp standards, car returns car of object
// if object is cons, nil if object is nil and panics otherwise.
//

HLL_PUB hll_value hll_car(hll_value lis);
HLL_PUB hll_value hll_cdr(hll_value lis);
HLL_PUB size_t hll_list_length(hll_value value);


#endif
