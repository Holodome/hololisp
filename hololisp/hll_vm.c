#include "hll_vm.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_compiler.h"
#include "hll_gc.h"
#include "hll_hololisp.h"
#include "hll_mem.h"
#include "hll_meta.h"
#include "hll_value.h"

extern void add_builtins(hll_vm *vm);

static void default_error_fn(hll_vm *vm, const char *text) {
  (void)vm;
  fprintf(stderr, "%s", text);
}

static void default_write_fn(hll_vm *vm, const char *text) {
  (void)vm;
  printf("%s", text);
  fflush(stdout);
}

void hll_initialize_default_config(hll_config *config) {
  config->write_fn = default_write_fn;
  config->error_fn = default_error_fn;

  config->heap_size = 10 << 20;
  config->min_heap_size = 1 << 20;
  config->heap_grow_percent = 50;

  config->user_data = NULL;
}

void hll_add_variable(hll_vm *vm, hll_value env, hll_value name,
                      hll_value value) {
  assert(hll_is_symb(name));
  hll_value slot = hll_new_cons(vm, name, value);
  hll_gc_push_temp_root(vm->gc, slot);
  hll_unwrap_env(env)->vars = hll_new_cons(vm, slot, hll_unwrap_env(env)->vars);
  hll_gc_pop_temp_root(vm->gc); // slot
}

hll_vm *hll_make_vm(const hll_config *config) {
  hll_vm *vm = hll_alloc(sizeof(*vm));

  if (config == NULL) {
    hll_initialize_default_config(&vm->config);
  } else {
    vm->config = *config;
  }
  // Set this value first not to accidentally trigger garbage collection with
  // allocating new nil object
  vm->gc = hll_make_gc(vm);
  vm->debug = hll_make_debug(vm, HLL_DEBUG_DIAGNOSTICS_COLORED);
  vm->rng_state = rand();

  vm->global_env = hll_new_env(vm, hll_nil(), hll_nil());
  vm->macro_env = hll_new_env(vm, hll_nil(), hll_nil());
  vm->env = vm->global_env;

  add_builtins(vm);
  return vm;
}

void hll_delete_vm(hll_vm *vm) {
  hll_delete_debug(vm->debug);
  hll_delete_gc(vm->gc);
  hll_free(vm, sizeof(hll_vm));
}

hll_interpret_result hll_interpret(hll_vm *vm, const char *source,
                                   const char *name, uint32_t flags) {
  hll_interpret_result result;

  hll_reset_debug(vm->debug);
  // Set debug colored flag
  vm->debug->flags &= ~HLL_DEBUG_DIAGNOSTICS_COLORED;
  if (flags & HLL_INTERPRET_COLORED) {
    vm->debug->flags |= HLL_DEBUG_DIAGNOSTICS_COLORED;
  }

  hll_value compiled;
  if (!hll_compile(vm, source, name, &compiled)) {
    result = HLL_RESULT_ERROR;
  } else {
    bool print_result = (flags & HLL_INTERPRET_PRINT_RESULT) != 0;
    result = hll_interpret_bytecode(vm, compiled, print_result);
  }

  if (result != HLL_RESULT_OK) {
    hll_debug_print_summary(vm->debug);
  }

  return result;
}

void hll_add_binding(hll_vm *vm, const char *symb_str,
                     hll_value (*bind_func)(hll_vm *vm, hll_value args)) {
  hll_value bind = hll_new_bind(vm, bind_func);
  hll_gc_push_temp_root(vm->gc, bind);
  hll_value symb = hll_new_symbolz(vm, symb_str);
  hll_gc_push_temp_root(vm->gc, symb);

  hll_add_variable(vm, vm->global_env, symb, bind);

  hll_gc_pop_temp_root(vm->gc); // bind
  hll_gc_pop_temp_root(vm->gc); // symb
}

bool hll_find_var(hll_value env, hll_value car, hll_value *found) {
  assert(hll_get_value_kind(car) == HLL_VALUE_SYMB &&
         "argument is not a symbol");
  hll_obj_symb *symb = hll_unwrap_symb(car);
  for (; !hll_is_nil(env); env = hll_unwrap_env(env)->up) {
    for (hll_value cons = hll_unwrap_env(env)->vars; hll_is_cons(cons);
         cons = hll_unwrap_cdr(cons)) {
      hll_value test = hll_unwrap_car(cons);
      assert(hll_get_value_kind(hll_unwrap_car(test)) == HLL_VALUE_SYMB &&
             "Variable is not a cons of symbol and its value");
      if (symb->hash == hll_unwrap_symb(hll_unwrap_car(test))->hash) {
        if (found != NULL) {
          *found = test;
        }
        return true;
      }
    }
  }

  return false;
}

void hll_print_value(hll_vm *vm, hll_value value) {
  switch (hll_get_value_kind(value)) {
  case HLL_VALUE_CONS:
    hll_print(vm, "(");
    while (hll_is_cons(value)) {
      hll_print_value(vm, hll_unwrap_car(value));

      hll_value cdr = hll_unwrap_cdr(value);
      if (!hll_is_list(cdr)) {
        hll_print(vm, " . ");
        hll_print_value(vm, cdr);
        break;
      } else if (hll_get_value_kind(cdr) != HLL_VALUE_NIL) {
        hll_print(vm, " ");
      }

      value = cdr;
    }
    hll_print(vm, ")");
    break;
  case HLL_VALUE_SYMB: {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s", hll_unwrap_zsymb(value));
    hll_print(vm, buffer);
  } break;
  case HLL_VALUE_NIL:
    hll_print(vm, "()");
    break;
  case HLL_VALUE_NUM: {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%lld", (long long)hll_unwrap_num(value));
    hll_print(vm, buffer);
  } break;
  case HLL_VALUE_TRUE:
    hll_print(vm, "t");
    break;
  case HLL_VALUE_BIND:
    hll_print(vm, "bind");
    break;
  case HLL_VALUE_ENV:
    hll_print(vm, "env");
    break;
  case HLL_VALUE_FUNC:
    hll_print(vm, "func");
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

void hll_runtime_error(hll_vm *vm, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  hll_report_runtime_errorv(vm->debug, fmt, args);
  va_end(args);

  longjmp(vm->err_jmp, 1);
}

static void call_func(hll_vm *vm, hll_value callable, hll_value args,
                      hll_call_frame **current_call_frame, bool mbtr) {
  switch (hll_get_value_kind(callable)) {
  case HLL_VALUE_FUNC: {
    hll_obj_func *func = hll_unwrap_func(callable);
    hll_value new_env = hll_new_env(vm, func->env, hll_nil());
    hll_gc_push_temp_root(vm->gc, new_env);
    hll_value param_name = func->param_names;
    hll_value param_value = args;
    if (hll_is_cons(param_name) && hll_is_symb(hll_unwrap_car(param_name))) {
      for (; hll_is_cons(param_name);
           param_name = hll_unwrap_cdr(param_name),
           param_value = hll_unwrap_cdr(param_value)) {
        if (hll_get_value_kind(param_value) != HLL_VALUE_CONS) {
          hll_runtime_error(vm, "number of arguments does not match");
        }
        hll_value name = hll_unwrap_car(param_name);
        assert(hll_is_symb(name));
        hll_value value = hll_unwrap_car(param_value);
        hll_add_variable(vm, new_env, name, value);
      }
    } else if (hll_is_cons(param_name) &&
               hll_is_nil(hll_unwrap_car(param_name))) {
      param_name = hll_unwrap_car(hll_unwrap_cdr(param_name));
    }

    if (!hll_is_nil(param_name)) {
      assert(hll_is_symb(param_name));
      hll_add_variable(vm, new_env, param_name, param_value);
    }

    if (mbtr && func->bytecode == (*current_call_frame)->bytecode) {
      (*current_call_frame)->ip = (*current_call_frame)->bytecode->ops;
    } else {
      hll_call_frame new_frame = {0};
      new_frame.bytecode = func->bytecode;
      new_frame.ip = func->bytecode->ops;
      new_frame.env = vm->env;
      new_frame.func = callable;
      hll_sb_push(vm->call_stack, new_frame);
    }
    *current_call_frame = &hll_sb_last(vm->call_stack);
    hll_gc_pop_temp_root(vm->gc); // new_env
    vm->env = new_env;
  } break;
  case HLL_VALUE_BIND: {
    hll_push_forbid_gc(vm->gc);
    hll_value result = hll_unwrap_bind(callable)->bind(vm, args);
    hll_pop_forbid_gc(vm->gc);
    hll_sb_push(vm->stack, result);
  } break;
  default:
    hll_runtime_error(vm, "object is not callable (got %s)",
                      hll_get_value_kind_str(hll_get_value_kind(callable)));
    break;
  }
}

#define HLL_USE_COMPUTED_GOTO

#ifndef HLL_USE_COMPUTED_GOTO
#define HLL_VM_SWITCH()                                                        \
  vm_start:                                                                    \
  switch (*current_call_frame->ip++)
#define HLL_VM_CASE(_name) case _name:
#define HLL_VM_DISPATCH()
#define HLL_VM_NEXT() goto vm_start;
#else
#define HLL_VM_SWITCH()
#define HLL_VM_CASE(_name) CASE_##_name:
#define HLL_VM_DISPATCH() goto *(dispatch_table[*current_call_frame->ip++])
#define HLL_VM_NEXT() HLL_VM_DISPATCH()
#endif

hll_value hll_interpret_bytecode_internal(hll_vm *vm, hll_value env_,
                                          hll_value compiled) {
  // Setup setjump for error handling
  if (setjmp(vm->err_jmp) == 1) {
    goto bail;
  }

  hll_gc_push_temp_root(vm->gc, compiled);
  hll_bytecode *initial_bytecode = hll_unwrap_func(compiled)->bytecode;
  vm->call_stack = NULL;
  vm->stack = NULL;
  vm->env = env_;

  {
    hll_call_frame original_frame = {0};
    original_frame.ip = initial_bytecode->ops;
    original_frame.bytecode = initial_bytecode;
    original_frame.env = env_;
    original_frame.func = compiled;
    hll_sb_push(vm->call_stack, original_frame);
  }
  hll_call_frame *current_call_frame = vm->call_stack;

#ifdef HLL_USE_COMPUTED_GOTO
  static const void *const dispatch_table[] = {
      &&CASE_HLL_BC_END,    &&CASE_HLL_BC_NIL,    &&CASE_HLL_BC_TRUE,
      &&CASE_HLL_BC_CONST,  &&CASE_HLL_BC_APPEND, &&CASE_HLL_BC_POP,
      &&CASE_HLL_BC_FIND,   &&CASE_HLL_BC_CALL,   &&CASE_HLL_BC_MBTRCALL,
      &&CASE_HLL_BC_JN,     &&CASE_HLL_BC_LET,    &&CASE_HLL_BC_PUSHENV,
      &&CASE_HLL_BC_POPENV, &&CASE_HLL_BC_CAR,    &&CASE_HLL_BC_CDR,
      &&CASE_HLL_BC_SETCAR, &&CASE_HLL_BC_SETCDR, &&CASE_HLL_BC_MAKEFUN,
  };
#endif
  HLL_VM_DISPATCH();

  HLL_VM_SWITCH() {
    HLL_VM_CASE(HLL_BC_END) {
      assert(hll_sb_len(vm->stack) != 0);
      vm->env = current_call_frame->env;
      assert(vm->env);
      (void)hll_sb_pop(vm->call_stack);
      if (hll_sb_len(vm->call_stack) == 0) {
        goto success;
      }
      current_call_frame = &hll_sb_last(vm->call_stack);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_POP) {
      assert(hll_sb_len(vm->stack) != 0);
      (void)hll_sb_pop(vm->stack);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_NIL) {
      hll_sb_push(vm->stack, hll_nil());
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_TRUE) {
      hll_sb_push(vm->stack, hll_true());
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_CONST) {
      uint16_t idx =
          (current_call_frame->ip[0] << 8) | current_call_frame->ip[1];
      current_call_frame->ip += 2;
      assert(idx < hll_sb_len(current_call_frame->bytecode->constant_pool));
      hll_value value = current_call_frame->bytecode->constant_pool[idx];
      hll_sb_push(vm->stack, value);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_APPEND) {
      assert(hll_sb_len(vm->stack) >= 3);
      hll_value *headp = &hll_sb_last(vm->stack) + -2;
      hll_value *tailp = &hll_sb_last(vm->stack) + -1;
      assert(hll_sb_len(vm->stack) != 0);
      hll_value obj = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, obj);

      hll_value cons = hll_new_cons(vm, obj, hll_nil());
      if (hll_is_nil(*headp)) {
        *headp = *tailp = cons;
      } else {
        if (hll_get_value_kind(*tailp) != HLL_VALUE_CONS) {
          hll_runtime_error(vm,
                            "tail operand of APPEND is not a cons (found %s)",
                            hll_get_value_kind_str(hll_get_value_kind(*tailp)));
        }
        hll_unwrap_cons(*tailp)->cdr = cons;
        *tailp = cons;
      }
      hll_gc_pop_temp_root(vm->gc); // obj
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_FIND) {
      hll_value symb = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, symb);
      if (hll_get_value_kind(symb) != HLL_VALUE_SYMB) {
        hll_runtime_error(vm, "operand of FIND is not a symb (found %s)",
                          hll_get_value_kind_str(hll_get_value_kind(symb)));
      }

      hll_value found;
      bool is_found = hll_find_var(vm->env, symb, &found);
      if (!is_found) {
        hll_runtime_error(vm, "failed to find variable '%s' in current scope",
                          hll_unwrap_zsymb(symb));
        goto bail;
      }
      hll_gc_pop_temp_root(vm->gc); // symb
      hll_sb_push(vm->stack, found);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_MAKEFUN) {
      uint16_t idx =
          (current_call_frame->ip[0] << 8) | current_call_frame->ip[1];
      current_call_frame->ip += 2;
      assert(idx < hll_sb_len(current_call_frame->bytecode->constant_pool));

      hll_value value = current_call_frame->bytecode->constant_pool[idx];
      assert(hll_get_value_kind(value) == HLL_VALUE_FUNC);
      value = hll_new_func(vm, hll_unwrap_func(value)->param_names,
                           hll_unwrap_func(value)->bytecode);
      hll_unwrap_func(value)->env = vm->env;

      hll_sb_push(vm->stack, value);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_MBTRCALL) {
      hll_value args = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, args);
      hll_value callable = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, callable);

      call_func(vm, callable, args, &current_call_frame, true);

      hll_gc_pop_temp_root(vm->gc); // args
      hll_gc_pop_temp_root(vm->gc); // callable
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_CALL) {
      hll_value args = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, args);
      hll_value callable = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, callable);

      call_func(vm, callable, args, &current_call_frame, false);

      hll_gc_pop_temp_root(vm->gc); // args
      hll_gc_pop_temp_root(vm->gc); // callable
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_JN) {
      uint16_t offset =
          (current_call_frame->ip[0] << 8) | current_call_frame->ip[1];
      current_call_frame->ip += 2;

      assert(hll_sb_len(vm->stack) != 0);
      hll_value cond = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, cond);
      if (hll_is_nil(cond)) {
        current_call_frame->ip += offset;
        assert(current_call_frame->ip <=
               &hll_sb_last(current_call_frame->bytecode->ops));
      }
      hll_gc_pop_temp_root(vm->gc);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_LET) {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value value = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, value);
      hll_value name = hll_sb_last(vm->stack);
      hll_add_variable(vm, vm->env, name, value);
      hll_gc_pop_temp_root(vm->gc); // value
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_PUSHENV) {
      vm->env = hll_new_env(vm, vm->env, hll_nil());
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_POPENV) {
      vm->env = hll_unwrap_env(vm->env)->up;
      assert(vm->env);
      assert(hll_get_value_kind(vm->env) == HLL_VALUE_ENV);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_CAR) {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value cons = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, cons);
      hll_value car = hll_car(vm, cons);
      hll_gc_pop_temp_root(vm->gc); // cons
      hll_sb_push(vm->stack, car);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_CDR) {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value cons = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, cons);
      hll_value cdr = hll_cdr(vm, cons);
      hll_gc_pop_temp_root(vm->gc); // cons
      hll_sb_push(vm->stack, cdr);
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_SETCAR) {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value car = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, car);
      hll_value cons = hll_sb_last(vm->stack);
      hll_gc_pop_temp_root(vm->gc); // car
      hll_unwrap_cons(cons)->car = car;
      HLL_VM_NEXT();
    }
    HLL_VM_CASE(HLL_BC_SETCDR) {
      assert(hll_sb_len(vm->stack) != 0);
      hll_value cdr = hll_sb_pop(vm->stack);
      hll_gc_push_temp_root(vm->gc, cdr);
      hll_value cons = hll_sb_last(vm->stack);
      hll_unwrap_cons(cons)->cdr = cdr;
      hll_gc_pop_temp_root(vm->gc); // cdr
      HLL_VM_NEXT();
    }
  }

  hll_value result;
  goto success;
success:
  result = vm->stack[0];
  goto end;
bail:
  result = hll_nil();
end:
  hll_sb_free(vm->stack);
  hll_sb_free(vm->call_stack);
  hll_gc_pop_temp_root(vm->gc); // callable

  return result;
}

hll_interpret_result hll_interpret_bytecode(hll_vm *vm, hll_value compiled,
                                            bool print_result) {
  hll_value result =
      hll_interpret_bytecode_internal(vm, vm->global_env, compiled);
  if (vm->debug->error_count != 0) {
    return HLL_RESULT_ERROR;
  }

  if (print_result && !vm->debug->error_count) {
    hll_print_value(vm, result);
    printf("\n");
  }

  return HLL_RESULT_OK;
}

hll_expand_macro_result hll_expand_macro(hll_vm *vm, hll_value macro,
                                         hll_value args, hll_value *dst) {
  hll_obj_func *func = hll_unwrap_func(macro);
  hll_value new_env = hll_new_env(vm, vm->global_env, hll_nil());
  hll_gc_push_temp_root(vm->gc, new_env);
  hll_value param_name = func->param_names;
  hll_value param_value = args;
  if (hll_is_cons(param_name) && hll_is_symb(hll_unwrap_car(param_name))) {
    for (; hll_is_cons(param_name); param_name = hll_unwrap_cdr(param_name),
                                    param_value = hll_unwrap_cdr(param_value)) {
      if (hll_get_value_kind(param_value) != HLL_VALUE_CONS) {
        return HLL_EXPAND_MACRO_ERR_ARGS;
      }
      hll_value name = hll_unwrap_car(param_name);
      assert(hll_is_symb(name));
      hll_value value = hll_unwrap_car(param_value);
      hll_add_variable(vm, new_env, name, value);
    }
  } else if (hll_is_cons(param_name) &&
             hll_is_nil(hll_unwrap_car(param_name))) {
    param_name = hll_unwrap_car(hll_unwrap_cdr(param_name));
  }

  if (!hll_is_nil(param_name)) {
    assert(hll_is_symb(param_name));
    hll_add_variable(vm, new_env, param_name, param_value);
  }

  hll_gc_pop_temp_root(vm->gc); // env
  *dst = hll_interpret_bytecode_internal(vm, new_env, macro);
  return HLL_EXPAND_MACRO_OK;
}

void hll_print(hll_vm *vm, const char *str) {
  if (vm->config.write_fn) {
    vm->config.write_fn(vm, str);
  }
}

hll_value hll_car(hll_vm *vm, hll_value lis) {
  if (hll_is_nil(lis)) {
    return lis;
  }

  if (hll_is_cons(lis)) {
    return hll_unwrap_car(lis);
  }

  hll_runtime_error(vm, "'car' argument is not a list");
  return lis;
}

hll_value hll_cdr(hll_vm *vm, hll_value lis) {
  if (hll_is_nil(lis)) {
    return lis;
  }

  if (hll_is_cons(lis)) {
    return hll_unwrap_cdr(lis);
  }

  hll_runtime_error(vm, "'cdr' argument is not a list");
  return lis;
}
