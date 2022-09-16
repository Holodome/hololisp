#include "hll_vm.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hll_compiler.h"
#include "hll_hololisp.h"
#include "hll_obj.h"
#include "hll_util.h"

extern void add_builtins(struct hll_vm *vm);

static void default_error_fn(struct hll_vm *vm, const char *text) {
  (void)vm;
  fprintf(stderr, "%s", text);
}

static void default_write_fn(struct hll_vm *vm, const char *text) {
  (void)vm;
  printf("%s", text);
}

static void initialize_default_config(struct hll_config *config) {
  config->write_fn = default_write_fn;
  config->error_fn = default_error_fn;

  config->heap_size = 10 << 20;
  config->min_heap_size = 1 << 20;
  config->heap_grow_percent = 50;

  config->user_data = NULL;
}

typedef struct {
  uint32_t line;
  uint32_t column;
} hll_source_loc;

static hll_source_loc get_source_loc(const char *source, size_t offset) {
  const char *cursor = source;
  hll_source_loc loc = {0};
  if (cursor == NULL) {
    return loc;
  }

  while (cursor < source + offset) {
    char symb = *cursor++;
    if (symb == '\n') {
      ++loc.line;
      loc.column = 0;
    } else {
      ++loc.column;
    }
  }

  return loc;
}

void hll_report_error(struct hll_vm *vm, size_t offset, uint32_t len,
                      const char *msg) {
  (void)len; // TODO: use in reporting parts of source code.
  ++vm->error_count;
  if (vm->config.error_fn == NULL) {
    return;
  }

  char buffer[4096];
  const char *filename = vm->current_filename;
  hll_source_loc loc = get_source_loc(vm->source, offset);
  if (filename != NULL) {
    snprintf(buffer, sizeof(buffer), "%s:%u:%u: %s\n", filename, loc.line,
             loc.column, msg);
  } else {
    snprintf(buffer, sizeof(buffer), "%u:%u: %s\n", loc.line, loc.column, msg);
  }

  vm->config.error_fn(vm, buffer);
}

void hll_add_variable(struct hll_vm *vm, hll_value env, hll_value name,
                      hll_value value) {
  assert(hll_get_value_kind(name) == HLL_OBJ_SYMB);
  hll_value slot = hll_new_cons(vm, name, value);
  hll_sb_push(vm->temp_roots, slot);
  hll_unwrap_env(env)->vars = hll_new_cons(vm, slot, hll_unwrap_env(env)->vars);
  (void)hll_sb_pop(vm->temp_roots); // slot
}

struct hll_vm *hll_make_vm(const struct hll_config *config) {
  struct hll_vm *vm = hll_alloc(sizeof(struct hll_vm));

  if (config == NULL) {
    initialize_default_config(&vm->config);
  } else {
    vm->config = *config;
  }
  // Set this value first not to accidentally trigger garbage collection with
  // allocating new nil object
  vm->next_gc = vm->config.heap_size;
  vm->rng_state = time(NULL);

  vm->quote_symb = hll_new_symbolz(vm, "quote");
  vm->nthcdr_symb = hll_new_symbolz(vm, "nthcdr");
  vm->global_env = hll_new_env(vm, hll_nil(), hll_nil());
  vm->env = vm->global_env;

  add_builtins(vm);
  return vm;
}

void hll_delete_vm(struct hll_vm *vm) {
  struct hll_obj *obj = vm->all_objects;
  while (obj != NULL) {
    struct hll_obj *next = obj->next_gc;
    assert(next != obj);
    hll_free_object(vm, obj);
    obj = next;
  }
  hll_sb_free(vm->gray_objs);
  hll_sb_free(vm->temp_roots);
  hll_free(vm, sizeof(struct hll_vm));
}

enum hll_interpret_result hll_interpret(struct hll_vm *vm, const char *name,
                                        const char *source, bool print_result) {
  vm->current_filename = name;
  vm->source = source;

  hll_value compiled;
  if (!hll_compile(vm, source, &compiled)) {
    return HLL_RESULT_COMPILE_ERROR;
  }

  enum hll_interpret_result result =
      hll_interpret_bytecode(vm, compiled, print_result)
          ? HLL_RESULT_RUNTIME_ERROR
          : HLL_RESULT_OK;
  return result;
}

void hll_add_binding(struct hll_vm *vm, const char *symb_str,
                     hll_value (*bind_func)(struct hll_vm *vm,
                                            hll_value args)) {
  hll_value bind = hll_new_bind(vm, bind_func);
  hll_sb_push(vm->temp_roots, bind);
  hll_value symb = hll_new_symbolz(vm, symb_str);
  hll_sb_push(vm->temp_roots, symb);

  hll_add_variable(vm, vm->global_env, symb, bind);

  (void)hll_sb_pop(vm->temp_roots); // bind
  (void)hll_sb_pop(vm->temp_roots); // symb
}

bool hll_find_var(struct hll_vm *vm, hll_value env, hll_value car,
                  hll_value *found) {
  (void)vm;
  assert(hll_get_value_kind(car) == HLL_OBJ_SYMB && "argument is not a symbol");
  struct hll_obj_symb *symb = hll_unwrap_symb(car);
  for (; !hll_is_nil(env); env = hll_unwrap_env(env)->up) {
    for (hll_value cons = hll_unwrap_env(env)->vars; !hll_is_nil(cons);
         cons = hll_unwrap_cdr(cons)) {
      hll_value test = hll_unwrap_car(cons);
      assert(hll_get_value_kind(hll_unwrap_car(test)) == HLL_OBJ_SYMB &&
             "Variable is not a cons of symbol and its value");
      if (symb->hash == hll_unwrap_symb(hll_unwrap_car(test))->hash) {
        *found = test;
        return true;
      }
    }
  }

  return false;
}

void hll_print(struct hll_vm *vm, hll_value value, void *file) {
  switch (hll_get_value_kind(value)) {
  case HLL_OBJ_CONS:
    fprintf(file, "(");
    while (hll_get_value_kind(value) != HLL_OBJ_NIL) {
      assert(hll_get_value_kind(value) == HLL_OBJ_CONS);
      hll_print(vm, hll_unwrap_car(value), file);

      hll_value cdr = hll_unwrap_cdr(value);
      if (hll_get_value_kind(cdr) != HLL_OBJ_NIL &&
          hll_get_value_kind(cdr) != HLL_OBJ_CONS) {
        fprintf(file, " . ");
        hll_print(vm, cdr, file);
        break;
      } else if (hll_get_value_kind(cdr) != HLL_OBJ_NIL) {
        fprintf(file, " ");
      }

      value = cdr;
    }
    fprintf(file, ")");
    break;
  case HLL_OBJ_SYMB:
    fprintf(file, "%s", hll_unwrap_zsymb(value));
    break;
  case HLL_OBJ_NIL:
    fprintf(file, "()");
    break;
  case HLL_OBJ_NUM:
    fprintf(file, "%lld", (long long)hll_unwrap_num(value));
    break;
  case HLL_OBJ_TRUE:
    fprintf(file, "t");
    break;
  case HLL_OBJ_BIND:
    fprintf(file, "bind");
    break;
  case HLL_OBJ_ENV:
    fprintf(file, "env");
    break;
  case HLL_OBJ_FUNC:
    fprintf(file, "func");
    break;
  default:
    assert(!"Not implemented");
    break;
  }
}

// Denotes situation that should be impossible in correctly compiled code.
HLL_ATTR(format(printf, 2, 3))
static void internal_compiler_error(struct hll_vm *vm, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[4096];
  size_t a = snprintf(buffer, sizeof(buffer), "INTERNAL ERROR: ");
  vsnprintf(buffer + a, sizeof(buffer) - a, fmt, args);
  va_end(args);
  // TODO: Line information
  hll_report_error(vm, 0, 0, buffer);
}

HLL_ATTR(format(printf, 2, 3))
void hll_runtime_error(struct hll_vm *vm, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[4096];
  size_t a = snprintf(buffer, sizeof(buffer), "Runtime error: ");
  vsnprintf(buffer + a, sizeof(buffer) - a, fmt, args);
  va_end(args);
  // TODO: Line information
  hll_report_error(vm, 0, 0, buffer);
}

static void hll_collect_garbage(struct hll_vm *vm) {
  // Reset allocated bytes count
  vm->bytes_allocated = 0;
  // These objects have unique instances
  hll_sb_purge(vm->gray_objs);
  hll_gray_obj(vm, vm->quote_symb);
  hll_gray_obj(vm, vm->nthcdr_symb);
  hll_gray_obj(vm, vm->global_env);
  for (size_t i = 0; i < hll_sb_len(vm->temp_roots); ++i) {
    hll_gray_obj(vm, vm->temp_roots[i]);
  }
  for (size_t i = 0; i < hll_sb_len(vm->stack); ++i) {
    hll_gray_obj(vm, vm->stack[i]);
  }
  for (size_t i = 0; i < hll_sb_len(vm->call_stack); ++i) {
    struct hll_call_frame *f = &vm->call_stack[i];
    hll_gray_obj(vm, f->env);
    hll_gray_obj(vm, f->func);
  }
  hll_gray_obj(vm, vm->env);

  for (size_t i = 0; i < hll_sb_len(vm->gray_objs); ++i) {
    hll_blacken_obj(vm, vm->gray_objs[i]);
  }

  // Free all objects not marked
  struct hll_obj **obj_ptr = &vm->all_objects;
  while (*obj_ptr != NULL) {
    if (!(*obj_ptr)->is_dark) {
      struct hll_obj *to_free = *obj_ptr;
      *obj_ptr = to_free->next_gc;
      hll_free_object(vm, to_free);
    } else {
      (*obj_ptr)->is_dark = false;
      obj_ptr = &(*obj_ptr)->next_gc;
    }
  }

  //    fprintf(stderr, "after collection %zu\n", vm->bytes_allocated);
  vm->next_gc = vm->bytes_allocated +
                ((vm->bytes_allocated * vm->config.heap_grow_percent) / 100);
  if (vm->next_gc < vm->config.min_heap_size) {
    vm->next_gc = vm->config.min_heap_size;
  }
}

void *hll_gc_realloc(struct hll_vm *vm, void *ptr, size_t old_size,
                     size_t new_size) {
  vm->bytes_allocated -= old_size;
  vm->bytes_allocated += new_size;

  if (new_size > 0 && !vm->forbid_gc
#if !HLL_STRESS_GC
      && vm->bytes_allocated > vm->next_gc
#endif
  ) {
    hll_collect_garbage(vm);
  }

  return hll_realloc(ptr, old_size, new_size);
}

hll_value hll_interpret_bytecode_internal(struct hll_vm *vm, hll_value env_,
                                          hll_value compiled) {
  hll_sb_push(vm->temp_roots, compiled);
  struct hll_bytecode *initial_bytecode;
  if (hll_get_value_kind(compiled) == HLL_OBJ_FUNC) {
    initial_bytecode = hll_unwrap_func(compiled)->bytecode;
  } else {
    initial_bytecode = hll_unwrap_macro(compiled)->bytecode;
  }
  vm->call_stack = NULL;
  vm->stack = NULL;
  vm->env = env_;

  struct hll_call_frame original_frame = {0};
  original_frame.ip = initial_bytecode->ops;
  original_frame.bytecode = initial_bytecode;
  original_frame.env = env_;
  original_frame.func = compiled;
  assert(env_);
  hll_sb_push(vm->call_stack, original_frame);

  while (hll_sb_len(vm->call_stack)) {
    uint8_t op = *hll_sb_last(vm->call_stack).ip++;
    switch (op) {
    case HLL_BYTECODE_END:
      assert(hll_sb_len(vm->stack) != 0);
      vm->env = hll_sb_last(vm->call_stack).env;
      assert(vm->env);
      (void)hll_sb_pop(vm->call_stack);
      break;
    case HLL_BYTECODE_POP:
      assert(hll_sb_len(vm->stack) != 0);
      (void)hll_sb_pop(vm->stack);
      break;
    case HLL_BYTECODE_NIL:
      hll_sb_push(vm->stack, hll_nil());
      break;
    case HLL_BYTECODE_TRUE:
      hll_sb_push(vm->stack, hll_true());
      break;
    case HLL_BYTECODE_CONST: {
      uint16_t idx = (hll_sb_last(vm->call_stack).ip[0] << 8) |
                     hll_sb_last(vm->call_stack).ip[1];
      hll_sb_last(vm->call_stack).ip += 2;
      if (HLL_UNLIKELY(idx >= hll_sb_len(hll_sb_last(vm->call_stack)
                                             .bytecode->constant_pool))) {
        internal_compiler_error(
            vm, "constant idx %" PRIu16 " is not in allowed range (max is %zu)",
            idx,
            hll_sb_len(hll_sb_last(vm->call_stack).bytecode->constant_pool));
        goto bail;
      }
      hll_value value =
          hll_sb_last(vm->call_stack).bytecode->constant_pool[idx];
      hll_sb_push(vm->stack, value);
    } break;
    case HLL_BYTECODE_APPEND: {
      assert(hll_sb_len(vm->stack) >= 3);
      hll_value *headp = &hll_sb_last(vm->stack) + -2;
      hll_value *tailp = &hll_sb_last(vm->stack) + -1;
      assert(hll_sb_len(vm->stack) != 0);
      hll_value obj = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, obj);

      hll_value cons = hll_new_cons(vm, obj, hll_nil());
      if (hll_get_value_kind(*headp) == HLL_OBJ_NIL) {
        *headp = *tailp = cons;
      } else {
        if (HLL_UNLIKELY(hll_get_value_kind(*tailp) != HLL_OBJ_CONS)) {
          hll_runtime_error(
              vm, "tail operand of APPEND is not a cons (found %s)",
              hll_get_object_kind_str(hll_get_value_kind(*tailp)));
          goto bail;
        }
        hll_unwrap_cons(*tailp)->cdr = cons;
        *tailp = cons;
      }
      (void)hll_sb_pop(vm->temp_roots); // obj
    } break;
    case HLL_BYTECODE_FIND: {
      hll_value symb = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, symb);
      if (HLL_UNLIKELY(hll_get_value_kind(symb) != HLL_OBJ_SYMB)) {
        hll_runtime_error(vm, "operand of FIND is not a symb (found %s)",
                          hll_get_object_kind_str(hll_get_value_kind(symb)));
        goto bail;
      }

      hll_value found;
      bool is_found = hll_find_var(vm, vm->env, symb, &found);
      if (HLL_UNLIKELY(!is_found)) {
        hll_runtime_error(vm, "failed to find variable '%s' in current scope",
                          hll_unwrap_zsymb(symb));
        goto bail;
      }
      (void)hll_sb_pop(vm->temp_roots); // symb
      hll_sb_push(vm->stack, found);
    } break;
    case HLL_BYTECODE_MAKEFUN: {
      uint16_t idx = (hll_sb_last(vm->call_stack).ip[0] << 8) |
                     hll_sb_last(vm->call_stack).ip[1];
      hll_sb_last(vm->call_stack).ip += 2;
      if (HLL_UNLIKELY(idx >= hll_sb_len(hll_sb_last(vm->call_stack)
                                             .bytecode->constant_pool))) {
        internal_compiler_error(
            vm, "constant idx %" PRIu16 " is not in allowed range (max is %zu)",
            idx,
            hll_sb_len(hll_sb_last(vm->call_stack).bytecode->constant_pool));
        goto bail;
      }

      hll_value value =
          hll_sb_last(vm->call_stack).bytecode->constant_pool[idx];
      if (HLL_UNLIKELY(hll_get_value_kind(value) != HLL_OBJ_FUNC)) {
        internal_compiler_error(
            vm, "specified object is not a function (got %s)",
            hll_get_object_kind_str(hll_get_value_kind(value)));
        goto bail;
      }
      // Copy the function
      value = hll_copy_obj(vm, value);
      struct hll_obj_func *func = hll_unwrap_func(value);
      func->env = vm->env;

      hll_sb_push(vm->stack, value);
    } break;
    case HLL_BYTECODE_CALL: {
      hll_value args = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, args);
      hll_value callable = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, callable);

      switch (hll_get_value_kind(callable)) {
      case HLL_OBJ_FUNC: {
        struct hll_obj_func *func = hll_unwrap_func(callable);
        hll_value new_env = hll_new_env(vm, func->env, hll_nil());
        hll_sb_push(vm->temp_roots, new_env);
        hll_value param_name = func->param_names;
        hll_value param_value = args;
        if (hll_is_cons(param_name) &&
            hll_is_symb(hll_unwrap_car(param_name))) {
          for (; hll_is_cons(param_name);
               param_name = hll_unwrap_cdr(param_name),
               param_value = hll_unwrap_cdr(param_value)) {
            if (hll_get_value_kind(param_value) != HLL_OBJ_CONS) {
              hll_runtime_error(vm, "number of arguments does not match");
              goto bail;
            }
            hll_value name = hll_unwrap_car(param_name);
            assert(hll_get_value_kind(name) == HLL_OBJ_SYMB);
            hll_value value = hll_unwrap_car(param_value);
            hll_add_variable(vm, new_env, name, value);
          }
        } else if (hll_is_cons(param_name) &&
                   hll_is_nil(hll_unwrap_car(param_name))) {
          param_name = hll_unwrap_car(hll_unwrap_cdr(param_name));
        }

        if (!hll_is_nil(param_name)) {
          assert(hll_get_value_kind(param_name) == HLL_OBJ_SYMB);
          hll_add_variable(vm, new_env, param_name, param_value);
        }

        struct hll_call_frame new_frame = {0};
        new_frame.bytecode = func->bytecode;
        new_frame.ip = func->bytecode->ops;
        new_frame.env = vm->env;
        new_frame.func = callable;
        hll_sb_push(vm->call_stack, new_frame);
        (void)hll_sb_pop(vm->temp_roots); // new_env
        vm->env = new_env;
      } break;
      case HLL_OBJ_BIND: {
        ++vm->forbid_gc;
        hll_value result = hll_unwrap_bind(callable)->bind(vm, args);
        --vm->forbid_gc;
        hll_sb_push(vm->stack, result);
      } break;
      default:
        hll_runtime_error(
            vm, "object is not callable (got %s)",
            hll_get_object_kind_str(hll_get_value_kind(callable)));
        goto bail;
        break;
      }

      (void)hll_sb_pop(vm->temp_roots); // args
      (void)hll_sb_pop(vm->temp_roots); // callable
    } break;
    case HLL_BYTECODE_DUP: {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value last = hll_sb_last(vm->stack);
      hll_sb_push(vm->stack, last);
    } break;
    case HLL_BYTECODE_JN: {
      int16_t offset = (hll_sb_last(vm->call_stack).ip[0] << 8) |
                       hll_sb_last(vm->call_stack).ip[1];
      hll_sb_last(vm->call_stack).ip += 2;

      assert(hll_sb_len(vm->stack) != 0);
      hll_value cond = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, cond);
      if (hll_get_value_kind(cond) == HLL_OBJ_NIL) {
        hll_sb_last(vm->call_stack).ip += offset;
        if (HLL_UNLIKELY(
                hll_sb_last(vm->call_stack).ip >
                &hll_sb_last(hll_sb_last(vm->call_stack).bytecode->ops))) {
          internal_compiler_error(
              vm,
              "jump is out of bytecode bounds (was %p, "
              "jump %p, became %p, bound %p)",
              (void *)(hll_sb_last(vm->call_stack).ip - offset),
              (void *)(uintptr_t)offset, (void *)hll_sb_last(vm->call_stack).ip,
              (void *)&hll_sb_last(hll_sb_last(vm->call_stack).bytecode->ops));
          hll_dump_bytecode(stderr, initial_bytecode);
          goto bail;
        }
      }
      (void)hll_sb_pop(vm->temp_roots);
    } break;
    case HLL_BYTECODE_LET: {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value value = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, value);
      hll_value name = hll_sb_last(vm->stack);
      hll_add_variable(vm, vm->env, name, value);
      (void)hll_sb_pop(vm->temp_roots); // value
    } break;
    case HLL_BYTECODE_PUSHENV:
      vm->env = hll_new_env(vm, vm->env, hll_nil());
      break;
    case HLL_BYTECODE_POPENV:
      vm->env = hll_unwrap_env(vm->env)->up;
      assert(vm->env);
      assert(hll_get_value_kind(vm->env) == HLL_OBJ_ENV);
      break;
    case HLL_BYTECODE_CAR: {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value cons = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, cons);
      hll_value car;
      if (hll_get_value_kind(cons) == HLL_OBJ_NIL) {
        car = hll_nil();
      } else if (HLL_UNLIKELY(hll_get_value_kind(cons) != HLL_OBJ_CONS)) {
        hll_runtime_error(vm, "CAR operand is not a cons (found %s)",
                          hll_get_object_kind_str(hll_get_value_kind(cons)));
        goto bail;
      } else {
        car = hll_unwrap_car(cons);
      }

      (void)hll_sb_pop(vm->temp_roots); // cons
      hll_sb_push(vm->stack, car);
    } break;
    case HLL_BYTECODE_CDR: {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value cons = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, cons);
      hll_value cdr;
      if (hll_get_value_kind(cons) == HLL_OBJ_NIL) {
        cdr = hll_nil();
      } else if (HLL_UNLIKELY(hll_get_value_kind(cons) != HLL_OBJ_CONS)) {
        hll_runtime_error(vm, "CDR operand is not a cons (found %s)",
                          hll_get_object_kind_str(hll_get_value_kind(cons)));
        goto bail;
      } else {
        cdr = hll_unwrap_cdr(cons);
      }
      (void)hll_sb_pop(vm->temp_roots); // cons
      hll_sb_push(vm->stack, cdr);
    } break;
    case HLL_BYTECODE_SETCAR: {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value car = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, car);
      hll_value cons = hll_sb_last(vm->stack);
      if (HLL_UNLIKELY(hll_get_value_kind(cons) != HLL_OBJ_CONS)) {
        hll_runtime_error(vm, "cons SETCAR operand is not a cons (found %s)",
                          hll_get_object_kind_str(hll_get_value_kind(cons)));
        goto bail;
      }
      (void)hll_sb_pop(vm->temp_roots); // car
      hll_unwrap_cons(cons)->car = car;
    } break;
    case HLL_BYTECODE_SETCDR: {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value cdr = hll_sb_pop(vm->stack);
      hll_sb_push(vm->temp_roots, cdr);
      hll_value cons = hll_sb_last(vm->stack);
      if (HLL_UNLIKELY(hll_get_value_kind(cons) != HLL_OBJ_CONS)) {
        hll_runtime_error(vm, "cons SETCDR operand is not a cons (found %s)",
                          hll_get_object_kind_str(hll_get_value_kind(cons)));
        goto bail;
      }
      hll_unwrap_cons(cons)->cdr = cdr;
      (void)hll_sb_pop(vm->temp_roots); // cdr
    } break;
    default:
      internal_compiler_error(vm, "Unknown instruction: %" PRIx8, op);
      break;
    }
  }

  goto out;
bail:
  //  hll_dump_bytecode(stderr, initial_bytecode);
  //  for (size_t i = 0; i < hll_sb_len(initial_bytecode->constant_pool); ++i) {
  //    hll_obj *test = initial_bytecode->constant_pool[i];
  //    fprintf(stderr, "\n\n");
  //    if (hll_get_value_kind(test) == HLL_OBJ_FUNC) {
  //      hll_dump_bytecode(stderr, hll_unwrap_func(test)->bytecode);
  //    }
  //  }
  return hll_nil();
out:
  assert(hll_sb_len(vm->stack));
  hll_value result = vm->stack[0];
  hll_sb_free(vm->stack);
  hll_sb_free(vm->call_stack);
  vm->call_stack = NULL;
  vm->stack = NULL;

  (void)hll_sb_pop(vm->temp_roots);

  return result;
}

bool hll_interpret_bytecode(struct hll_vm *vm, hll_value compiled,
                            bool print_result) {
  hll_value result =
      hll_interpret_bytecode_internal(vm, vm->global_env, compiled);
  if (print_result) {
    //    if (result == NULL) {
    //      internal_compiler_error(vm, "expected one value to print");
    //      return true;
    //    }
    hll_print(vm, result, stdout);
    printf("\n");
  }

  return false;
}

hll_value hll_expand_macro(struct hll_vm *vm, hll_value macro, hll_value args) {
  hll_value env = hll_new_env(vm, vm->global_env, hll_nil());
  hll_sb_push(vm->temp_roots, env);

  struct hll_obj_func *fun = hll_unwrap_macro(macro);
  hll_value name_slot = fun->param_names;
  hll_value value_slot = args;
  for (; hll_get_value_kind(name_slot) == HLL_OBJ_CONS;
       name_slot = hll_unwrap_cdr(name_slot),
       value_slot = hll_unwrap_cdr(value_slot)) {
    if (hll_get_value_kind(value_slot) != HLL_OBJ_CONS) {
      hll_runtime_error(vm, "number of arguments does not match");
      (void)hll_sb_pop(vm->temp_roots); // env
      return hll_nil();
    }
    hll_value name = hll_unwrap_car(name_slot);
    assert(hll_get_value_kind(name) == HLL_OBJ_SYMB);
    hll_value value = hll_unwrap_car(value_slot);
    hll_add_variable(vm, env, name, value);
  }

  if (hll_get_value_kind(name_slot) != HLL_OBJ_NIL) {
    assert(hll_get_value_kind(name_slot) == HLL_OBJ_SYMB);
    hll_add_variable(vm, env, name_slot, value_slot);
  }

  (void)hll_sb_pop(vm->temp_roots); // env
  return hll_interpret_bytecode_internal(vm, env, macro);
}
