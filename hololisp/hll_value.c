#include "hll_hololisp.h"

#include <assert.h>
#include <string.h>

static uint32_t djb2(const char *src, const char *dst) {
  uint32_t hash = 5381;
  do {
    int c = *src++;
    hash = ((hash << 5) + hash) + c;
  } while (src != dst);
  return hash;
}

const char *hll_value_kind_str_(uint8_t kind) {
  static const char *strs[] = {"num",  "nil",  "true", "cons",
                               "symb", "bind", "env",  "func"};
  assert(kind < sizeof(strs) / sizeof(strs[0]));
  return strs[kind];
}

const char *hll_value_kind_str(hll_value value) {
  return hll_value_kind_str_(hll_value_kind(value));
}

void hll_free_obj(hll_vm *vm, hll_obj *obj) {
  switch (obj->kind) {
  case HLL_VALUE_CONS:
    hll_gc_free(vm->gc, obj, sizeof(hll_obj) + sizeof(hll_obj_cons));
    break;
  case HLL_VALUE_SYMB:
    hll_gc_free(vm->gc, obj,
                sizeof(hll_obj) + sizeof(hll_obj_symb) +
                    ((hll_obj_symb *)obj->as)->length + 1);
    break;
  case HLL_VALUE_BIND:
    hll_gc_free(vm->gc, obj, sizeof(hll_obj) + sizeof(hll_obj_bind));
    break;
  case HLL_VALUE_ENV:
    hll_gc_free(vm->gc, obj, sizeof(hll_obj) + sizeof(hll_obj_env));
    break;
  case HLL_VALUE_FUNC:
    hll_bytecode_dec_refcount(((hll_obj_func *)obj->as)->bytecode);
    hll_gc_free(vm->gc, obj, sizeof(hll_obj) + sizeof(hll_obj_func));
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

static void register_gc_obj(hll_vm *vm, hll_obj *obj) {
  obj->next_gc = vm->gc->all_objs;
  vm->gc->all_objs = obj;
}

hll_value hll_new_symbol(hll_vm *vm, const char *symbol, size_t length) {
  assert(symbol != NULL);
  assert(length != 0);
  assert(length < UINT32_MAX);

  void *memory =
      hll_gc_alloc(vm->gc, sizeof(hll_obj) + sizeof(hll_obj_symb) + length + 1);
  hll_obj *obj = memory;
  obj->kind = HLL_VALUE_SYMB;

  hll_obj_symb *symb = (void *)(obj + 1);
  symb->length = length;
  symb->hash = djb2(symbol, symbol + length);
  memcpy(symb->symb, symbol, length);
  register_gc_obj(vm, obj);

  return nan_box_ptr(obj);
}

hll_value hll_new_symbolz(hll_vm *vm, const char *symbol) {
  return hll_new_symbol(vm, symbol, strlen(symbol));
}

hll_value hll_new_cons(hll_vm *vm, hll_value car, hll_value cdr) {
  void *memory = hll_gc_alloc(vm->gc, sizeof(hll_obj) + sizeof(hll_obj_cons));
  hll_obj *obj = memory;
  obj->kind = HLL_VALUE_CONS;
  hll_obj_cons *cons = (void *)(obj + 1);
  cons->car = car;
  cons->cdr = cdr;
  register_gc_obj(vm, obj);

  return nan_box_ptr(obj);
}

hll_value hll_new_bind(hll_vm *vm,
                       hll_value (*bind)(hll_vm *vm, hll_value args)) {
  assert(bind != NULL);
  void *memory = hll_gc_alloc(vm->gc, sizeof(hll_obj) + sizeof(hll_obj_bind));
  hll_obj *obj = memory;
  obj->kind = HLL_VALUE_BIND;
  hll_obj_bind *binding = (void *)(obj + 1);
  binding->bind = bind;
  register_gc_obj(vm, obj);

  return nan_box_ptr(obj);
}

hll_value hll_new_env(hll_vm *vm, hll_value up, hll_value vars) {
  void *memory = hll_gc_alloc(vm->gc, sizeof(hll_obj) + sizeof(hll_obj_env));
  hll_obj *obj = memory;
  obj->kind = HLL_VALUE_ENV;
  hll_obj_env *env = (void *)(obj + 1);
  env->up = up;
  env->vars = vars;
  register_gc_obj(vm, obj);

  return nan_box_ptr(obj);
}

hll_value hll_new_func(hll_vm *vm, hll_value params, hll_bytecode *bytecode) {
  void *memory = hll_gc_alloc(vm->gc, sizeof(hll_obj) + sizeof(hll_obj_func));
  hll_obj *obj = memory;
  obj->kind = HLL_VALUE_FUNC;
  hll_obj_func *func = (void *)(obj + 1);
  func->param_names = params;
  func->bytecode = bytecode;
  register_gc_obj(vm, obj);
  hll_bytecode_inc_refcount(bytecode);

  return nan_box_ptr(obj);
}

size_t hll_list_length(hll_value value) {
  size_t length = 0;
  while (hll_is_cons(value)) {
    ++length;
    value = hll_unwrap_cdr(value);
  }

  return length;
}
