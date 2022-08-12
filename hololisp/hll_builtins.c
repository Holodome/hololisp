#include "hll_obj.h"
#include "hll_util.h"
#include "hll_vm.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static hll_obj *builtin_print(hll_vm *vm, hll_obj *args) {
  hll_print(vm, hll_unwrap_car(args), stdout);
  printf("\n");
  return vm->nil;
}

static hll_obj *builtin_add(hll_vm *vm, hll_obj *args) {
  double result = 0;
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
  double result = first->as.num;
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
  double result = first->as.num;
  for (hll_obj *obj = hll_unwrap_cdr(args); obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, HLL_OBJ_INT, "arguments");
    result /= value->as.num;
  }
  return hll_new_num(vm, result);
}

static hll_obj *builtin_mul(hll_vm *vm, hll_obj *args) {
  double result = 1;
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

static uint64_t xorshift64(uint64_t *state) {
  uint64_t x = *state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return *state = x;
}

static hll_obj *builtin_random(hll_vm *vm, hll_obj *args) {
  hll_obj *low = NULL, *high = NULL;
  if (args->kind == HLL_OBJ_CONS) {
    high = hll_unwrap_car(args);
    args = hll_unwrap_cdr(args);

    if (args->kind == HLL_OBJ_CONS) {
      low = hll_unwrap_car(args);
      args = hll_unwrap_cdr(args);

      if (args->kind != HLL_OBJ_NIL) {
        hll_runtime_error(vm, "'random' form expects at most 2 arguments");
        return NULL;
      }
    }
  }

  if (HLL_UNLIKELY((high && high->kind != HLL_OBJ_NUM) ||
                   (low && low->kind != HLL_OBJ_NUM))) {
    hll_runtime_error(vm, "'random' form expects number arguments");
    return NULL;
  }

  double result;
  if (high == NULL) { // 0-1 float
    result = (double)xorshift64(&vm->rng_state) / (double)UINT64_MAX;
  } else if (low == NULL) { // 0-high int
    uint64_t upper = floor(high->as.num);
    uint64_t random = xorshift64(&vm->rng_state);
    result = random % upper;
  } else {
    assert(low != NULL && high != NULL);
    uint64_t lower = floor(low->as.num);
    uint64_t upper = floor(high->as.num);
    uint64_t random = xorshift64(&vm->rng_state);
    result = random % upper + lower;
  }

  return hll_new_num(vm, result);
}

static hll_obj *builtin_min(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(args->kind != HLL_OBJ_CONS)) {
    hll_runtime_error(vm, "min' form expects at least single argument");
    return NULL;
  }

  hll_obj *result = vm->nil;
  for (hll_obj *obj = args; obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *test = hll_unwrap_car(obj);
    if (HLL_UNLIKELY(test->kind != HLL_OBJ_NUM)) {
      hll_runtime_error(vm, "'min' form expects number arguments");
      return NULL;
    }

    if (result->kind == HLL_OBJ_NIL || result->as.num > test->as.num) {
      result = test;
    }
  }

  return result;
}

static hll_obj *builtin_max(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(args->kind != HLL_OBJ_CONS)) {
    hll_runtime_error(vm, "max' form expects at least single argument");
    return NULL;
  }

  hll_obj *result = vm->nil;
  for (hll_obj *obj = args; obj->kind == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    hll_obj *test = hll_unwrap_car(obj);
    if (HLL_UNLIKELY(test->kind != HLL_OBJ_NUM)) {
      hll_runtime_error(vm, "'max' form expects number arguments");
      return NULL;
    }

    if (result->kind == HLL_OBJ_NIL || result->as.num < test->as.num) {
      result = test;
    }
  }

  return result;
}

static hll_obj *builtin_listp(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'list?' expects exactly 1 argument");
    return NULL;
  }

  hll_obj *obj = hll_unwrap_car(args);
  hll_obj *result = vm->nil;
  if (obj->kind == HLL_OBJ_CONS || obj->kind == HLL_OBJ_NIL) {
    result = vm->true_;
  }

  return result;
}

static hll_obj *builtin_null(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'null' expects exactly 1 argument");
    return NULL;
  }

  hll_obj *obj = hll_unwrap_car(args);
  hll_obj *result = vm->nil;

  if (obj->kind == HLL_OBJ_NIL) {
    result = vm->true_;
  }

  return result;
}

static hll_obj *builtin_minusp(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'negative?' expects exactly 1 argument");
    return NULL;
  }

  hll_obj *obj = hll_unwrap_car(args);
  if (HLL_UNLIKELY(obj->kind != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'negative?' expects number argument");
    return NULL;
  }

  hll_obj *result = vm->nil;
  if (obj->as.num < 0) {
    result = vm->true_;
  }

  return result;
}

static hll_obj *builtin_zerop(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'negative?' expects exactly 1 argument");
    return NULL;
  }

  hll_obj *obj = hll_unwrap_car(args);
  if (HLL_UNLIKELY(obj->kind != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'negative?' expects number argument");
    return NULL;
  }

  hll_obj *result = vm->nil;
  if (obj->as.num == 0) {
    result = vm->true_;
  }

  return result;
}

static hll_obj *builtin_plusp(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'negative?' expects exactly 1 argument");
    return NULL;
  }

  hll_obj *obj = hll_unwrap_car(args);
  if (HLL_UNLIKELY(obj->kind != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'negative?' expects number argument");
    return NULL;
  }

  hll_obj *result = vm->nil;
  if (obj->as.num > 0) {
    result = vm->true_;
  }

  return result;
}

static hll_obj *builtin_numberp(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'null' expects exactly 1 argument");
    return NULL;
  }

  hll_obj *obj = hll_unwrap_car(args);
  hll_obj *result = vm->nil;
  if (obj->kind == HLL_OBJ_NUM) {
    result = vm->true_;
  }

  return result;
}

static hll_obj *builtin_abs(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'abs' expects exactly 1 argument");
    return NULL;
  }

  hll_obj *obj = hll_unwrap_car(args);
  if (HLL_UNLIKELY(obj->kind != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'abs' expects number argument");
    return NULL;
  }

  return hll_new_num(vm, fabs(obj->as.num));
}

static hll_obj *builtin_append(hll_vm *vm, hll_obj *args) {
  // NOTE: This seems like it can be compiled to bytecode directly
  if (HLL_UNLIKELY(hll_list_length(args) != 2)) {
    hll_runtime_error(vm, "'append' expects exactly 2 arguments");
    return NULL;
  }

  hll_obj *list1 = hll_unwrap_car(args);
  hll_obj *list2 = hll_unwrap_car(hll_unwrap_cdr(args));

  hll_obj *tail = list1;
  for (; hll_unwrap_cdr(tail)->kind != HLL_OBJ_NIL; tail = hll_unwrap_cdr(tail))
    ;

  hll_unwrap_cons(tail)->cdr = list2;
  return list1;
}

static hll_obj *builtin_reverse(hll_vm *vm, hll_obj *args) {
  // NOTE: This seems like it can be compiled to bytecode directly
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'reverse' expects exactly 1 argument");
    return NULL;
  }

  hll_obj *obj = hll_unwrap_car(args);
  hll_obj *result = vm->nil;

  while (obj->kind != HLL_OBJ_NIL) {
    assert(obj->kind == HLL_OBJ_CONS);
    hll_obj *head = obj;
    obj = hll_unwrap_cdr(obj);
    hll_unwrap_cons(head)->cdr = result;
    result = head;
  }

  return result;
}

static hll_obj *builtin_nthcdr(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 2)) {
    hll_runtime_error(vm, "'nthcdr' expects exactly 2 arguments");
    return NULL;
  }

  hll_obj *num = hll_unwrap_car(args);
  if (HLL_UNLIKELY(num->kind != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'nthcdr' first argument must be a number");
    return NULL;
  } else if (HLL_UNLIKELY(num->as.num < 0)) {
    hll_runtime_error(vm, "'nthcdr' expects non-negative number");
    return NULL;
  }

  size_t n = (size_t)floor(num->as.num);
  hll_obj *list = hll_unwrap_car(hll_unwrap_cdr(args));
  for (; list->kind == HLL_OBJ_CONS && n != 0; --n, list = hll_unwrap_cdr(list))
    ;

  hll_obj *result = vm->nil;
  if (n == 0) {
    result = list;
  }

  return result;
}

static hll_obj *builtin_nth(hll_vm *vm, hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 2)) {
    hll_runtime_error(vm, "'nthcdr' expects exactly 2 arguments");
    return NULL;
  }

  hll_obj *num = hll_unwrap_car(args);
  if (HLL_UNLIKELY(num->kind != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'nthcdr' first argument must be a number");
    return NULL;
  } else if (HLL_UNLIKELY(num->as.num < 0)) {
    hll_runtime_error(vm, "'nthcdr' expects non-negative number");
    return NULL;
  }

  size_t n = (size_t)floor(num->as.num);
  hll_obj *list = hll_unwrap_car(hll_unwrap_cdr(args));
  for (; list->kind == HLL_OBJ_CONS && n != 0; --n, list = hll_unwrap_cdr(list))
    ;

  hll_obj *result = vm->nil;
  if (n == 0 && list->kind == HLL_OBJ_CONS) {
    result = hll_unwrap_car(list);
  }

  return result;
}

static hll_obj *builtin_clear(hll_vm *vm, hll_obj *args) {
  (void)args;
  printf("\033[2J");
  return vm->nil;
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
  hll_add_binding(vm, "random", builtin_random);
  hll_add_binding(vm, "max", builtin_max);
  hll_add_binding(vm, "min", builtin_min);
  hll_add_binding(vm, "list?", builtin_listp);
  hll_add_binding(vm, "null?", builtin_null);
  hll_add_binding(vm, "negative?", builtin_minusp);
  hll_add_binding(vm, "positive?", builtin_plusp);
  hll_add_binding(vm, "zero?", builtin_zerop);
  hll_add_binding(vm, "number?", builtin_numberp);
  hll_add_binding(vm, "abs", builtin_abs);
  hll_add_binding(vm, "reverse", builtin_reverse);
  hll_add_binding(vm, "append", builtin_append);
  hll_add_binding(vm, "nthcdr", builtin_nthcdr);
  hll_add_binding(vm, "nth", builtin_nth);
  hll_add_binding(vm, "clear", builtin_clear);
}
