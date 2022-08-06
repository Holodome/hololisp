#include "hll_vm.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_compiler.h"
#include "hll_hololisp.h"
#include "hll_obj.h"
#include "hll_util.h"

extern void add_builtins(hll_vm *vm);

static void default_error_fn(hll_vm *vm, const char *text) {
  (void)vm;
  fprintf(stderr, "%s\n", text);
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

typedef struct {
  uint32_t line;
  uint32_t column;
} hll_source_loc;

static hll_source_loc get_source_loc(const char *source, size_t offset) {
  const char *cursor = source;
  hll_source_loc loc = {0};

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

void hll_report_error(hll_vm *vm, size_t offset, uint32_t len,
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

hll_interpret_result hll_interpret(hll_vm *vm, char const *name,
                                   const char *source) {
  vm->current_filename = name;
  vm->source = source;

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

  hll_unwrap_env(vm->env)->vars = hll_new_cons(vm, hll_new_cons(vm, symb, bind),
                                               hll_unwrap_env(vm->env)->vars);
}

static hll_obj *hll_find_var(hll_vm *vm, hll_obj *car) {
  assert(car->kind == HLL_OBJ_SYMB && "argument is not a symbol");
  hll_obj *result = NULL;

  for (hll_obj *env = vm->env; env->kind != HLL_OBJ_NIL && result == NULL;
       env = hll_unwrap_env(env)->up) {
    for (hll_obj *cons = hll_unwrap_env(env)->vars;
         cons->kind != HLL_OBJ_NIL && result == NULL;
         cons = hll_unwrap_cdr(cons)) {
      hll_obj *test = hll_unwrap_car(cons);
      assert(hll_unwrap_car(test)->kind == HLL_OBJ_SYMB &&
             "Variable is not a cons of symbol and its value");
      if (strcmp(hll_unwrap_zsymb(hll_unwrap_car(test)),
                 hll_unwrap_zsymb(car)) == 0) {
        result = test;
      }
    }
  }

  return result;
}

void hll_print(hll_vm *vm, hll_obj *obj, void *file) {
  switch (obj->kind) {
  case HLL_OBJ_CONS:
    fprintf(file, "(");
    while (obj->kind != HLL_OBJ_NIL) {
      assert(obj->kind == HLL_OBJ_CONS);
      hll_print(vm, hll_unwrap_car(obj), file);

      hll_obj *cdr = hll_unwrap_cdr(obj);
      if (cdr->kind != HLL_OBJ_NIL && cdr->kind != HLL_OBJ_CONS) {
        fprintf(file, " . ");
        hll_print(vm, cdr, file);
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

// Denotes situation that should be impossible in correctly compiled code.
HLL_ATTR(format(printf, 2, 3))
static void internal_compiler_error(hll_vm *vm, const char *fmt, ...) {
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
static void runtime_error(hll_vm *vm, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[4096];
  size_t a = snprintf(buffer, sizeof(buffer), "Runtime error: ");
  vsnprintf(buffer + a, sizeof(buffer) - a, fmt, args);
  va_end(args);
  // TODO: Line information
  hll_report_error(vm, 0, 0, buffer);
}

// static void dump_stack_trace(hll_vm *vm, hll_bytecode *bytecode, uint8_t *ip,
//                              hll_obj **stack) {
//   const char *file_dump_name = "/tmp/hololisp.dump";
//   FILE *f = fopen(file_dump_name, "w");
//   assert(f != NULL);
//
//   fprintf(f, "program dump:\n");
//   fprintf(f, "stack:\n");
//   size_t stack_size = hll_sb_len(stack);
//   for (size_t i = 0; i < stack_size; ++i) {
//     fprintf(f, " %zu: ", i);
//     print_internal(vm, stack[i], f);
//     fprintf(f, "\n");
//   }
//   fprintf(f, "failed instruction byte: %lx\n",
//           (long)(size_t)(ip - bytecode->ops));
//   fprintf(f, "whole program disassembly:\n");
//   hll_dump_bytecode(f, bytecode);
//
//   fclose(f);
//
//   printf("wrote trace to %s\n", file_dump_name);
// }

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
      if (HLL_UNLIKELY(idx >= hll_sb_len(bytecode->constant_pool))) {
        internal_compiler_error(
            vm, "constant idx %" PRIu16 " is not in allowed range (max is %zu)",
            idx, hll_sb_len(bytecode->constant_pool));
        goto bail;
      }
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
        if (HLL_UNLIKELY((*tailp)->kind != HLL_OBJ_CONS)) {
          runtime_error(vm, "tail operand of APPEND is not a cons (found %s)",
                        hll_get_object_kind_str((*tailp)->kind));
          goto bail;
        }
        hll_unwrap_cons(*tailp)->cdr = cons;
        *tailp = cons;
      }
    } break;
    case HLL_BYTECODE_FIND: {
      hll_obj *symb = hll_sb_pop(stack);
      if (HLL_UNLIKELY(symb->kind != HLL_OBJ_SYMB)) {
        runtime_error(vm, "operand of FIND is not a symb (found %s)",
                      hll_get_object_kind_str(symb->kind));
        goto bail;
      }

      hll_obj *found = hll_find_var(vm, symb);
      if (HLL_UNLIKELY(found == NULL)) {
        runtime_error(vm, "failed to find variable '%s' in current scope",
                      hll_unwrap_zsymb(symb));
        goto bail;
      }

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
        if (HLL_UNLIKELY(ip > &hll_sb_last(bytecode->ops))) {
          internal_compiler_error(vm,
                                  "jump is out of bytecode bounds (was %p, "
                                  "jump %p, became %p, bound %p)",
                                  (void *)(ip - offset),
                                  (void *)(uintptr_t)offset, (void *)ip,
                                  (void *)&hll_sb_last(bytecode->ops));
          goto bail;
        }
      }
    } break;
    case HLL_BYTECODE_MAKE_LAMBDA:
      assert(!"Not implemented LAMBDA");
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
      hll_obj *car;
      if (cons->kind == HLL_OBJ_NIL) {
        car = vm->nil;
      } else if (HLL_UNLIKELY(cons->kind != HLL_OBJ_CONS)) {
        runtime_error(vm, "CAR operand is not a cons (found %s)",
                      hll_get_object_kind_str(cons->kind));
        goto bail;
      } else {
        car = hll_unwrap_car(cons);
        ;
      }

      hll_sb_push(stack, car);
    } break;
    case HLL_BYTECODE_CDR: {
      hll_obj *cons = hll_sb_pop(stack);
      hll_obj *cdr;
      if (cons->kind == HLL_OBJ_NIL) {
        cdr = vm->nil;
      } else if (HLL_UNLIKELY(cons->kind != HLL_OBJ_CONS)) {
        runtime_error(vm, "CDR operand is not a cons (found %s)",
                      hll_get_object_kind_str(cons->kind));
        goto bail;
      } else {
        cdr = hll_unwrap_cdr(cons);
      }
      hll_sb_push(stack, cdr);
    } break;
    case HLL_BYTECODE_SETCAR: {
      hll_obj *car = hll_sb_pop(stack);
      hll_obj *cons = hll_sb_last(stack);
      if (HLL_UNLIKELY(cons->kind != HLL_OBJ_CONS)) {
        runtime_error(vm, "cons SETCAR operand is not a cons (found %s)",
                      hll_get_object_kind_str(cons->kind));
        goto bail;
      }
      hll_unwrap_cons(cons)->car = car;
    } break;
    case HLL_BYTECODE_SETCDR: {
      hll_obj *cdr = hll_sb_pop(stack);
      hll_obj *cons = hll_sb_last(stack);
      if (HLL_UNLIKELY(cons->kind != HLL_OBJ_CONS)) {
        runtime_error(vm, "cons SETCDR operand is not a cons (found %s)",
                      hll_get_object_kind_str(cons->kind));
        goto bail;
      }
      hll_unwrap_cons(cons)->cdr = cdr;
    } break;
    default:
      internal_compiler_error(vm, "Unknown instruction: %" PRIx8, op);
      break;
    }
  }

  if (print_result) {
    if (hll_sb_len(stack) != 1) {
      internal_compiler_error(vm, "stack is empty (expected value to print)");
      hll_dump_bytecode(stderr, bytecode);
    }
    hll_obj *obj = stack[0];
    hll_print(vm, obj, stdout);
    printf("\n");
  }

bail:
  hll_sb_free(stack);

  return HLL_RESULT_OK;
}
