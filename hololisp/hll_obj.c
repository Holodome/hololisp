#include "hll_obj.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hll_util.h"
#include "hll_vm.h"

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
  default:
#if HLL_DEBUG
    HLL_UNREACHABLE;
#endif
    break;
  }

  return str;
}

void free_object(struct hll_vm *vm, struct hll_obj *obj) {
  (void)vm;
  switch (obj->kind) {
  case HLL_OBJ_CONS:
    free(obj->as.body);
    break;
  case HLL_OBJ_SYMB:
    free(obj->as.body);
    break;
  case HLL_OBJ_BIND:
    free(obj->as.body);
    break;
  case HLL_OBJ_NIL:
    break;
  case HLL_OBJ_NUM:
    break;
  case HLL_OBJ_ENV:
    break;
  case HLL_OBJ_TRUE:
    break;
  case HLL_OBJ_FUNC:
    break;
  }

  free(obj);
}

hll_obj *hll_new_nil(struct hll_vm *vm) {
  (void)vm;
  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_NIL;
  return obj;
}

hll_obj *hll_new_true(struct hll_vm *vm) {
  (void)vm;
  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_TRUE;
  return obj;
}

hll_obj *hll_new_num(struct hll_vm *vm, double num) {
  (void)vm;
  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_NUM;
  obj->as.num = num;
  return obj;
}

hll_obj *hll_new_symbol(struct hll_vm *vm, const char *symbol, size_t length) {
  (void)vm;
  hll_obj_symb *body = calloc(1, sizeof(hll_obj_symb) + length + 1);
  memcpy(body->symb, symbol, length);
  body->symb[length] = '\0';
  body->length = length;

  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_SYMB;
  obj->as.body = body;

  return obj;
}

hll_obj *hll_new_symbolz(struct hll_vm *vm, const char *symbol) {
  return hll_new_symbol(vm, symbol, strlen(symbol));
}

hll_obj *hll_new_cons(struct hll_vm *vm, hll_obj *car, hll_obj *cdr) {
  (void)vm;
  hll_obj_cons *body = calloc(1, sizeof(hll_obj_cons));
  body->car = car;
  body->cdr = cdr;

  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_CONS;
  obj->as.body = body;

  return obj;
}

hll_obj *hll_new_bind(struct hll_vm *vm,
                      struct hll_obj *(*bind)(struct hll_vm *vm,
                                              struct hll_obj *args)) {
  (void)vm;
  hll_obj_bind *body = calloc(1, sizeof(hll_obj_bind));
  body->bind = bind;

  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_BIND;
  obj->as.body = body;

  return obj;
}

hll_obj *hll_new_env(struct hll_vm *vm, hll_obj *up, hll_obj *vars) {
  (void)vm;
  hll_obj_env *body = calloc(1, sizeof(hll_obj_env));
  body->up = up;
  body->vars = vars;

  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_ENV;
  obj->as.body = body;

  return obj;
}

hll_obj *hll_new_func(struct hll_vm *vm, struct hll_obj *params,
                      struct hll_bytecode *bytecode, const char *name) {
  (void)vm;
  hll_obj_func *body = calloc(1, sizeof(hll_obj_func));
  body->param_names = params;
  body->bytecode = bytecode;
  body->name = name;

  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_FUNC;
  obj->as.body = body;

  return obj;
}

hll_obj_cons *hll_unwrap_cons(const struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return (hll_obj_cons *)obj->as.body;
}

const char *hll_unwrap_zsymb(const struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_SYMB);
  return ((hll_obj_symb *)obj->as.body)->symb;
}

hll_obj_symb *hll_unwrap_symb(const struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_SYMB);
  return (hll_obj_symb *)obj->as.body;
}

hll_obj *hll_unwrap_cdr(const struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return ((hll_obj_cons *)obj->as.body)->cdr;
}

hll_obj *hll_unwrap_car(const struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return ((hll_obj_cons *)obj->as.body)->car;
}

hll_obj_bind *hll_unwrap_bind(const struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_BIND);
  return (hll_obj_bind *)obj->as.body;
}

hll_obj_env *hll_unwrap_env(const struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_ENV);
  return (hll_obj_env *)obj->as.body;
}

hll_obj_func *hll_unwrap_func(const struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_FUNC);
  return (hll_obj_func *)obj->as.body;
}

size_t hll_list_length(const struct hll_obj *obj) {
  size_t length = 0;
  while (obj->kind == HLL_OBJ_CONS) {
    ++length;
    obj = hll_unwrap_cdr(obj);
  }

  return length;
}

hll_obj *hll_copy_obj(struct hll_vm *vm, struct hll_obj *src) {
  hll_obj *result = NULL;
  switch (src->kind) {
  case HLL_OBJ_CONS:
    result = hll_new_cons(vm, hll_unwrap_car(src), hll_unwrap_cdr(src));
    break;
  case HLL_OBJ_SYMB: {
    hll_obj_symb *symb = hll_unwrap_symb(src);
    result = hll_new_symbol(vm, symb->symb, symb->length);
  } break;
  case HLL_OBJ_NIL:
    result = vm->nil;
    break;
  case HLL_OBJ_NUM:
    result = hll_new_num(vm, src->as.num);
    break;
  case HLL_OBJ_BIND:
    result = hll_new_bind(vm, hll_unwrap_bind(src)->bind);
    break;
  case HLL_OBJ_ENV: {
    hll_obj_env *env = hll_unwrap_env(src);
    result = hll_new_env(vm, env->up, env->vars);
  } break;
  case HLL_OBJ_TRUE:
    result = vm->true_;
    break;
  case HLL_OBJ_FUNC: {
    hll_obj_func *func = hll_unwrap_func(src);
    result = hll_new_func(vm, func->param_names, func->bytecode, func->name);
    hll_unwrap_func(result)->var_list = vm->nil;
  } break;
  default:
    HLL_UNREACHABLE;
    break;
  }

  return result;
}
