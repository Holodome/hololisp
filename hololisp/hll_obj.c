#include "hll_obj.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_mem.h"
#include "hll_util.h"
#include "hll_vm.h"

#define HLL_SIGN_BIT ((uint64_t)1 << 63)
#define HLL_QNAN ((uint64_t)0x7ffc000000000000)

#define HLL_IS_NUM(_value) (((_value)&HLL_QNAN) != HLL_QNAN)
#define HLL_IS_OBJ(_value)                                                     \
  (((_value) & (HLL_QNAN | HLL_SIGN_BIT)) == (HLL_QNAN | HLL_SIGN_BIT))

#define HLL_NAN_BOX_KIND(_kind) ((hll_value)(uint64_t)(HLL_QNAN | (_kind)))
#define HLL_NAN_BOX_OBJ(_ptr)                                                  \
  ((hll_value)(((uint64_t)(uintptr_t)(_ptr)) | (HLL_SIGN_BIT | HLL_QNAN)))
#define HLL_NAN_BOX_OBJ_UNWRAP(_value)                                         \
  ((struct hll_obj *)(uintptr_t)((_value) & ~(HLL_SIGN_BIT | HLL_QNAN)))
#define HLL_VALUE_KIND(_value) ((uint64_t)(_value) & ~HLL_QNAN)

static uint32_t djb2(const char *src, const char *dst) {
  uint32_t hash = 5381;
  do {
    int c = *src++;
    hash = ((hash << 5) + hash) + c;
  } while (src != dst);

  return hash;
}

const char *hll_get_object_kind_str(hll_object_kind kind) {
  const char *str = NULL;
  switch (kind) {
  case HLL_OBJ_CONS:
    str = "cons";
    break;
  case HLL_OBJ_SYMB:
    str = "symb";
    break;
  case HLL_OBJ_NIL:
    str = "nil";
    break;
  case HLL_OBJ_NUM:
    str = "num";
    break;
  case HLL_OBJ_BIND:
    str = "bind";
    break;
  case HLL_OBJ_ENV:
    str = "env";
    break;
  case HLL_OBJ_TRUE:
    str = "true";
    break;
  case HLL_OBJ_FUNC:
    str = "func";
    break;
  case HLL_OBJ_MACRO:
    str = "macro";
    break;
  default:
#if HLL_DEBUG
    HLL_UNREACHABLE;
#endif
    break;
  }

  return str;
}

void hll_free_object(struct hll_vm *vm, struct hll_obj *obj) {
  switch (obj->kind) {
  case HLL_OBJ_CONS:
    hll_gc_free(vm, obj, sizeof(struct hll_obj) + sizeof(struct hll_obj_cons));
    break;
  case HLL_OBJ_SYMB:
    hll_gc_free(vm, obj,
                sizeof(struct hll_obj) + sizeof(struct hll_obj_symb) +
                    ((struct hll_obj_symb *)obj->as)->length + 1);
    break;
  case HLL_OBJ_BIND:
    hll_gc_free(vm, obj, sizeof(struct hll_obj) + sizeof(struct hll_obj_bind));
    break;
  case HLL_OBJ_ENV:
    hll_gc_free(vm, obj, sizeof(struct hll_obj) + sizeof(struct hll_obj_env));
    break;
  case HLL_OBJ_FUNC:
    hll_bytecode_dec_refcount(((struct hll_obj_func *)obj->as)->bytecode);
    hll_gc_free(vm, obj, sizeof(struct hll_obj) + sizeof(struct hll_obj_func));
    break;
  case HLL_OBJ_MACRO:
    hll_bytecode_dec_refcount(((struct hll_obj_func *)obj->as)->bytecode);
    hll_gc_free(vm, obj, sizeof(struct hll_obj) + sizeof(struct hll_obj_func));
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

void register_object(struct hll_vm *vm, struct hll_obj *obj) {
  obj->next_gc = vm->all_objects;
  vm->all_objects = obj;
}

hll_value hll_nil(void) {
  hll_value value = HLL_NAN_BOX_KIND(HLL_OBJ_NIL);
  return value;
}

hll_value hll_true(void) {
  hll_value value = HLL_NAN_BOX_KIND(HLL_OBJ_TRUE);
  return value;
}

hll_value hll_num(double num) {
  hll_value value;
  memcpy(&value, &num, sizeof(hll_value));
  return value;
}

hll_value hll_new_symbol(struct hll_vm *vm, const char *symbol, size_t length) {
  assert(symbol != NULL);
  assert(length != 0);
  if (length >= HLL_MAX_SYMB_LENGTH) {
    // todo error
    assert(0);
  }

  void *memory = hll_gc_alloc(vm, sizeof(struct hll_obj) +
                                      sizeof(struct hll_obj_symb) + length + 1);
  struct hll_obj *obj = memory;
  obj->kind = HLL_OBJ_SYMB;

  struct hll_obj_symb *symb = (void *)(obj + 1);
  symb->length = length;
  symb->hash = djb2(symbol, symbol + length);
  memcpy(symb->symb, symbol, length);

  register_object(vm, obj);
  return HLL_NAN_BOX_OBJ(obj);
}

hll_value hll_new_symbolz(struct hll_vm *vm, const char *symbol) {
  return hll_new_symbol(vm, symbol, strlen(symbol));
}

hll_value hll_new_cons(struct hll_vm *vm, hll_value car, hll_value cdr) {
  void *memory =
      hll_gc_alloc(vm, sizeof(struct hll_obj) + sizeof(struct hll_obj_cons));
  struct hll_obj *obj = memory;
  obj->kind = HLL_OBJ_CONS;
  struct hll_obj_cons *cons = (void *)(obj + 1);
  cons->car = car;
  cons->cdr = cdr;
  register_object(vm, obj);

  return HLL_NAN_BOX_OBJ(obj);
}

hll_value hll_new_bind(struct hll_vm *vm,
                       hll_value (*bind)(struct hll_vm *vm, hll_value args)) {
  assert(bind != NULL);
  void *memory =
      hll_gc_alloc(vm, sizeof(struct hll_obj) + sizeof(struct hll_obj_bind));
  struct hll_obj *obj = memory;
  obj->kind = HLL_OBJ_BIND;
  struct hll_obj_bind *binding = (void *)(obj + 1);
  binding->bind = bind;
  register_object(vm, obj);

  return HLL_NAN_BOX_OBJ(obj);
}

hll_value hll_new_env(struct hll_vm *vm, hll_value up, hll_value vars) {
  void *memory =
      hll_gc_alloc(vm, sizeof(struct hll_obj) + sizeof(struct hll_obj_env));
  struct hll_obj *obj = memory;
  obj->kind = HLL_OBJ_ENV;
  struct hll_obj_env *env = (void *)(obj + 1);
  env->up = up;
  env->vars = vars;
  register_object(vm, obj);

  return HLL_NAN_BOX_OBJ(obj);
}

hll_value hll_new_func(struct hll_vm *vm, hll_value params,
                       struct hll_bytecode *bytecode) {
  void *memory =
      hll_gc_alloc(vm, sizeof(struct hll_obj) + sizeof(struct hll_obj_func));
  struct hll_obj *obj = memory;
  obj->kind = HLL_OBJ_FUNC;
  struct hll_obj_func *func = (void *)(obj + 1);
  func->param_names = params;
  func->bytecode = bytecode;
  register_object(vm, obj);
  hll_bytecode_inc_refcount(bytecode);

  return HLL_NAN_BOX_OBJ(obj);
}

hll_value hll_new_macro(struct hll_vm *vm, hll_value params,
                        struct hll_bytecode *bytecode) {
  void *memory =
      hll_gc_alloc(vm, sizeof(struct hll_obj) + sizeof(struct hll_obj_func));
  struct hll_obj *obj = memory;
  obj->kind = HLL_OBJ_MACRO;
  struct hll_obj_func *func = (void *)(obj + 1);
  func->param_names = params;
  func->bytecode = bytecode;
  register_object(vm, obj);
  hll_bytecode_inc_refcount(bytecode);

  return HLL_NAN_BOX_OBJ(obj);
}

struct hll_obj_cons *hll_unwrap_cons(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_CONS);
  return (struct hll_obj_cons *)obj->as;
}

const char *hll_unwrap_zsymb(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_SYMB);
  return ((struct hll_obj_symb *)obj->as)->symb;
}

struct hll_obj_symb *hll_unwrap_symb(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_SYMB);
  return (struct hll_obj_symb *)obj->as;
}

hll_value hll_unwrap_cdr(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_CONS);
  return ((struct hll_obj_cons *)obj->as)->cdr;
}

hll_value hll_unwrap_car(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_CONS);
  return ((struct hll_obj_cons *)obj->as)->car;
}

struct hll_obj_bind *hll_unwrap_bind(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_BIND);
  return (struct hll_obj_bind *)obj->as;
}

struct hll_obj_env *hll_unwrap_env(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_ENV);
  return (struct hll_obj_env *)obj->as;
}

struct hll_obj_func *hll_unwrap_func(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_FUNC);
  return (struct hll_obj_func *)obj->as;
}

struct hll_obj_func *hll_unwrap_macro(hll_value value) {
  assert(HLL_IS_OBJ(value));
  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  assert(obj->kind == HLL_OBJ_MACRO);
  return (struct hll_obj_func *)obj->as;
}

double hll_unwrap_num(hll_value value) {
  assert(HLL_IS_NUM(value));
  double result;
  memcpy(&result, &value, sizeof(hll_value));
  return result;
}

size_t hll_list_length(hll_value value) {
  size_t length = 0;
  while (hll_get_value_kind(value) == HLL_OBJ_CONS) {
    ++length;
    value = hll_unwrap_cdr(value);
  }

  return length;
}

hll_value hll_copy_obj(struct hll_vm *vm, hll_value src) {
  hll_value result;
  switch (hll_get_value_kind(src)) {
  case HLL_OBJ_NIL:
  case HLL_OBJ_TRUE:
  case HLL_OBJ_NUM:
    result = src;
    break;
  case HLL_OBJ_CONS:
    result = hll_new_cons(vm, hll_unwrap_car(src), hll_unwrap_cdr(src));
    break;
  case HLL_OBJ_SYMB: {
    struct hll_obj_symb *symb = hll_unwrap_symb(src);
    result = hll_new_symbol(vm, symb->symb, symb->length);
  } break;
  case HLL_OBJ_BIND:
    result = hll_new_bind(vm, hll_unwrap_bind(src)->bind);
    break;
  case HLL_OBJ_ENV: {
    struct hll_obj_env *env = hll_unwrap_env(src);
    result = hll_new_env(vm, env->up, env->vars);
  } break;
  case HLL_OBJ_MACRO: {
    struct hll_obj_func *func = hll_unwrap_macro(src);
    struct hll_bytecode *bytecode = func->bytecode;
    result = hll_new_macro(vm, func->param_names, bytecode);
  } break;
  case HLL_OBJ_FUNC: {
    struct hll_obj_func *func = hll_unwrap_func(src);
    struct hll_bytecode *bytecode = func->bytecode;
    result = hll_new_func(vm, func->param_names, bytecode);
  } break;
  default:
    HLL_UNREACHABLE;
    break;
  }

  return result;
}

void hll_gray_obj(struct hll_vm *vm, hll_value value) {
  if (!HLL_IS_OBJ(value)) {
    return;
  }

  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  if (obj->is_dark) {
    return;
  }

  obj->is_dark = true;
  hll_sb_push(vm->gray_objs, value);
}

void hll_blacken_obj(struct hll_vm *vm, hll_value value) {
  if (!HLL_IS_OBJ(value)) {
    return;
  }

  struct hll_obj *obj = HLL_NAN_BOX_OBJ_UNWRAP(value);
  vm->bytes_allocated += sizeof(struct hll_obj);

  switch (obj->kind) {
  case HLL_OBJ_CONS:
    hll_gray_obj(vm, hll_unwrap_car(value));
    hll_gray_obj(vm, hll_unwrap_cdr(value));
    break;
  case HLL_OBJ_SYMB:
    vm->bytes_allocated +=
        hll_unwrap_symb(value)->length + 1 + sizeof(struct hll_obj_symb);
    break;
  case HLL_OBJ_BIND:
    vm->bytes_allocated += sizeof(struct hll_obj_bind);
    break;
  case HLL_OBJ_ENV:
    vm->bytes_allocated += sizeof(struct hll_obj_env);
    hll_gray_obj(vm, hll_unwrap_env(value)->vars);
    hll_gray_obj(vm, hll_unwrap_env(value)->up);
    break;
  case HLL_OBJ_FUNC: {
    vm->bytes_allocated += sizeof(struct hll_obj_func);
    hll_gray_obj(vm, hll_unwrap_func(value)->param_names);
    hll_gray_obj(vm, hll_unwrap_func(value)->env);
    struct hll_bytecode *bytecode = hll_unwrap_func(value)->bytecode;
    for (size_t i = 0; i < hll_sb_len(bytecode->constant_pool); ++i) {
      hll_gray_obj(vm, bytecode->constant_pool[i]);
    }
  } break;
  case HLL_OBJ_MACRO: {
    vm->bytes_allocated += sizeof(struct hll_obj_func);
    hll_gray_obj(vm, hll_unwrap_macro(value)->param_names);
    hll_gray_obj(vm, hll_unwrap_macro(value)->env);
    struct hll_bytecode *bytecode = hll_unwrap_macro(value)->bytecode;
    for (size_t i = 0; i < hll_sb_len(bytecode->constant_pool); ++i) {
      hll_gray_obj(vm, bytecode->constant_pool[i]);
    }
  } break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

hll_object_kind hll_get_value_kind(hll_value value) {
  return HLL_IS_OBJ(value) ? HLL_NAN_BOX_OBJ_UNWRAP(value)->kind
                           : HLL_VALUE_KIND(value);
}

bool hll_is_nil(hll_value value) {
  return (uint64_t)value == HLL_NAN_BOX_KIND(HLL_OBJ_NIL);
}

bool hll_is_num(hll_value value) { return HLL_IS_NUM(value); }

bool hll_is_cons(hll_value value) {
  return HLL_IS_OBJ(value) &&
         HLL_NAN_BOX_OBJ_UNWRAP(value)->kind == HLL_OBJ_CONS;
}

bool hll_is_symb(hll_value value) {
  return HLL_IS_OBJ(value) &&
         HLL_NAN_BOX_OBJ_UNWRAP(value)->kind == HLL_OBJ_SYMB;
}

bool hll_is_list(hll_value value) {
  return hll_is_cons(value) || hll_is_nil(value);
}

HLL_PUB hll_value hll_car(hll_value lis) {
  if (hll_is_nil(lis)) {
    return lis;
  }

  return hll_unwrap_car(lis);
}
HLL_PUB hll_value hll_cdr(hll_value lis) {
  if (hll_is_nil(lis)) {
    return lis;
  }

  return hll_unwrap_cdr(lis);
}
