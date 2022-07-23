#include "hll_vm.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_compiler.h"
#include "hll_hololisp.h"
#include "hll_obj.h"

static void default_error_fn(hll_vm *vm, uint32_t line, uint32_t column,
                             char const *message) {
  (void)vm;
  fprintf(stderr, "[ERROR]: %" PRIu32 ":%" PRIu32 ": %s", line + 1, column + 1,
          message);
}

static void default_write_fn(hll_vm *vm, char const *text) {
  (void)vm;
  printf("%s", text);
}

static void initialize_default_config(hll_config *config) {
  config->write_fn = default_write_fn;
  config->error_fn = default_error_fn;

  config->heap_size = 10 << 20;
  config->min_heap_size = 1 << 20;
  config->heap_grow_percent = 50;

  config->user_data = NULL;
}

static void add_builtins(hll_vm *vm);

hll_vm *hll_make_vm(hll_config const *config) {
  hll_vm *vm = calloc(1, sizeof(hll_vm));

  if (config == NULL) {
    initialize_default_config(&vm->config);
  } else {
    vm->config = *config;
  }

  vm->nil = hll_new_nil(vm);
  vm->true_ = hll_new_true(vm);
  vm->global_env = vm->env = hll_new_env(vm, vm->nil, vm->nil);

  add_builtins(vm);

  return vm;
}

void hll_delete_vm(hll_vm *vm) { free(vm); }

hll_interpret_result hll_interpret(hll_vm *vm, const char *source) {
  hll_bytecode *bytecode = hll_compile(vm, source);
  if (bytecode == NULL) {
    return HLL_RESULT_COMPILE_ERROR;
  }

  return hll_interpret_bytecode(vm, bytecode, true) ? HLL_RESULT_RUNTIME_ERROR
                                                    : HLL_RESULT_OK;
}

void hll_add_binding(hll_vm *vm, const char *symb_str,
                     hll_obj *(*bind_func)(hll_vm *vm, hll_obj *args)) {
  hll_obj *bind = hll_new_bind(vm, bind_func);
  hll_obj *symb = hll_new_symbol(vm, symb_str, strlen(symb_str));

  hll_unwrap_env(vm->env)->vars = hll_new_cons(vm, hll_new_cons(vm, bind, symb),
                                               hll_unwrap_env(vm->env)->vars);
}

static hll_obj *hll_find_var(hll_vm *vm, hll_obj *car) {
  hll_obj *result = NULL;

  for (hll_obj *env = vm->env; env->kind != HLL_OBJ_NIL && result == NULL;
       env = hll_unwrap_env(env)->up) {
    for (hll_obj *cons = hll_unwrap_env(env)->vars;
         cons->kind != HLL_OBJ_NIL && result == NULL;
         cons = hll_unwrap_cdr(cons)) {
      hll_obj *test = hll_unwrap_car(cons);
      if (strcmp(hll_unwrap_zsymb(hll_unwrap_car(test)),
                 hll_unwrap_zsymb(car)) == 0) {
        result = test;
      }
    }
  }

  return result;
}

static void print_internal(hll_vm *vm, hll_obj *obj) {
  FILE *file = stdout;

  switch (obj->kind) {
  case HLL_OBJ_CONS:
    fprintf(file, "(");
    while (obj->kind != HLL_OBJ_NIL) {
      assert(obj->kind == HLL_OBJ_CONS);
      print_internal(vm, hll_unwrap_car(obj));

      hll_obj *cdr = hll_unwrap_cdr(obj);
      if (cdr->kind != HLL_OBJ_NIL && cdr->kind != HLL_OBJ_CONS) {
        fprintf(file, " . ");
        print_internal(vm, cdr);
        break;
      } else if (cdr->kind != HLL_OBJ_NIL) {
        fprintf(file, " ");
      }

      obj = cdr;
    }
    fprintf(file, ")");
    break;
  case HLL_OBJ_SYMB:
    fprintf(file, "%s", hll_unwrap_zsymb(obj));
    break;
  case HLL_OBJ_NIL:
    fprintf(file, "()");
    break;
  case HLL_OBJ_NUM:
    fprintf(file, "%lld", (long long)obj->as.num);
    break;
  case HLL_OBJ_TRUE:
    fprintf(file, "t");
    break;
  default:
    assert(!"Not implemented");
    break;
  }
}

static hll_obj *builtin_print(hll_vm *vm, hll_obj *args) {
  print_internal(vm, hll_unwrap_car(args));
  printf("\n");
  return vm->nil;
}

static hll_obj *builtin_add(hll_vm *vm, hll_obj *args) {
  hll_num result = 0;
  for (hll_obj *obj = args; obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
    result += value->as.num;
  }
  return hll_new_num(vm, result);
}

static hll_obj *builtin_sub(hll_vm *vm, hll_obj *args) {
  // CHECK_HAS_ATLEAST_N_ARGS(1);
  hll_obj *first = hll_unwrap_car(args);
  // CHECK_TYPE(first, HLL_OBJ_INT, "arguments");
  hll_num result = first->as.num;
  for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
    result -= value->as.num;
  }
  return hll_new_num(vm, result);
}

static hll_obj *builtin_div(hll_vm *vm, hll_obj *args) {
  // CHECK_HAS_ATLEAST_N_ARGS(1);
  hll_obj *first = hll_unwrap_car(args);
  // CHECK_TYPE(first, HLL_OBJ_INT, "arguments");
  hll_num result = first->as.num;
  for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
    result /= value->as.num;
  }
  return hll_new_num(vm, result);
}

static hll_obj *builtin_mul(hll_vm *vm, hll_obj *args) {
  hll_num result = 1;
  for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
    result *= value->as.num;
  }
  return hll_new_num(vm, result);
}

static void add_builtins(hll_vm *vm) {
  hll_add_binding(vm, "print", builtin_print);
  hll_add_binding(vm, "+", builtin_add);
  hll_add_binding(vm, "-", builtin_sub);
  hll_add_binding(vm, "*", builtin_mul);
  hll_add_binding(vm, "/", builtin_div);
}

bool hll_interpret_bytecode(hll_vm *vm, hll_bytecode *bytecode,
                            bool print_result) {
  (void)vm;
  uint8_t *ip = bytecode->ops;
  hll_obj **stack = NULL;

  uint8_t op;
  while ((op = *ip++) != HLL_BYTECODE_END) {
    switch (op) {
    case HLL_BYTECODE_POP:
      hll_sb_pop(stack);
      break;
    case HLL_BYTECODE_NIL:
      hll_sb_push(stack, vm->nil);
      break;
    case HLL_BYTECODE_TRUE:
      hll_sb_push(stack, vm->true_);
      break;
    case HLL_BYTECODE_CONST: {
      uint16_t idx = (ip[0] << 8) | ip[1];
      ip += 2;
      assert(idx < hll_sb_len(bytecode->constant_pool));
      hll_obj *value = bytecode->constant_pool[idx];
      hll_sb_push(stack, value);
    } break;
    case HLL_BYTECODE_APPEND: {
      assert(hll_sb_len(stack) >= 3);
      hll_obj **headp = &hll_sb_last(stack) + -2;
      hll_obj **tailp = &hll_sb_last(stack) + -1;
      hll_obj *obj = hll_sb_pop(stack);

      hll_obj *cons = hll_new_cons(vm, obj, vm->nil);
      if ((*headp)->kind == HLL_OBJ_NIL) {
        *headp = *tailp = cons;
      } else {
        assert((*tailp)->kind == HLL_OBJ_CONS);
        hll_unwrap_cons(*tailp)->cdr = cons;
        *tailp = cons;
      }
    } break;
    case HLL_BYTECODE_FIND: {
      hll_obj *symb = hll_sb_pop(stack);
      assert(symb->kind == HLL_OBJ_SYMB); // TODO throw

      hll_obj *found = hll_find_var(vm, symb);
      assert(found != NULL); // TODO throw
      hll_sb_push(stack, found);
    } break;
    case HLL_BYTECODE_CALL: {
      hll_obj *args = hll_sb_pop(stack);
      hll_obj *callable = hll_sb_pop(stack);
      switch (callable->kind) {
      case HLL_OBJ_FUNC:
        assert(!"TODO");
        break;
      case HLL_OBJ_BIND: {
        hll_obj *new_env = vm->global_env;
        hll_obj *cur_env = vm->env;

        vm->env = new_env;
        hll_obj *result = hll_unwrap_bind(callable)->bind(vm, args);
        vm->env = cur_env;

        hll_sb_push(stack, result);
      } break;
      default:
        assert(0);
        break; // TODO throw
      }
    } break;
    case HLL_BYTECODE_JN: {
      int16_t offset = (ip[0] << 8) | ip[1];
      ip += 2;

      hll_obj *cond = hll_sb_pop(stack);
      if (cond->kind == HLL_OBJ_NIL) {
        ip += offset;
        assert(ip <= &hll_sb_last(bytecode->ops));
      }
    } break;
    case HLL_BYTECODE_MAKE_LAMBDA:
      assert(!"Not implemented");
      break;
    case HLL_BYTECODE_LET: {
      hll_obj *value = hll_sb_pop(stack);
      hll_obj *name = hll_sb_pop(stack);
      hll_unwrap_env(vm->env)->vars = hll_new_cons(
          vm, hll_new_cons(vm, name, value), hll_unwrap_env(vm->env)->vars);
    } break;
    case HLL_BYTECODE_PUSHENV:
      break;
    case HLL_BYTECODE_POPENV:
      break;
    case HLL_BYTECODE_CAR: {
      hll_obj *cons = hll_sb_pop(stack);
      assert(cons->kind == HLL_OBJ_CONS); // TODO throw
      hll_obj *car = hll_unwrap_car(cons);
      hll_sb_push(stack, car);
    } break;
    case HLL_BYTECODE_CDR: {
      hll_obj *cons = hll_sb_pop(stack);
      assert(cons->kind == HLL_OBJ_CONS); // TODO throw
      hll_obj *car = hll_unwrap_cdr(cons);
      hll_sb_push(stack, car);
    } break;
    case HLL_BYTECODE_SETCAR: {
      hll_obj *cons = hll_sb_pop(stack);
      hll_obj *car = hll_sb_pop(stack);
      assert(cons->kind == HLL_OBJ_CONS); // TODO throw
      hll_unwrap_cons(cons)->car = car;
    } break;
    case HLL_BYTECODE_SETCDR: {
      hll_obj *cons = hll_sb_pop(stack);
      hll_obj *cdr = hll_sb_pop(stack);
      assert(cons->kind == HLL_OBJ_CONS); // TODO throw
      hll_unwrap_cons(cons)->cdr = cdr;
    } break;
    default:
      assert(!"Unknown instruction");
      break;
    }
  }

  if (print_result) {
    printf("len: %zu\n", hll_sb_len(stack));
    assert(hll_sb_len(stack) == 1);
    hll_obj *obj = stack[0];
    print_internal(vm, obj);
  }

  hll_sb_free(stack);

  return HLL_RESULT_OK;
}
