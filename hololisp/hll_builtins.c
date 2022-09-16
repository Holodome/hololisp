#include "hll_compiler.h"
#include "hll_obj.h"
#include "hll_util.h"
#include "hll_vm.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

// 2008 edition of the POSIX standard (IEEE Standard 1003.1-2008)
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE 1
// For some reason we can obtain realpath definition only with these macros
#define __USE_XOPEN_EXTENDED
#define __USE_MISC
#include <unistd.h>

static hll_value builtin_print(struct hll_vm *vm, hll_value args) {
  for (; hll_is_cons(args); args = hll_cdr(args)) {
    hll_print(vm, hll_car(args), stdout);
    if (hll_is_cons(hll_cdr(args))) {
      printf(" ");
    }
  }
  printf("\n");
  return hll_car(args);
}

static hll_value builtin_add(struct hll_vm *vm, hll_value args) {
  (void)vm;
  double result = 0;
  for (hll_value obj = args; hll_is_cons(obj); obj = hll_cdr(obj)) {
    hll_value value = hll_car(obj);
    // CHECK_TYPE(value, struct hll_obj_INT, "arguments");
    result += hll_unwrap_num(value);
  }
  return hll_num(result);
}

static hll_value builtin_sub(struct hll_vm *vm, hll_value args) {
  (void)vm;
  // CHECK_HAS_ATLEAST_N_ARGS(1);
  hll_value first = hll_car(args);
  if (hll_is_nil(hll_unwrap_cdr(args))) {
    return hll_num(-hll_unwrap_num(first));
  }
  // CHECK_TYPE(first, struct hll_obj_INT, "arguments");
  double result = hll_unwrap_num(first);
  for (hll_value obj = hll_cdr(args); hll_is_cons(obj); obj = hll_cdr(obj)) {
    hll_value value = hll_car(obj);
    // CHECK_TYPE(value, struct hll_obj_INT, "arguments");
    result -= hll_unwrap_num(value);
  }
  return hll_num(result);
}

static hll_value builtin_div(struct hll_vm *vm, hll_value args) {
  (void)vm;
  // CHECK_HAS_ATLEAST_N_ARGS(1);
  hll_value first = hll_car(args);
  // CHECK_TYPE(first, struct hll_obj_INT, "arguments");
  double result = hll_unwrap_num(first);
  for (hll_value obj = hll_cdr(args); hll_is_cons(obj); obj = hll_cdr(obj)) {
    hll_value value = hll_car(obj);
    // CHECK_TYPE(value, struct hll_obj_INT, "arguments");
    result /= hll_unwrap_num(value);
  }
  return hll_num(result);
}

static hll_value builtin_mul(struct hll_vm *vm, hll_value args) {
  (void)vm;
  double result = 1;
  for (hll_value obj = args; hll_is_cons(obj); obj = hll_cdr(obj)) {
    hll_value value = hll_car(obj);
    // CHECK_TYPE(value, struct hll_obj_INT, "arguments");
    result *= hll_unwrap_num(value);
  }
  return hll_num(result);
}

static hll_value builtin_num_ne(struct hll_vm *vm, hll_value args) {
  (void)vm;
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  for (hll_value obj1 = args; hll_get_value_kind(obj1) == HLL_OBJ_CONS;
       obj1 = hll_cdr(obj1)) {
    hll_value num1 = hll_car(obj1);
    //    CHECK_TYPE(num1, struct hll_obj_INT, "arguments");
    for (hll_value obj2 = hll_cdr(obj1);
         hll_get_value_kind(obj2) == HLL_OBJ_CONS; obj2 = hll_cdr(obj2)) {
      if (obj1 == obj2) {
        continue;
      }

      hll_value num2 = hll_car(obj2);
      //      CHECK_TYPE(num2, struct hll_obj_INT, "arguments");

      if (fabs(hll_unwrap_num(num1) - hll_unwrap_num(num2)) < 1e-3) {
        return hll_nil();
      }
    }
  }

  return hll_true();
}

static hll_value builtin_num_eq(struct hll_vm *vm, hll_value args) {
  (void)vm;
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  for (hll_value obj1 = args; hll_get_value_kind(obj1) == HLL_OBJ_CONS;
       obj1 = hll_cdr(obj1)) {
    hll_value num1 = hll_car(obj1);
    //    CHECK_TYPE(num1, struct hll_obj_INT, "arguments");

    for (hll_value obj2 = hll_cdr(obj1);
         hll_get_value_kind(obj2) == HLL_OBJ_CONS; obj2 = hll_cdr(obj2)) {
      if (obj1 == obj2) {
        continue;
      }

      hll_value num2 = hll_car(obj2);
      //      CHECK_TYPE(num2, struct hll_obj_INT, "arguments");

      if (fabs(hll_unwrap_num(num1) - hll_unwrap_num(num2)) >= 1e-3) {
        return hll_nil();
      }
    }
  }

  return hll_true();
}

static hll_value builtin_num_gt(struct hll_vm *vm, hll_value args) {
  (void)vm;
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  hll_value prev = hll_car(args);
  //  CHECK_TYPE(prev, struct hll_obj_INT, "arguments");

  for (hll_value obj = hll_cdr(args); hll_is_cons(obj); obj = hll_cdr(obj)) {
    hll_value num = hll_car(obj);
    //    CHECK_TYPE(num, struct hll_obj_INT, "arguments");

    if (hll_unwrap_num(prev) <= hll_unwrap_num(num)) {
      return hll_nil();
    }

    prev = num;
  }

  return hll_true();
}

static hll_value builtin_num_ge(struct hll_vm *vm, hll_value args) {
  (void)vm;
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  hll_value prev = hll_car(args);
  //  CHECK_TYPE(prev, struct hll_obj_INT, "arguments");

  for (hll_value obj = hll_cdr(args); hll_get_value_kind(obj) != HLL_OBJ_NIL;
       obj = hll_cdr(obj)) {
    hll_value num = hll_car(obj);
    //    CHECK_TYPE(num, struct hll_obj_INT, "arguments");

    if (hll_unwrap_num(prev) < hll_unwrap_num(num)) {
      return hll_nil();
    }
    prev = num;
  }

  return hll_true();
}

static hll_value builtin_num_lt(struct hll_vm *vm, hll_value args) {
  (void)vm;
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  hll_value prev = hll_car(args);
  //  CHECK_TYPE(prev, struct hll_obj_INT, "arguments");

  for (hll_value obj = hll_cdr(args); hll_get_value_kind(obj) != HLL_OBJ_NIL;
       obj = hll_cdr(obj)) {
    hll_value num = hll_car(obj);
    //    CHECK_TYPE(num, struct hll_obj_INT, "arguments");

    if (hll_unwrap_num(prev) >= hll_unwrap_num(num)) {
      return hll_nil();
    }
    prev = num;
  }

  return hll_true();
}

static hll_value builtin_num_le(struct hll_vm *vm, hll_value args) {
  (void)vm;
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  hll_value prev = hll_car(args);
  //  CHECK_TYPE(prev, struct hll_obj_INT, "arguments");

  for (hll_value obj = hll_cdr(args); hll_get_value_kind(obj) != HLL_OBJ_NIL;
       obj = hll_cdr(obj)) {
    hll_value num = hll_car(obj);
    //    CHECK_TYPE(num, struct hll_obj_INT, "arguments");

    if (hll_unwrap_num(prev) > hll_unwrap_num(num)) {
      return hll_nil();
    }
    prev = num;
  }

  return hll_true();
}

static hll_value builtin_rem(struct hll_vm *vm, hll_value args) {
  (void)vm;
  //  CHECK_HAS_N_ARGS(2);

  hll_value x = hll_car(args);
  //  CHECK_TYPE(x, struct hll_obj_INT, "dividend");
  hll_value y = hll_car(hll_cdr(args));
  //  CHECK_TYPE(y, struct hll_obj_INT, "divisor");

  return hll_num(fmod(hll_unwrap_num(x), hll_unwrap_num(y)));
}

static uint64_t xorshift64(uint64_t *state) {
  uint64_t x = *state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return *state = x;
}

static hll_value builtin_random(struct hll_vm *vm, hll_value args) {
  hll_value low = hll_nil();
  hll_value high = hll_nil();
  if (hll_get_value_kind(args) == HLL_OBJ_CONS) {
    high = hll_car(args);
    if (!hll_is_num(high)) {
      hll_runtime_error(vm, "'random' form expects number arguments");
      return hll_nil();
    }

    args = hll_cdr(args);

    if (hll_get_value_kind(args) == HLL_OBJ_CONS) {
      low = hll_car(args);
      if (!hll_is_num(low)) {
        hll_runtime_error(vm, "'random' form expects number arguments");
        return hll_nil();
      }
      args = hll_cdr(args);

      if (hll_get_value_kind(args) != HLL_OBJ_NIL) {
        hll_runtime_error(vm, "'random' form expects at most 2 arguments");
        return hll_nil();
      }
    }
  }

  double result;
  if (hll_is_nil(high)) { // 0-1 float
    result = (double)xorshift64(&vm->rng_state) / (double)UINT64_MAX;
  } else if (hll_is_nil(low)) { // 0-high int
    uint64_t upper = floor(hll_unwrap_num(high));
    uint64_t random = xorshift64(&vm->rng_state);
    result = random % upper;
  } else {
    uint64_t lower = floor(hll_unwrap_num(high));
    uint64_t upper = floor(hll_unwrap_num(low));
    assert(upper > lower);
    uint64_t random = xorshift64(&vm->rng_state);
    result = random % (upper - lower) + lower;
  }

  return hll_num(result);
}

static hll_value builtin_range(struct hll_vm *vm, hll_value args) {
  hll_value low = hll_nil();
  hll_value high = hll_nil();
  if (hll_get_value_kind(args) == HLL_OBJ_CONS) {
    high = hll_car(args);
    if (!hll_is_num(high)) {
      hll_runtime_error(vm, "'range' form expects number arguments");
      return hll_nil();
    }

    args = hll_cdr(args);

    if (hll_get_value_kind(args) == HLL_OBJ_CONS) {
      low = hll_car(args);
      if (!hll_is_num(low)) {
        hll_runtime_error(vm, "'range' form expects number arguments");
        return hll_nil();
      }
      args = hll_cdr(args);

      if (hll_get_value_kind(args) != HLL_OBJ_NIL) {
        hll_runtime_error(vm, "'range' form expects at most 2 arguments");
        return hll_nil();
      }
    }
  } else {
    hll_runtime_error(vm, "'range' form expects at least single argument");
    return hll_nil();
  }

  hll_value list_head = hll_nil();
  hll_value list_tail = hll_nil();
  if (hll_is_nil(low)) { // 0-high int
    uint64_t upper = floor(hll_unwrap_num(high));
    for (uint64_t i = 0; i < upper; ++i) {
      hll_value n = hll_num(i);
      hll_value cons = hll_new_cons(vm, n, hll_nil());
      if (hll_is_nil(list_head)) {
        list_head = list_tail = cons;
      } else {
        hll_unwrap_cons(list_tail)->cdr = cons;
        list_tail = cons;
      }
    }
  } else {
    uint64_t lower = floor(hll_unwrap_num(high));
    uint64_t upper = floor(hll_unwrap_num(low));
    for (uint64_t i = lower; i < upper; ++i) {
      hll_value n = hll_num(i);
      hll_value cons = hll_new_cons(vm, n, hll_nil());
      if (hll_is_nil(list_head)) {
        list_head = list_tail = cons;
      } else {
        hll_unwrap_cons(list_tail)->cdr = cons;
        list_tail = cons;
      }
    }
  }

  return list_head;
}

static hll_value builtin_min(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_get_value_kind(args) != HLL_OBJ_CONS)) {
    hll_runtime_error(vm, "min' form expects at least single argument");
    return hll_nil();
  }

  hll_value result = hll_nil();
  for (hll_value obj = args; hll_is_cons(obj); obj = hll_cdr(obj)) {
    hll_value test = hll_car(obj);
    if (HLL_UNLIKELY(hll_get_value_kind(test) != HLL_OBJ_NUM)) {
      hll_runtime_error(vm, "'min' form expects number arguments");
      return hll_nil();
    }

    if (hll_get_value_kind(result) == HLL_OBJ_NIL ||
        hll_unwrap_num(result) > hll_unwrap_num(test)) {
      result = test;
    }
  }

  return result;
}

static hll_value builtin_max(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_get_value_kind(args) != HLL_OBJ_CONS)) {
    hll_runtime_error(vm, "max' form expects at least single argument");
    return hll_nil();
  }

  hll_value result = hll_nil();
  for (hll_value obj = args; hll_is_cons(obj); obj = hll_cdr(obj)) {
    hll_value test = hll_car(obj);
    if (HLL_UNLIKELY(hll_get_value_kind(test) != HLL_OBJ_NUM)) {
      hll_runtime_error(vm, "'max' form expects number arguments");
      return hll_nil();
    }

    if (hll_get_value_kind(result) == HLL_OBJ_NIL ||
        hll_unwrap_num(result) < hll_unwrap_num(test)) {
      result = test;
    }
  }

  return result;
}

static hll_value builtin_listp(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'list?' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  hll_value result = hll_nil();
  if (hll_is_cons(obj) || hll_get_value_kind(obj) == HLL_OBJ_NIL) {
    result = hll_true();
  }

  return result;
}

static hll_value builtin_null(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'null' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  hll_value result = hll_nil();

  if (hll_get_value_kind(obj) == HLL_OBJ_NIL) {
    result = hll_true();
  }

  return result;
}

static hll_value builtin_minusp(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'negative?' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  if (HLL_UNLIKELY(hll_get_value_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'negative?' expects number argument");
    return hll_nil();
  }

  hll_value result = hll_nil();
  if (hll_unwrap_num(obj) < 0) {
    result = hll_true();
  }

  return result;
}

static hll_value builtin_zerop(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'zero?' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  if (HLL_UNLIKELY(hll_get_value_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'zero?' expects number argument");
    return hll_nil();
  }

  hll_value result = hll_nil();
  if (hll_unwrap_num(obj) == 0) {
    result = hll_true();
  }

  return result;
}

static hll_value builtin_even(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'even?' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  if (HLL_UNLIKELY(hll_get_value_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'even?' expects number argument");
    return hll_nil();
  }

  hll_value result = hll_nil();
  if ((long long)hll_unwrap_num(obj) % 2 == 0) {
    result = hll_true();
  }

  return result;
}
static hll_value builtin_odd(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'odd?' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  if (HLL_UNLIKELY(hll_get_value_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'odd?' expects number argument");
    return hll_nil();
  }

  hll_value result = hll_nil();
  if ((long long)hll_unwrap_num(obj) % 2 != 0) {
    result = hll_true();
  }

  return result;
}

static hll_value builtin_plusp(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'negative?' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  if (HLL_UNLIKELY(hll_get_value_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'negative?' expects number argument");
    return hll_nil();
  }

  hll_value result = hll_nil();
  if (hll_unwrap_num(obj) > 0) {
    result = hll_true();
  }

  return result;
}

static hll_value builtin_numberp(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'null' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  hll_value result = hll_nil();
  if (hll_get_value_kind(obj) == HLL_OBJ_NUM) {
    result = hll_true();
  }

  return result;
}

static hll_value builtin_abs(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'abs' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  if (HLL_UNLIKELY(hll_get_value_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'abs' expects number argument");
    return hll_nil();
  }

  return hll_num(fabs(hll_unwrap_num(obj)));
}

static hll_value builtin_append(struct hll_vm *vm, hll_value args) {
  if (hll_get_value_kind(args) == HLL_OBJ_NIL) {
    return args;
  }

  (void)vm;
  hll_value list = hll_car(args);
  while (hll_get_value_kind(list) != HLL_OBJ_CONS &&
         hll_get_value_kind(args) == HLL_OBJ_CONS) {
    args = hll_cdr(args);
    list = hll_car(args);
  }

  if (hll_get_value_kind(list) == HLL_OBJ_NIL) {
    return list;
  }

  hll_value tail = list;
  for (hll_value slot = hll_cdr(args); hll_get_value_kind(slot) == HLL_OBJ_CONS;
       slot = hll_cdr(slot)) {
    for (; hll_get_value_kind(tail) == HLL_OBJ_CONS &&
           hll_get_value_kind(hll_cdr(tail)) != HLL_OBJ_NIL;
         tail = hll_cdr(tail)) {
    }

    if (hll_get_value_kind(tail) == HLL_OBJ_CONS) {
      hll_unwrap_cons(tail)->cdr = hll_car(slot);
    } else {
      tail = slot;
      list = slot;
    }
  }

  return list;
}

static hll_value builtin_reverse(struct hll_vm *vm, hll_value args) {
  // NOTE: This seems like it can be compiled to bytecode directly
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'reverse' expects exactly 1 argument");
    return hll_nil();
  }

  hll_value obj = hll_car(args);
  hll_value result = hll_nil();

  while (hll_get_value_kind(obj) != HLL_OBJ_NIL) {
    assert(hll_is_cons(obj));
    hll_value head = obj;
    obj = hll_cdr(obj);
    hll_unwrap_cons(head)->cdr = result;
    result = head;
  }

  return result;
}

static hll_value builtin_nthcdr(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 2)) {
    hll_runtime_error(vm, "'nthcdr' expects exactly 2 arguments");
    return hll_nil();
  }

  hll_value num = hll_car(args);
  if (HLL_UNLIKELY(hll_get_value_kind(num) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'nthcdr' first argument must be a number");
    return hll_nil();
  } else if (HLL_UNLIKELY(hll_unwrap_num(num) < 0)) {
    hll_runtime_error(vm, "'nthcdr' expects non-negative number (got %F)",
                      hll_unwrap_num(num));
    return hll_nil();
  }

  size_t n = (size_t)floor(hll_unwrap_num(num));
  hll_value list = hll_car(hll_cdr(args));
  for (; hll_get_value_kind(list) == HLL_OBJ_CONS && n != 0;
       --n, list = hll_cdr(list))
    ;

  hll_value result = hll_nil();
  if (n == 0) {
    result = list;
  }

  return result;
}

static hll_value builtin_nth(struct hll_vm *vm, hll_value args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 2)) {
    hll_runtime_error(vm, "'nth' expects exactly 2 arguments");
    return hll_nil();
  }

  hll_value num = hll_car(args);
  if (HLL_UNLIKELY(hll_get_value_kind(num) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'nth' first argument must be a number");
    return hll_nil();
  } else if (HLL_UNLIKELY(hll_unwrap_num(num) < 0)) {
    hll_runtime_error(vm, "'nth' expects non-negative number (got %F)",
                      hll_unwrap_num(num));
    return hll_nil();
  }

  size_t n = (size_t)floor(hll_unwrap_num(num));
  hll_value list = hll_car(hll_cdr(args));
  for (; hll_get_value_kind(list) == HLL_OBJ_CONS && n != 0;
       --n, list = hll_cdr(list))
    ;

  hll_value result = hll_nil();
  if (n == 0 && hll_get_value_kind(list) == HLL_OBJ_CONS) {
    result = hll_car(list);
  }

  return result;
}

static hll_value builtin_clear(struct hll_vm *vm, hll_value args) {
  (void)vm;
  (void)args;
  printf("\033[2J");
  return hll_nil();
}

static hll_value builtin_sleep(struct hll_vm *vm, hll_value args) {
  (void)vm;
  double num = hll_unwrap_num(hll_car(args));
  usleep(num * 1000);
  return hll_nil();
}

static hll_value builtin_length(struct hll_vm *vm, hll_value args) {
  (void)vm;
  return hll_num(hll_list_length(hll_car(args)));
}

static hll_value builtin_eq(struct hll_vm *vm, hll_value args) {
  (void)vm;
  for (hll_value obj1 = args; hll_get_value_kind(obj1) == HLL_OBJ_CONS;
       obj1 = hll_cdr(obj1)) {
    hll_value it1 = hll_car(obj1);
    for (hll_value obj2 = hll_cdr(obj1);
         hll_get_value_kind(obj2) == HLL_OBJ_CONS; obj2 = hll_cdr(obj2)) {
      if (obj1 == obj2) {
        continue;
      }

      hll_value it2 = hll_car(obj2);
      if (it1 == it2) {
        return hll_nil();
      }
    }
  }

  return hll_true();
}

static hll_value builtin_eval(struct hll_vm *vm, hll_value args) {
  (void)args;
  char buffer[4096];
  if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
    hll_runtime_error(vm, "Failed to input string");
    return hll_nil();
  }

  hll_value compiled;
  if (!hll_compile(vm, buffer, &compiled)) {
    hll_runtime_error(vm, "failed to compile eval");
    return hll_nil();
  }

  return hll_interpret_bytecode_internal(vm, vm->env, compiled);
}

void add_builtins(struct hll_vm *vm) {
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
  hll_add_binding(vm, "even?", builtin_even);
  hll_add_binding(vm, "odd?", builtin_odd);
  hll_add_binding(vm, "number?", builtin_numberp);
  hll_add_binding(vm, "abs", builtin_abs);
  hll_add_binding(vm, "reverse!", builtin_reverse);
  hll_add_binding(vm, "append", builtin_append);
  hll_add_binding(vm, "nthcdr", builtin_nthcdr);
  hll_add_binding(vm, "nth", builtin_nth);
  hll_add_binding(vm, "clear", builtin_clear);
  hll_add_binding(vm, "sleep", builtin_sleep);
  hll_add_binding(vm, "length", builtin_length);
  hll_add_binding(vm, "range", builtin_range);
  hll_add_binding(vm, "eval", builtin_eval);

  hll_add_binding(vm, "eq?", builtin_eq);
#if 1
  hll_interpret(vm, "builtins",
                "(defmacro (not x) (list 'if x () 't))\n"
                "(defmacro (when expr . body)\n"
                "  (cons 'if (cons expr (list (cons 'progn body)))))\n"
                "(defmacro (unless expr . body)\n"
                "  (cons 'if (cons expr (cons () body))))\n"
                "(defmacro (not x) (list 'if x () 't))\n"
                "(defmacro (when expr . body)\n"
                "  (cons 'if (cons expr (list (cons 'progn body)))))\n"
                "(defmacro (unless expr . body)\n"
                "  (cons 'if (cons expr (cons () body))))\n"
                "(define (tail lis)\n"
                "  (if (cdr lis)\n"
                "      (tail (cdr lis))\n"
                "      (car lis)))\n"
                "(define (count pred lis)\n"
                "  (if lis\n"
                "      (+ (if (pred (car lis)) 1 0) (count pred (cdr lis)))\n"
                "      0))\n"
                "(define (heads lis)\n"
                "  (when lis\n"
                "     (cons (caar lis) (heads (cdr lis)))))\n"
                "(define (tails lis)\n"
                "  (when lis\n"
                "    (cons (tail (car lis)) (tails (cdr lis)))))\n"
                "(define (map fn . lis)\n"
                "  (when lis\n"
                "    (cons (fn (heads lis))\n"
                "          (map fn (cdr lis)))))\n"
                "(define (any pred lis)\n"
                "  (when lis\n"
                "    (or (pred (car lis))\n"
                "        (any pred (cdr lis)))))\n"
                "(define (filter pred lis)\n"
                "  (when lis\n"
                "    (let ((value (pred (car lis))))\n"
                "      (if value\n"
                "          (cons (car lis) (filter pred (cdr lis)))\n"
                "          (filter pred (cdr lis))))))\n"
                "(define (all pred lis)\n"
                "  (if lis\n"
                "      (and (pred (car lis)) (all pred (cdr lis)))\n"
                "      t))\n",
                false);
  hll_interpret(vm, "builtins part 2",

                "(define (any pred lis)\n"
                "  (when lis\n"
                "    (or (pred (car lis))\n"
                "        (any pred (cdr lis)))))\n"
                "(define (filter pred lis)\n"
                "  (when lis\n"
                "    (let ((value (pred (car lis))))\n"
                "      (if value\n"
                "          (cons (car lis) (filter pred (cdr lis)))\n"
                "          (filter pred (cdr lis))))))\n"
                "(define (all pred lis)\n"
                "  (if lis\n"
                "      (and (pred (car lis)) (all pred (cdr lis)))\n"
                "      t))\n"
                "(define (reduce form lis)\n"
                "  (if (cdr lis)\n"
                "      (form (car lis) (reduce form (cdr lis)))\n"
                "      (car lis)))\n"
                "(define (repeat n x)\n"
                "  (when (positive? n)\n"
                "    (cons x (repeat (- n 1) x))))\n"
                "(defmacro (amap fn-body lis)\n"
                "  (list 'map (list 'lambda (list 'it) fn-body) lis))\n"
                "(defmacro (afilter fn-body lis)\n"
                "  (list 'filter (list 'lambda (list 'it) fn-body) lis))\n"
                "(defmacro (acount fn-body lis)\n"
                "  (list 'count (list 'lambda (list 'it) fn-body) lis))\n"
                "(defmacro (inc! val)\n"
                "  (list 'set! val (list '+ val 1)))\n"
                "(defmacro (dec! val)\n"
                "  (list 'set! val (list '- val 1)))\n"
                "(define (mapcat func lis)\n"
                "  (define (cat lis)\n"
                "    (when lis\n"
                "      (append (car lis) (cat (cdr lis)))))\n"
                "  (cat (map func lis)))\n"
                "(define (map fn lis)\n"
                "  (when lis\n"
                "    (cons (fn (car lis))\n"
                "          (map fn (cdr lis)))))\n",
                false);
#endif
}
