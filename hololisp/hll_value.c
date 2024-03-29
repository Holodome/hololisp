#include "hll_value.h"

#include <assert.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_gc.h"
#include "hll_mem.h"
#include "hll_util.h"
#include "hll_vm.h"

#define HLL_SIGN_BIT ((uint64_t)1 << 63)
#define HLL_QNAN ((uint64_t)0x7ffc000000000000)

bool hll_is_num(hll_value value) { return (value & HLL_QNAN) != HLL_QNAN; }
bool hll_is_obj(hll_value value) {
  return (((value) & (HLL_QNAN | HLL_SIGN_BIT)) == (HLL_QNAN | HLL_SIGN_BIT));
}

static hll_value nan_box_singleton(hll_value_kind kind) {
  return HLL_QNAN | kind;
}

static hll_value_kind nan_unbox_singleton(hll_value value) {
  return value & ~(HLL_QNAN);
}

static hll_value nan_box_ptr(void *ptr) {
  return ((uintptr_t)ptr) | (HLL_SIGN_BIT | HLL_QNAN);
}

static hll_obj *nan_unbox_ptr(hll_value value) {
  return (hll_obj *)(uintptr_t)(value & ~(HLL_SIGN_BIT | HLL_QNAN));
}

static uint32_t djb2(const char *src, const char *dst) {
  uint32_t hash = 5381;
  do {
    int c = *src++;
    hash = ((hash << 5) + hash) + c;
  } while (src != dst);

  return hash;
}

const char *hll_get_value_kind_str(hll_value_kind kind) {
  static const char *strs[] = {"num",  "nil",  "true", "cons",
                               "symb", "bind", "env",  "func"};

  assert(kind < sizeof(strs) / sizeof(strs[0]));
  return strs[kind];
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

hll_value hll_nil(void) { return nan_box_singleton(HLL_VALUE_NIL); }
hll_value hll_true(void) { return nan_box_singleton(HLL_VALUE_TRUE); }

hll_value hll_num(double num) {
  hll_value value;
  memcpy(&value, &num, sizeof(hll_value));
  return value;
}

hll_value hll_new_symbol(hll_vm *vm, const char *symbol, size_t length) {
  assert(symbol != NULL);
  assert(length != 0);

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

hll_obj_cons *hll_unwrap_cons(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_CONS);
  return (hll_obj_cons *)obj->as;
}

const char *hll_unwrap_zsymb(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_SYMB);
  return ((hll_obj_symb *)obj->as)->symb;
}

hll_obj_symb *hll_unwrap_symb(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_SYMB);
  return (hll_obj_symb *)obj->as;
}

hll_value hll_unwrap_cdr(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_CONS);
  return ((hll_obj_cons *)obj->as)->cdr;
}

hll_value hll_unwrap_car(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_CONS);
  return ((hll_obj_cons *)obj->as)->car;
}

hll_obj_bind *hll_unwrap_bind(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_BIND);
  return (hll_obj_bind *)obj->as;
}

hll_obj_env *hll_unwrap_env(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_ENV);
  return (hll_obj_env *)obj->as;
}

hll_obj_func *hll_unwrap_func(hll_value value) {
  assert(hll_is_obj(value));
  hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_VALUE_FUNC);
  return (hll_obj_func *)obj->as;
}

double hll_unwrap_num(hll_value value) {
  assert(hll_is_num(value));
  double result;
  memcpy(&result, &value, sizeof(hll_value));
  return result;
}

hll_obj *hll_unwrap_obj(hll_value value) {
  assert(hll_is_obj(value));
  return nan_unbox_ptr(value);
}

void hll_setcar(hll_value cons, hll_value car) {
  hll_unwrap_cons(cons)->car = car;
}

void hll_setcdr(hll_value cons, hll_value cdr) {
  hll_unwrap_cons(cons)->cdr = cdr;
}

size_t hll_list_length(hll_value value) {
  size_t length = 0;
  while (hll_is_cons(value)) {
    ++length;
    value = hll_unwrap_cdr(value);
  }

  return length;
}

hll_value_kind hll_get_value_kind(hll_value value) {
  return hll_is_obj(value) ? nan_unbox_ptr(value)->kind
                           : nan_unbox_singleton(value);
}

bool hll_is_nil(hll_value value) { return value == hll_nil(); }

bool hll_is_cons(hll_value value) {
  return hll_is_obj(value) && nan_unbox_ptr(value)->kind == HLL_VALUE_CONS;
}

bool hll_is_symb(hll_value value) {
  return hll_is_obj(value) && nan_unbox_ptr(value)->kind == HLL_VALUE_SYMB;
}

bool hll_is_list(hll_value value) {
  return hll_is_cons(value) || hll_is_nil(value);
}
