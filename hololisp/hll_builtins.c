#include "hll_obj.h"
#include "hll_vm.h"

#include <math.h>
#include <stdio.h>

static hll_obj *builtin_print(hll_vm *vm, hll_obj *args) {
  hll_print(vm, hll_unwrap_car(args), stdout);
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
  for (hll_obj *obj = args; obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
    result *= value->as.num;
  }
  return hll_new_num(vm, result);
}

static hll_obj *builtin_num_ne(hll_vm *vm, hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  for (hll_obj *obj1 = args; obj1->kind == HLL_OBJ_CONS;
       obj1 = hll_unwrap_cdr(obj1)) {
    hll_obj *num1 = hll_unwrap_car(obj1);
    //    CHECK_TYPE(num1, HLL_OBJ_INT, "arguments");
    for (hll_obj *obj2 = hll_unwrap_cdr(obj1); obj2->kind == HLL_OBJ_CONS;
         obj2 = hll_unwrap_cdr(obj2)) {
      if (obj1 == obj2) {
        continue;
      }

      hll_obj *num2 = hll_unwrap_car(obj2);
      //      CHECK_TYPE(num2, HLL_OBJ_INT, "arguments");

      if (num1->as.num == num2->as.num) {
        return vm->nil;
      }
    }
  }

  return vm->true_;
}

static hll_obj *builtin_num_eq(hll_vm *vm, hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  for (hll_obj *obj1 = args; obj1->kind == HLL_OBJ_CONS;
       obj1 = hll_unwrap_cdr(obj1)) {
    hll_obj *num1 = hll_unwrap_car(obj1);
    //    CHECK_TYPE(num1, HLL_OBJ_INT, "arguments");

    for (hll_obj *obj2 = hll_unwrap_cdr(obj1); obj2->kind == HLL_OBJ_CONS;
         obj2 = hll_unwrap_cdr(obj2)) {
      if (obj1 == obj2) {
        continue;
      }

      hll_obj *num2 = hll_unwrap_car(obj2);
      //      CHECK_TYPE(num2, HLL_OBJ_INT, "arguments");

      if (num1->as.num != num2->as.num) {
        return vm->nil;
      }
    }
  }

  return vm->true_;
}

static hll_obj *builtin_num_gt(hll_vm *vm, hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  hll_obj *prev = hll_unwrap_car(args);
  //  CHECK_TYPE(prev, HLL_OBJ_INT, "arguments");

  for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *num = hll_unwrap_car(obj);
    //    CHECK_TYPE(num, HLL_OBJ_INT, "arguments");

    if (prev->as.num <= num->as.num) {
      return vm->nil;
    }

    prev = num;
  }

  return vm->true_;
}

static hll_obj *builtin_num_ge(hll_vm *vm, hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  hll_obj *prev = hll_unwrap_car(args);
  //  CHECK_TYPE(prev, HLL_OBJ_INT, "arguments");

  for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *num = hll_unwrap_car(obj);
    //    CHECK_TYPE(num, HLL_OBJ_INT, "arguments");

    if (prev->as.num < num->as.num) {
      return vm->nil;
    }
    prev = num;
  }

  return vm->true_;
}

static hll_obj *builtin_num_lt(hll_vm *vm, hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  hll_obj *prev = hll_unwrap_car(args);
  //  CHECK_TYPE(prev, HLL_OBJ_INT, "arguments");

  for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *num = hll_unwrap_car(obj);
    //    CHECK_TYPE(num, HLL_OBJ_INT, "arguments");

    if (prev->as.num >= num->as.num) {
      return vm->nil;
    }
    prev = num;
  }

  return vm->true_;
}

static hll_obj *builtin_num_le(hll_vm *vm, hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  hll_obj *prev = hll_unwrap_car(args);
  //  CHECK_TYPE(prev, HLL_OBJ_INT, "arguments");

  for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind != HLL_OBJ_NIL;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *num = hll_unwrap_car(obj);
    //    CHECK_TYPE(num, HLL_OBJ_INT, "arguments");

    if (prev->as.num > num->as.num) {
      return vm->nil;
    }
    prev = num;
  }

  return vm->true_;
}

static hll_obj *builtin_rem(hll_vm *vm, hll_obj *args) {
  //  CHECK_HAS_N_ARGS(2);

  hll_obj *x = hll_unwrap_car(args);
  //  CHECK_TYPE(x, HLL_OBJ_INT, "dividend");
  hll_obj *y = hll_unwrap_car(hll_unwrap_cdr(args));
  //  CHECK_TYPE(y, HLL_OBJ_INT, "divisor");

  return hll_new_num(vm, fmod(x->as.num, y->as.num));
}

static hll_obj *builtin_and(hll_vm *vm, hll_obj *args) {
  for (hll_obj *obj = args; obj->kind != HLL_OBJ_NIL;
       obj = hll_unwrap_cdr(obj)) {
    if (hll_unwrap_car(obj)->kind == HLL_OBJ_NIL) {
      return vm->nil;
    }
  }

  return vm->true_;
}

void add_builtins(hll_vm *vm) {
  hll_add_binding(vm, "print", builtin_print);
  hll_add_binding(vm, "+", builtin_add);
  hll_add_binding(vm, "-", builtin_sub);
  hll_add_binding(vm, "*", builtin_mul);
  hll_add_binding(vm, "/", builtin_div);
  hll_add_binding(vm, "<", builtin_num_lt);
  hll_add_binding(vm, "<=", builtin_num_le);
  hll_add_binding(vm, ">", builtin_num_gt);
  hll_add_binding(vm, ">=", builtin_num_ge);
  hll_add_binding(vm, "=", builtin_num_eq);
  hll_add_binding(vm, "/=", builtin_num_ne);
  hll_add_binding(vm, "rem", builtin_rem);
  hll_add_binding(vm, "and", builtin_and);
}
