#include "hll_obj.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hll_vm.h"

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

hll_obj *hll_new_num(struct hll_vm *vm, hll_num num) {
  (void)vm;
  hll_obj *obj = calloc(1, sizeof(hll_obj));
  obj->kind = HLL_OBJ_NUM;
  obj->as.num = num;
  return obj;
}

hll_obj *hll_new_symbol(struct hll_vm *vm, char const *symbol, size_t length) {
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

hll_obj_cons *hll_unwrap_cons(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return (hll_obj_cons *)obj->as.body;
}

const char *hll_unwrap_zsymb(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_SYMB);
  return ((hll_obj_symb *)obj->as.body)->symb;
}

hll_obj *hll_unwrap_cdr(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return ((hll_obj_cons *)obj->as.body)->cdr;
}

hll_obj *hll_unwrap_car(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return ((hll_obj_cons *)obj->as.body)->car;
}

hll_obj_bind *hll_unwrap_bind(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_BIND);
  return (hll_obj_bind *)obj->as.body;
}

hll_obj_env *hll_unwrap_env(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_ENV);
  return (hll_obj_env *)obj->as.body;
}
