#include "hll_obj.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_mem.h"
#include "hll_util.h"
#include "hll_vm.h"

const char *hll_get_object_kind_str(enum hll_object_kind kind) {
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
#if 0
  fprintf(stderr, "freed object %s ",
          hll_get_object_kind_str(obj->kind));
  if (obj->kind == HLL_OBJ_SYMB) {
    fprintf(stderr, "%s", hll_unwrap_zsymb(obj));
  }
  //      hll_dump_object(stderr, to_free);
  fprintf(stderr, "\n");
#endif
  enum hll_object_kind kind = obj->kind;
  switch (kind) {
  case HLL_OBJ_NIL:
  case HLL_OBJ_TRUE:
  case HLL_OBJ_NUM:
  case HLL_OBJ_CONS:
    break;
  case HLL_OBJ_SYMB:
    hll_gc_free(vm, obj->as.body,
                sizeof(struct hll_obj_symb) + hll_unwrap_symb(obj)->length + 1);
    break;
  case HLL_OBJ_BIND:
    hll_free(obj->as.body, sizeof(struct hll_obj_bind));
    break;
  case HLL_OBJ_ENV:
    hll_free(obj->as.body, sizeof(struct hll_obj_env));
    break;
  case HLL_OBJ_FUNC:
    hll_free_bytecode(hll_unwrap_func(obj)->bytecode);
    hll_gc_free(vm, obj->as.body, sizeof(struct hll_obj_func));
    break;
  case HLL_OBJ_MACRO:
    hll_free_bytecode(hll_unwrap_macro(obj)->bytecode);
    hll_gc_free(vm, obj->as.body, sizeof(struct hll_obj_func));
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
  memset(obj, 0, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_FREED;

  hll_gc_free(vm, obj, sizeof(struct hll_obj));
}

void register_object(struct hll_vm *vm, struct hll_obj *obj) {
  //  fprintf(stderr, "registered object %s\n",
  //  hll_get_object_kind_str(obj->kind));
  obj->next_gc = vm->all_objects;
  vm->all_objects = obj;
}

struct hll_obj *hll_new_nil(struct hll_vm *vm) {
  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_NIL;

  register_object(vm, obj);
  return obj;
}

struct hll_obj *hll_new_true(struct hll_vm *vm) {
  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_TRUE;

  register_object(vm, obj);
  return obj;
}

struct hll_obj *hll_new_num(struct hll_vm *vm, double num) {
  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_NUM;
  obj->as.num = num;

  register_object(vm, obj);
  return obj;
}

struct hll_obj *hll_new_symbol(struct hll_vm *vm, const char *symbol,
                               size_t length) {
  assert(symbol);

  struct hll_obj_symb *body =
      hll_gc_alloc(vm, sizeof(struct hll_obj_symb) + length + 1);
  memcpy(body->symb, symbol, length);
  body->symb[length] = '\0';
  body->length = length;

  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_SYMB;
  obj->as.body = body;

  register_object(vm, obj);
  return obj;
}

struct hll_obj *hll_new_symbolz(struct hll_vm *vm, const char *symbol) {
  return hll_new_symbol(vm, symbol, strlen(symbol));
}

struct hll_obj *hll_new_cons(struct hll_vm *vm, struct hll_obj *car,
                             struct hll_obj *cdr) {
  assert(car && cdr);
  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_CONS;
  obj->as.cons.car = car;
  obj->as.cons.cdr = cdr;

  register_object(vm, obj);
  return obj;
}

struct hll_obj *hll_new_bind(struct hll_vm *vm,
                             struct hll_obj *(*bind)(struct hll_vm *vm,
                                                     struct hll_obj *args)) {
  assert(bind);
  struct hll_obj_bind *body = hll_gc_alloc(vm, sizeof(struct hll_obj_bind));
  body->bind = bind;

  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_BIND;
  obj->as.body = body;

  register_object(vm, obj);
  return obj;
}

struct hll_obj *hll_new_env(struct hll_vm *vm, struct hll_obj *up,
                            struct hll_obj *vars) {
  assert(up && vars);
  struct hll_obj_env *body = hll_gc_alloc(vm, sizeof(struct hll_obj_env));
  body->up = up;
  body->vars = vars;

  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_ENV;
  obj->as.body = body;

  register_object(vm, obj);
  return obj;
}

struct hll_obj *hll_new_func(struct hll_vm *vm, struct hll_obj *params,
                             struct hll_bytecode *bytecode, const char *name) {
  assert(params && bytecode && name);
  struct hll_obj_func *body = hll_gc_alloc(vm, sizeof(struct hll_obj_func));
  body->param_names = params;
  body->bytecode = bytecode;
  body->name = name;

  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_FUNC;
  obj->as.body = body;

  register_object(vm, obj);
  return obj;
}

struct hll_obj *hll_new_macro(struct hll_vm *vm, struct hll_obj *params,
                              struct hll_bytecode *bytecode, const char *name) {
  assert(params && bytecode && name);
  struct hll_obj_func *body = hll_gc_alloc(vm, sizeof(struct hll_obj_func));
  body->param_names = params;
  body->bytecode = bytecode;
  body->name = name;

  struct hll_obj *obj = hll_gc_alloc(vm, sizeof(struct hll_obj));
  obj->kind = HLL_OBJ_MACRO;
  obj->as.body = body;

  register_object(vm, obj);
  return obj;
}

struct hll_obj_cons *hll_unwrap_cons(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return (struct hll_obj_cons *)&obj->as.cons;
}

const char *hll_unwrap_zsymb(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_SYMB);
  return ((struct hll_obj_symb *)obj->as.body)->symb;
}

struct hll_obj_symb *hll_unwrap_symb(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_SYMB);
  return (struct hll_obj_symb *)obj->as.body;
}

struct hll_obj *hll_unwrap_cdr(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return obj->as.cons.cdr;
}

struct hll_obj *hll_unwrap_car(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_CONS);
  return obj->as.cons.car;
}

struct hll_obj_bind *hll_unwrap_bind(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_BIND);
  return (struct hll_obj_bind *)obj->as.body;
}

struct hll_obj_env *hll_unwrap_env(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_ENV);
  return (struct hll_obj_env *)obj->as.body;
}

struct hll_obj_func *hll_unwrap_func(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_FUNC);
  return (struct hll_obj_func *)obj->as.body;
}

struct hll_obj_func *hll_unwrap_macro(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_MACRO);
  return (struct hll_obj_func *)obj->as.body;
}

double hll_unwrap_num(struct hll_obj *obj) {
  assert(obj->kind == HLL_OBJ_NUM);
  return obj->as.num;
}

size_t hll_list_length(struct hll_obj *obj) {
  size_t length = 0;
  while (obj->kind == HLL_OBJ_CONS) {
    ++length;
    obj = hll_unwrap_cdr(obj);
  }

  return length;
}

struct hll_obj *hll_copy_obj(struct hll_vm *vm, struct hll_obj *src) {
  struct hll_obj *result = NULL;
  switch (src->kind) {
  case HLL_OBJ_CONS:
    result = hll_new_cons(vm, hll_unwrap_car(src), hll_unwrap_cdr(src));
    break;
  case HLL_OBJ_SYMB: {
    struct hll_obj_symb *symb = hll_unwrap_symb(src);
    result = hll_new_symbol(vm, symb->symb, symb->length);
  } break;
  case HLL_OBJ_NIL:
    result = vm->nil;
    break;
  case HLL_OBJ_NUM:
    result = hll_new_num(vm, hll_unwrap_num(src));
    break;
  case HLL_OBJ_BIND:
    result = hll_new_bind(vm, hll_unwrap_bind(src)->bind);
    break;
  case HLL_OBJ_ENV: {
    struct hll_obj_env *env = hll_unwrap_env(src);
    result = hll_new_env(vm, env->up, env->vars);
  } break;
  case HLL_OBJ_TRUE:
    result = vm->true_;
    break;
  case HLL_OBJ_MACRO: {
    struct hll_obj_func *func = hll_unwrap_macro(src);
    struct hll_bytecode *bytecode = hll_alloc(sizeof(struct hll_bytecode));
    for (size_t i = 0; i < hll_sb_len(func->bytecode->ops); ++i) {
      hll_sb_push(bytecode->ops, func->bytecode->ops[i]);
    }
    for (size_t i = 0; i < hll_sb_len(func->bytecode->constant_pool); ++i) {
      hll_sb_push(bytecode->constant_pool, func->bytecode->constant_pool[i]);
    }

    result = hll_new_macro(vm, func->param_names, bytecode, func->name);
    hll_unwrap_macro(result)->var_list = vm->nil;
  } break;
  case HLL_OBJ_FUNC: {
    struct hll_obj_func *func = hll_unwrap_func(src);
    struct hll_bytecode *bytecode = hll_alloc(sizeof(struct hll_bytecode));
    for (size_t i = 0; i < hll_sb_len(func->bytecode->ops); ++i) {
      hll_sb_push(bytecode->ops, func->bytecode->ops[i]);
    }
    for (size_t i = 0; i < hll_sb_len(func->bytecode->constant_pool); ++i) {
      hll_sb_push(bytecode->constant_pool, func->bytecode->constant_pool[i]);
    }

    result = hll_new_func(vm, func->param_names, bytecode, func->name);
    hll_unwrap_func(result)->var_list = vm->nil;
  } break;
  default:
    HLL_UNREACHABLE;
    break;
  }

  return result;
}

void hll_gray_obj(struct hll_vm *vm, struct hll_obj *obj) {
  if (obj == NULL || obj->is_dark) {
    return;
  }

  obj->is_dark = true;
  hll_sb_push(vm->gray_objs, obj);
}

void hll_blacken_obj(struct hll_vm *vm, struct hll_obj *obj) {
  vm->bytes_allocated += sizeof(struct hll_obj);
  switch (obj->kind) {
  case HLL_OBJ_CONS:
    hll_gray_obj(vm, obj->as.cons.car);
    hll_gray_obj(vm, obj->as.cons.cdr);
    break;
  case HLL_OBJ_SYMB:
    vm->bytes_allocated +=
        hll_unwrap_symb(obj)->length + 1 + sizeof(struct hll_obj_symb);
    break;
  case HLL_OBJ_TRUE:
    vm->bytes_allocated -= sizeof(struct hll_obj);
    break;
  case HLL_OBJ_NIL:
    vm->bytes_allocated -= sizeof(struct hll_obj);
    break;
  case HLL_OBJ_NUM:
    break;
  case HLL_OBJ_BIND:
    vm->bytes_allocated += sizeof(struct hll_obj_bind);
    break;
  case HLL_OBJ_ENV:
    vm->bytes_allocated += sizeof(struct hll_obj_env);
    hll_gray_obj(vm, hll_unwrap_env(obj)->vars);
    hll_gray_obj(vm, hll_unwrap_env(obj)->up);
    break;
  case HLL_OBJ_FUNC: {
    vm->bytes_allocated += sizeof(struct hll_obj_func);
    hll_gray_obj(vm, hll_unwrap_func(obj)->param_names);
    hll_gray_obj(vm, hll_unwrap_func(obj)->env);
    hll_gray_obj(vm, hll_unwrap_func(obj)->var_list);
    struct hll_bytecode *bytecode = hll_unwrap_func(obj)->bytecode;
    for (size_t i = 0; i < hll_sb_len(bytecode->constant_pool); ++i) {
      hll_gray_obj(vm, bytecode->constant_pool[i]);
    }
  } break;
  case HLL_OBJ_MACRO: {
    vm->bytes_allocated += sizeof(struct hll_obj_func);
    hll_gray_obj(vm, hll_unwrap_macro(obj)->param_names);
    hll_gray_obj(vm, hll_unwrap_macro(obj)->env);
    hll_gray_obj(vm, hll_unwrap_macro(obj)->var_list);
    struct hll_bytecode *bytecode = hll_unwrap_macro(obj)->bytecode;
    for (size_t i = 0; i < hll_sb_len(bytecode->constant_pool); ++i) {
      hll_gray_obj(vm, bytecode->constant_pool[i]);
    }
  } break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}
