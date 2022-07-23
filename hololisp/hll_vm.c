#include "hll_vm.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

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

hll_vm *hll_make_vm(hll_config const *config) {
  hll_vm *vm = calloc(1, sizeof(hll_vm));

  if (config == NULL) {
    initialize_default_config(&vm->config);
  } else {
    vm->config = *config;
  }

  vm->nil = hll_new_nil(vm);
  vm->true_ = hll_new_true(vm);

  return vm;
}

void hll_delete_vm(hll_vm *vm) { free(vm); }

hll_interpret_result hll_interpret(hll_vm *vm, char const *source) {
  hll_bytecode *bytecode = hll_compile(vm, source);
  if (bytecode == NULL) {
    return HLL_RESULT_COMPILE_ERROR;
  }

  return hll_interpret_bytecode(vm, bytecode) ? HLL_RESULT_RUNTIME_ERROR
                                              : HLL_RESULT_OK;
}

bool hll_interpret_bytecode(hll_vm *vm, hll_bytecode *bytecode) {
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
      hll_obj *obj = hll_sb_last(stack);

      hll_obj *cons = hll_new_cons(vm, obj, vm->nil);
      if ((*headp)->kind == HLL_OBJ_NIL) {
        *headp = *tailp = cons;
      } else {
        assert((*tailp)->kind == HLL_OBJ_CONS);
        hll_unwrap_cons(*tailp)->cdr = cons;
        *tailp = cons;
      }

      hll_sb_pop(stack);
    } break;
    case HLL_BYTECODE_FIND:
      break;
    case HLL_BYTECODE_CALL:
      break;
    case HLL_BYTECODE_JN:
      break;
    case HLL_BYTECODE_MAKE_LAMBDA:
      break;
    case HLL_BYTECODE_LET:
      break;
    case HLL_BYTECODE_PUSHENV:
      break;
    case HLL_BYTECODE_POPENV:
      break;
    case HLL_BYTECODE_CAR:
      break;
    case HLL_BYTECODE_CDR:
      break;
    case HLL_BYTECODE_SETCAR:
      break;
    case HLL_BYTECODE_SETCDR:
      break;
    default:
      assert(!"Unknown instruction");
      break;
    }
  }

  hll_sb_free(stack);

  return HLL_RESULT_OK;
}
