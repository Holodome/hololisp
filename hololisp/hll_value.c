#include "hll_value.h"

#include <assert.h>
#include <string.h>

#include "hll_bytecode.h"
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

static struct hll_obj *nan_unbox_ptr(hll_value value) {
  return (struct hll_obj *)(uintptr_t)(value & ~(HLL_SIGN_BIT | HLL_QNAN));
}

static uint32_t djb2(const char *src, const char *dst) {
  uint32_t hash = 5381;
  do {
    int c = *src++;
    hash = ((hash << 5) + hash) + c;
  } while (src != dst);

  return hash;
}

const char *hll_get_object_kind_str(hll_value_kind kind) {
  static const char *strs[] = {"num",  "nil", "true", "cons", "symb",
                               "bind", "env", "func", "macro"};

  assert(kind < sizeof(strs) / sizeof(strs[0]));
  return strs[kind];
}

void hll_free_obj(struct hll_vm *vm, struct hll_obj *obj) {
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

static void register_gc_obj(struct hll_vm *vm, struct hll_obj *obj) {
  obj->next_gc = vm->all_objs;
  vm->all_objs = obj;
}

hll_value hll_nil(void) { return nan_box_singleton(HLL_OBJ_NIL); }
hll_value hll_true(void) { return nan_box_singleton(HLL_OBJ_TRUE); }

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
  register_gc_obj(vm, obj);

  return nan_box_ptr(obj);
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
  register_gc_obj(vm, obj);

  return nan_box_ptr(obj);
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
  register_gc_obj(vm, obj);

  return nan_box_ptr(obj);
}

hll_value hll_new_env(struct hll_vm *vm, hll_value up, hll_value vars) {
  void *memory =
      hll_gc_alloc(vm, sizeof(struct hll_obj) + sizeof(struct hll_obj_env));
  struct hll_obj *obj = memory;
  obj->kind = HLL_OBJ_ENV;
  struct hll_obj_env *env = (void *)(obj + 1);
  env->up = up;
  env->vars = vars;
  register_gc_obj(vm, obj);

  return nan_box_ptr(obj);
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
  register_gc_obj(vm, obj);
  hll_bytecode_inc_refcount(bytecode);

  return nan_box_ptr(obj);
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
  register_gc_obj(vm, obj);
  hll_bytecode_inc_refcount(bytecode);

  return nan_box_ptr(obj);
}

struct hll_obj_cons *hll_unwrap_cons(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_CONS);
  return (struct hll_obj_cons *)obj->as;
}

const char *hll_unwrap_zsymb(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_SYMB);
  return ((struct hll_obj_symb *)obj->as)->symb;
}

struct hll_obj_symb *hll_unwrap_symb(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_SYMB);
  return (struct hll_obj_symb *)obj->as;
}

hll_value hll_unwrap_cdr(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_CONS);
  return ((struct hll_obj_cons *)obj->as)->cdr;
}

hll_value hll_unwrap_car(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_CONS);
  return ((struct hll_obj_cons *)obj->as)->car;
}

struct hll_obj_bind *hll_unwrap_bind(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_BIND);
  return (struct hll_obj_bind *)obj->as;
}

struct hll_obj_env *hll_unwrap_env(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_ENV);
  return (struct hll_obj_env *)obj->as;
}

struct hll_obj_func *hll_unwrap_func(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_FUNC);
  return (struct hll_obj_func *)obj->as;
}

struct hll_obj_func *hll_unwrap_macro(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = nan_unbox_ptr(value);
  assert(obj->kind == HLL_OBJ_MACRO);
  return (struct hll_obj_func *)obj->as;
}

double hll_unwrap_num(hll_value value) {
  assert(hll_is_num(value));
  double result;
  memcpy(&result, &value, sizeof(hll_value));
  return result;
}

size_t hll_list_length(hll_value value) {
  size_t length = 0;
  while (hll_is_cons(value)) {
    ++length;
    value = hll_unwrap_cdr(value);
  }

  return length;
}

void hll_gray_obj(struct hll_vm *vm, hll_value value) {
  if (!hll_is_obj(value)) {
    return;
  }

  struct hll_obj *obj = nan_unbox_ptr(value);
  if (obj->is_dark) {
    return;
  }

  obj->is_dark = true;
  hll_sb_push(vm->gray_objs, value);
}

void hll_blacken_obj(struct hll_vm *vm, hll_value value) {
  if (!hll_is_obj(value)) {
    return;
  }

  struct hll_obj *obj = nan_unbox_ptr(value);
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

hll_value_kind hll_get_value_kind(hll_value value) {
  return hll_is_obj(value) ? nan_unbox_ptr(value)->kind
                           : nan_unbox_singleton(value);
}

bool hll_is_nil(hll_value value) { return (uint64_t)value == hll_nil(); }

bool hll_is_cons(hll_value value) {
  return hll_is_obj(value) && nan_unbox_ptr(value)->kind == HLL_OBJ_CONS;
}

bool hll_is_symb(hll_value value) {
  return hll_is_obj(value) && nan_unbox_ptr(value)->kind == HLL_OBJ_SYMB;
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
