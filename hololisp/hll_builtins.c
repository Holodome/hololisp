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

static struct hll_obj *builtin_print(struct hll_vm *vm, struct hll_obj *args) {
  hll_print(vm, hll_unwrap_car(args), stdout);
  printf("\n");
  return hll_unwrap_car(args);
}

static struct hll_obj *builtin_add(struct hll_vm *vm, struct hll_obj *args) {
  double result = 0;
  for (struct hll_obj *obj = args; hll_get_obj_kind(obj) == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, struct hll_obj_INT, "arguments");
    result += hll_unwrap_num(value);
  }
  return hll_new_num(vm, result);
}

static struct hll_obj *builtin_sub(struct hll_vm *vm, struct hll_obj *args) {
  // CHECK_HAS_ATLEAST_N_ARGS(1);
  struct hll_obj *first = hll_unwrap_car(args);
  // CHECK_TYPE(first, struct hll_obj_INT, "arguments");
  double result = hll_unwrap_num(first);
  for (struct hll_obj *obj = hll_unwrap_cdr(args);
       hll_get_obj_kind(obj) == HLL_OBJ_CONS; obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, struct hll_obj_INT, "arguments");
    result -= hll_unwrap_num(value);
  }
  return hll_new_num(vm, result);
}

static struct hll_obj *builtin_div(struct hll_vm *vm, struct hll_obj *args) {
  // CHECK_HAS_ATLEAST_N_ARGS(1);
  struct hll_obj *first = hll_unwrap_car(args);
  // CHECK_TYPE(first, struct hll_obj_INT, "arguments");
  double result = hll_unwrap_num(first);
  for (struct hll_obj *obj = hll_unwrap_cdr(args);
       hll_get_obj_kind(obj) == HLL_OBJ_CONS; obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, struct hll_obj_INT, "arguments");
    result /= hll_unwrap_num(value);
  }
  return hll_new_num(vm, result);
}

static struct hll_obj *builtin_mul(struct hll_vm *vm, struct hll_obj *args) {
  double result = 1;
  for (struct hll_obj *obj = args; hll_get_obj_kind(obj) == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *value = hll_unwrap_car(obj);
    // CHECK_TYPE(value, struct hll_obj_INT, "arguments");
    result *= hll_unwrap_num(value);
  }
  return hll_new_num(vm, result);
}

static struct hll_obj *builtin_num_ne(struct hll_vm *vm, struct hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  for (struct hll_obj *obj1 = args; hll_get_obj_kind(obj1) == HLL_OBJ_CONS;
       obj1 = hll_unwrap_cdr(obj1)) {
    struct hll_obj *num1 = hll_unwrap_car(obj1);
    //    CHECK_TYPE(num1, struct hll_obj_INT, "arguments");
    for (struct hll_obj *obj2 = hll_unwrap_cdr(obj1);
         hll_get_obj_kind(obj2) == HLL_OBJ_CONS; obj2 = hll_unwrap_cdr(obj2)) {
      if (obj1 == obj2) {
        continue;
      }

      struct hll_obj *num2 = hll_unwrap_car(obj2);
      //      CHECK_TYPE(num2, struct hll_obj_INT, "arguments");

      if (fabs(hll_unwrap_num(num1) - hll_unwrap_num(num2)) < 1e-3) {
        return vm->nil;
      }
    }
  }

  return vm->true_;
}

static struct hll_obj *builtin_num_eq(struct hll_vm *vm, struct hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  for (struct hll_obj *obj1 = args; hll_get_obj_kind(obj1) == HLL_OBJ_CONS;
       obj1 = hll_unwrap_cdr(obj1)) {
    struct hll_obj *num1 = hll_unwrap_car(obj1);
    //    CHECK_TYPE(num1, struct hll_obj_INT, "arguments");

    for (struct hll_obj *obj2 = hll_unwrap_cdr(obj1);
         hll_get_obj_kind(obj2) == HLL_OBJ_CONS; obj2 = hll_unwrap_cdr(obj2)) {
      if (obj1 == obj2) {
        continue;
      }

      struct hll_obj *num2 = hll_unwrap_car(obj2);
      //      CHECK_TYPE(num2, struct hll_obj_INT, "arguments");

      if (fabs(hll_unwrap_num(num1) - hll_unwrap_num(num2)) >= 1e-3) {
        return vm->nil;
      }
    }
  }

  return vm->true_;
}

static struct hll_obj *builtin_num_gt(struct hll_vm *vm, struct hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  struct hll_obj *prev = hll_unwrap_car(args);
  //  CHECK_TYPE(prev, struct hll_obj_INT, "arguments");

  for (struct hll_obj *obj = hll_unwrap_cdr(args);
       hll_get_obj_kind(obj) == HLL_OBJ_CONS; obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *num = hll_unwrap_car(obj);
    //    CHECK_TYPE(num, struct hll_obj_INT, "arguments");

    if (hll_unwrap_num(prev) <= hll_unwrap_num(num)) {
      return vm->nil;
    }

    prev = num;
  }

  return vm->true_;
}

static struct hll_obj *builtin_num_ge(struct hll_vm *vm, struct hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  struct hll_obj *prev = hll_unwrap_car(args);
  //  CHECK_TYPE(prev, struct hll_obj_INT, "arguments");

  for (struct hll_obj *obj = hll_unwrap_cdr(args);
       hll_get_obj_kind(obj) != HLL_OBJ_NIL; obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *num = hll_unwrap_car(obj);
    //    CHECK_TYPE(num, struct hll_obj_INT, "arguments");

    if (hll_unwrap_num(prev) < hll_unwrap_num(num)) {
      return vm->nil;
    }
    prev = num;
  }

  return vm->true_;
}

static struct hll_obj *builtin_num_lt(struct hll_vm *vm, struct hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  struct hll_obj *prev = hll_unwrap_car(args);
  //  CHECK_TYPE(prev, struct hll_obj_INT, "arguments");

  for (struct hll_obj *obj = hll_unwrap_cdr(args);
       hll_get_obj_kind(obj) != HLL_OBJ_NIL; obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *num = hll_unwrap_car(obj);
    //    CHECK_TYPE(num, struct hll_obj_INT, "arguments");

    if (hll_unwrap_num(prev) >= hll_unwrap_num(num)) {
      return vm->nil;
    }
    prev = num;
  }

  return vm->true_;
}

static struct hll_obj *builtin_num_le(struct hll_vm *vm, struct hll_obj *args) {
  //  CHECK_HAS_ATLEAST_N_ARGS(1);

  struct hll_obj *prev = hll_unwrap_car(args);
  //  CHECK_TYPE(prev, struct hll_obj_INT, "arguments");

  for (struct hll_obj *obj = hll_unwrap_cdr(args);
       hll_get_obj_kind(obj) != HLL_OBJ_NIL; obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *num = hll_unwrap_car(obj);
    //    CHECK_TYPE(num, struct hll_obj_INT, "arguments");

    if (hll_unwrap_num(prev) > hll_unwrap_num(num)) {
      return vm->nil;
    }
    prev = num;
  }

  return vm->true_;
}

static struct hll_obj *builtin_rem(struct hll_vm *vm, struct hll_obj *args) {
  //  CHECK_HAS_N_ARGS(2);

  struct hll_obj *x = hll_unwrap_car(args);
  //  CHECK_TYPE(x, struct hll_obj_INT, "dividend");
  struct hll_obj *y = hll_unwrap_car(hll_unwrap_cdr(args));
  //  CHECK_TYPE(y, struct hll_obj_INT, "divisor");

  return hll_new_num(vm, fmod(hll_unwrap_num(x), hll_unwrap_num(y)));
}

static uint64_t xorshift64(uint64_t *state) {
  uint64_t x = *state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return *state = x;
}

static struct hll_obj *builtin_random(struct hll_vm *vm, struct hll_obj *args) {
  struct hll_obj *low = NULL, *high = NULL;
  if (hll_get_obj_kind(args) == HLL_OBJ_CONS) {
    high = hll_unwrap_car(args);
    args = hll_unwrap_cdr(args);

    if (hll_get_obj_kind(args) == HLL_OBJ_CONS) {
      low = hll_unwrap_car(args);
      args = hll_unwrap_cdr(args);

      if (hll_get_obj_kind(args) != HLL_OBJ_NIL) {
        hll_runtime_error(vm, "'random' form expects at most 2 arguments");
        return NULL;
      }
    }
  }

  if (HLL_UNLIKELY((high && hll_get_obj_kind(high) != HLL_OBJ_NUM) ||
                   (low && hll_get_obj_kind(low) != HLL_OBJ_NUM))) {
    hll_runtime_error(vm, "'random' form expects number arguments");
    return NULL;
  }

  double result;
  if (high == NULL) { // 0-1 float
    result = (double)xorshift64(&vm->rng_state) / (double)UINT64_MAX;
  } else if (low == NULL) { // 0-high int
    uint64_t upper = floor(hll_unwrap_num(high));
    uint64_t random = xorshift64(&vm->rng_state);
    result = random % upper;
  } else {
    assert(low != NULL && high != NULL);
    uint64_t lower = floor(hll_unwrap_num(high));
    uint64_t upper = floor(hll_unwrap_num(low));
    assert(upper > lower);
    uint64_t random = xorshift64(&vm->rng_state);
    result = random % (upper - lower) + lower;
  }

  return hll_new_num(vm, result);
}

static struct hll_obj *builtin_min(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_get_obj_kind(args) != HLL_OBJ_CONS)) {
    hll_runtime_error(vm, "min' form expects at least single argument");
    return NULL;
  }

  struct hll_obj *result = vm->nil;
  for (struct hll_obj *obj = args; hll_get_obj_kind(obj) == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *test = hll_unwrap_car(obj);
    if (HLL_UNLIKELY(hll_get_obj_kind(test) != HLL_OBJ_NUM)) {
      hll_runtime_error(vm, "'min' form expects number arguments");
      return NULL;
    }

    if (hll_get_obj_kind(result) == HLL_OBJ_NIL ||
        hll_unwrap_num(result) > hll_unwrap_num(test)) {
      result = test;
    }
  }

  return result;
}

static struct hll_obj *builtin_max(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_get_obj_kind(args) != HLL_OBJ_CONS)) {
    hll_runtime_error(vm, "max' form expects at least single argument");
    return NULL;
  }

  struct hll_obj *result = vm->nil;
  for (struct hll_obj *obj = args; hll_get_obj_kind(obj) == HLL_OBJ_CONS;
       obj = hll_unwrap_cdr(obj)) {
    struct hll_obj *test = hll_unwrap_car(obj);
    if (HLL_UNLIKELY(hll_get_obj_kind(test) != HLL_OBJ_NUM)) {
      hll_runtime_error(vm, "'max' form expects number arguments");
      return NULL;
    }

    if (hll_get_obj_kind(result) == HLL_OBJ_NIL ||
        hll_unwrap_num(result) < hll_unwrap_num(test)) {
      result = test;
    }
  }

  return result;
}

static struct hll_obj *builtin_listp(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'list?' expects exactly 1 argument");
    return NULL;
  }

  struct hll_obj *obj = hll_unwrap_car(args);
  struct hll_obj *result = vm->nil;
  if (hll_get_obj_kind(obj) == HLL_OBJ_CONS ||
      hll_get_obj_kind(obj) == HLL_OBJ_NIL) {
    result = vm->true_;
  }

  return result;
}

static struct hll_obj *builtin_null(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'null' expects exactly 1 argument");
    return NULL;
  }

  struct hll_obj *obj = hll_unwrap_car(args);
  struct hll_obj *result = vm->nil;

  if (hll_get_obj_kind(obj) == HLL_OBJ_NIL) {
    result = vm->true_;
  }

  return result;
}

static struct hll_obj *builtin_minusp(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'negative?' expects exactly 1 argument");
    return NULL;
  }

  struct hll_obj *obj = hll_unwrap_car(args);
  if (HLL_UNLIKELY(hll_get_obj_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'negative?' expects number argument");
    return NULL;
  }

  struct hll_obj *result = vm->nil;
  if (hll_unwrap_num(obj) < 0) {
    result = vm->true_;
  }

  return result;
}

static struct hll_obj *builtin_zerop(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'negative?' expects exactly 1 argument");
    return NULL;
  }

  struct hll_obj *obj = hll_unwrap_car(args);
  if (HLL_UNLIKELY(hll_get_obj_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'negative?' expects number argument");
    return NULL;
  }

  struct hll_obj *result = vm->nil;
  if (hll_unwrap_num(obj) == 0) {
    result = vm->true_;
  }

  return result;
}

static struct hll_obj *builtin_plusp(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'negative?' expects exactly 1 argument");
    return NULL;
  }

  struct hll_obj *obj = hll_unwrap_car(args);
  if (HLL_UNLIKELY(hll_get_obj_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'negative?' expects number argument");
    return NULL;
  }

  struct hll_obj *result = vm->nil;
  if (hll_unwrap_num(obj) > 0) {
    result = vm->true_;
  }

  return result;
}

static struct hll_obj *builtin_numberp(struct hll_vm *vm,
                                       struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'null' expects exactly 1 argument");
    return NULL;
  }

  struct hll_obj *obj = hll_unwrap_car(args);
  struct hll_obj *result = vm->nil;
  if (hll_get_obj_kind(obj) == HLL_OBJ_NUM) {
    result = vm->true_;
  }

  return result;
}

static struct hll_obj *builtin_abs(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'abs' expects exactly 1 argument");
    return NULL;
  }

  struct hll_obj *obj = hll_unwrap_car(args);
  if (HLL_UNLIKELY(hll_get_obj_kind(obj) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'abs' expects number argument");
    return NULL;
  }

  return hll_new_num(vm, fabs(hll_unwrap_num(obj)));
}

static struct hll_obj *builtin_append(struct hll_vm *vm, struct hll_obj *args) {
  if (hll_get_obj_kind(args) == HLL_OBJ_NIL) {
    return args;
  }

  (void)vm;
  struct hll_obj *list = hll_unwrap_car(args);
  while (hll_get_obj_kind(list) != HLL_OBJ_CONS &&
         hll_get_obj_kind(args) == HLL_OBJ_CONS) {
    args = hll_unwrap_cdr(args);
    list = hll_unwrap_car(args);
  }

  if (hll_get_obj_kind(list) == HLL_OBJ_NIL) {
    return list;
  }

  struct hll_obj *tail = list;
  for (struct hll_obj *slot = hll_unwrap_cdr(args);
       hll_get_obj_kind(slot) == HLL_OBJ_CONS; slot = hll_unwrap_cdr(slot)) {
    for (; hll_get_obj_kind(tail) == HLL_OBJ_CONS &&
           hll_get_obj_kind(hll_unwrap_cdr(tail)) != HLL_OBJ_NIL;
         tail = hll_unwrap_cdr(tail)) {
    }

    if (hll_get_obj_kind(tail) == HLL_OBJ_CONS) {
      hll_unwrap_cons(tail)->cdr = hll_unwrap_car(slot);
    } else {
      tail = slot;
      list = slot;
    }
  }

  return list;
}

static struct hll_obj *builtin_reverse(struct hll_vm *vm,
                                       struct hll_obj *args) {
  // NOTE: This seems like it can be compiled to bytecode directly
  if (HLL_UNLIKELY(hll_list_length(args) != 1)) {
    hll_runtime_error(vm, "'reverse' expects exactly 1 argument");
    return NULL;
  }

  struct hll_obj *obj = hll_unwrap_car(args);
  struct hll_obj *result = vm->nil;

  while (hll_get_obj_kind(obj) != HLL_OBJ_NIL) {
    assert(hll_get_obj_kind(obj) == HLL_OBJ_CONS);
    struct hll_obj *head = obj;
    obj = hll_unwrap_cdr(obj);
    hll_unwrap_cons(head)->cdr = result;
    result = head;
  }

  return result;
}

static struct hll_obj *builtin_nthcdr(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 2)) {
    hll_runtime_error(vm, "'nthcdr' expects exactly 2 arguments");
    return NULL;
  }

  struct hll_obj *num = hll_unwrap_car(args);
  if (HLL_UNLIKELY(hll_get_obj_kind(num) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'nthcdr' first argument must be a number");
    return NULL;
  } else if (HLL_UNLIKELY(hll_unwrap_num(num) < 0)) {
    hll_runtime_error(vm, "'nthcdr' expects non-negative number (got %F)",
                      hll_unwrap_num(num));
    return NULL;
  }

  size_t n = (size_t)floor(hll_unwrap_num(num));
  struct hll_obj *list = hll_unwrap_car(hll_unwrap_cdr(args));
  for (; hll_get_obj_kind(list) == HLL_OBJ_CONS && n != 0;
       --n, list = hll_unwrap_cdr(list))
    ;

  struct hll_obj *result = vm->nil;
  if (n == 0) {
    result = list;
  }

  return result;
}

static struct hll_obj *builtin_nth(struct hll_vm *vm, struct hll_obj *args) {
  if (HLL_UNLIKELY(hll_list_length(args) != 2)) {
    hll_runtime_error(vm, "'nth' expects exactly 2 arguments");
    return NULL;
  }

  struct hll_obj *num = hll_unwrap_car(args);
  if (HLL_UNLIKELY(hll_get_obj_kind(num) != HLL_OBJ_NUM)) {
    hll_runtime_error(vm, "'nth' first argument must be a number");
    return NULL;
  } else if (HLL_UNLIKELY(hll_unwrap_num(num) < 0)) {
    hll_runtime_error(vm, "'nth' expects non-negative number (got %F)",
                      hll_unwrap_num(num));
    return NULL;
  }

  size_t n = (size_t)floor(hll_unwrap_num(num));
  struct hll_obj *list = hll_unwrap_car(hll_unwrap_cdr(args));
  for (; hll_get_obj_kind(list) == HLL_OBJ_CONS && n != 0;
       --n, list = hll_unwrap_cdr(list))
    ;

  struct hll_obj *result = vm->nil;
  if (n == 0 && hll_get_obj_kind(list) == HLL_OBJ_CONS) {
    result = hll_unwrap_car(list);
  }

  return result;
}

static struct hll_obj *builtin_clear(struct hll_vm *vm, struct hll_obj *args) {
  (void)args;
  printf("\033[2J");
  return vm->nil;
}

static struct hll_obj *builtin_sleep(struct hll_vm *vm, struct hll_obj *args) {
  double num = hll_unwrap_num(hll_unwrap_car(args));
  usleep(num * 1000);
  return vm->nil;
}

static struct hll_obj *builtin_length(struct hll_vm *vm, struct hll_obj *args) {
  return hll_new_num(vm, hll_list_length(hll_unwrap_car(args)));
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
  hll_add_binding(vm, "number?", builtin_numberp);
  hll_add_binding(vm, "abs", builtin_abs);
  hll_add_binding(vm, "reverse", builtin_reverse);
  hll_add_binding(vm, "append", builtin_append);
  hll_add_binding(vm, "nthcdr", builtin_nthcdr);
  hll_add_binding(vm, "nth", builtin_nth);
  hll_add_binding(vm, "clear", builtin_clear);
  hll_add_binding(vm, "sleep", builtin_sleep);
  hll_add_binding(vm, "length", builtin_length);
  hll_interpret(
      vm, "builtins",
      "(defmacro (not x) (list 'if x () 't))                                \n"
      "(defmacro (when expr . body)                                         \n"
      "  (cons 'if (cons expr (list (cons 'progn body)))))                  \n"
      "(defmacro (unless expr . body)                                       \n"
      "  (cons 'if (cons expr (cons () body))))                             \n"
      "(define (map fn lis)                                                 \n"
      "  (when lis                                                          \n"
      "    (cons (fn (car lis))                                             \n"
      "          (map fn (cdr lis)))))                                      \n"
      "(define (any pred lis)                                               \n"
      "  (when lis                                                          \n"
      "    (or (pred (car lis))                                             \n"
      "        (any pred (cdr lis)))))                                      \n"
      "(define (filter pred lis)                                            \n"
      "  (when lis                                                          \n"
      "    (let ((value (pred (car lis))))                                  \n"
      "      (if value                                                      \n"
      "          (cons (car lis) (filter pred (cdr lis)))                   \n"
      "          (filter pred (cdr lis))))))                                \n"
      "(define (all pred lis)                                               \n"
      "  (if lis                                                            \n"
      "      (and (pred (car lis)) (all pred (cdr lis)))                    \n"
      "      t))                                                            \n"
      "(defmacro (not x) (list 'if x () 't))                                \n"
      "(defmacro (when expr . body)                                         \n"
      "  (cons 'if (cons expr (list (cons 'progn body)))))                  \n"
      "(defmacro (unless expr . body)                                       \n"
      "  (cons 'if (cons expr (cons () body))))                             \n"
      "(define (map fn lis)                                                 \n"
      "  (when lis                                                          \n"
      "    (cons (fn (car lis))                                             \n"
      "          (map fn (cdr lis)))))                                      \n"
      "(define (any pred lis)                                               \n"
      "  (when lis                                                          \n"
      "    (or (pred (car lis))                                             \n"
      "        (any pred (cdr lis)))))                                      \n"
      "(define (filter pred lis)                                            \n"
      "  (when lis                                                          \n"
      "    (let ((value (pred (car lis))))                                  \n"
      "      (if value                                                      \n"
      "          (cons (car lis) (filter pred (cdr lis)))                   \n"
      "          (filter pred (cdr lis))))))                                \n"
      "(define (all pred lis)                                               \n"
      "  (if lis                                                            \n"
      "      (and (pred (car lis)) (all pred (cdr lis)))                    \n"
      "      t))                                                            \n"
      "(define (reduce form lis)                                            \n"
      "  (if (cdr lis)                                                      \n"
      "      (form (car lis) (reduce form (cdr lis)))                       \n"
      "      (car lis)))                                                    \n"
      "(define (repeat n x)                                                 \n"
      "  (when (positive? n)                                                \n"
      "    (cons x (repeat (- n 1) x))))                                    \n"
      "(define (count pred lis)                                             \n"
      "  (if lis                                                            \n"
      "      (+ (if (pred (car lis)) 1 0) (count pred (cdr lis)))           \n"
      "      0))                                                            \n",
      false);
  hll_interpret(
      vm, "builtins part 2",
      "(defmacro (amap fn-body lis)                                         \n"
      "  (list 'map (list 'lambda (list 'it) fn-body) lis))                 \n"
      "(defmacro (acount fn-body lis)                                       \n"
      "  (list 'count (list 'lambda (list 'it) fn-body) lis))               \n",
      false);
}
