#include "hll_bytecode.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "hll_mem.h"
#include "hll_obj.h"
#include "hll_util.h"

void hll_dump_value(void *file, hll_value value) {
  switch (hll_get_value_kind(value)) {
  case HLL_OBJ_CONS:
    fprintf(file, "cons(");
    hll_dump_value(file, hll_unwrap_car(value));
    fprintf(file, ", ");
    hll_dump_value(file, hll_unwrap_cdr(value));
    fprintf(file, ")");
    break;
  case HLL_OBJ_SYMB:
    fprintf(file, "symb(%s)", hll_unwrap_zsymb(value));
    break;
  case HLL_OBJ_NIL:
    fprintf(file, "nil");
    break;
  case HLL_OBJ_NUM:
    fprintf(file, "num(%f)", hll_unwrap_num(value));
    break;
  case HLL_OBJ_BIND:
    fprintf(file, "bind");
    break;
  case HLL_OBJ_ENV:
    fprintf(file, "env");
    break;
  case HLL_OBJ_TRUE:
    fprintf(file, "true");
    break;
  case HLL_OBJ_FUNC:
    fprintf(file, "func");
    break;
  case HLL_OBJ_MACRO:
    fprintf(file, "macro");
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

void hll_dump_bytecode(void *file, const struct hll_bytecode *bytecode) {
  uint8_t *instruction = bytecode->ops;
  if (instruction == NULL) {
    fprintf(file, "(null)\n");
    return;
  }

  enum hll_bytecode_op op;
  size_t counter = 0;
  while ((op = *instruction++) != HLL_BYTECODE_END) {
    fprintf(file, "%4llX:#%-4llX ", (long long unsigned)counter,
            (long long unsigned)(instruction - bytecode->ops - 1));
    ++counter;
    switch (op) {
    case HLL_BYTECODE_SETCAR:
      fprintf(file, "SETCAR\n");
      break;
    case HLL_BYTECODE_SETCDR:
      fprintf(file, "SETCDR\n");
      break;
    case HLL_BYTECODE_CAR:
      fprintf(file, "CAR\n");
      break;
    case HLL_BYTECODE_CDR:
      fprintf(file, "CDR\n");
      break;
    case HLL_BYTECODE_PUSHENV:
      fprintf(file, "PUSHENV\n");
      break;
    case HLL_BYTECODE_POPENV:
      fprintf(file, "POPENV\n");
      break;
    case HLL_BYTECODE_LET:
      fprintf(file, "LET\n");
      break;
    case HLL_BYTECODE_DUP:
      fprintf(file, "DUP\n");
      break;
    case HLL_BYTECODE_JN: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t offset = ((uint16_t)high) << 8 | low;
      fprintf(file, "JN %" PRIx16 " (->#%llX)\n", offset,
              (long long unsigned)(instruction + offset - bytecode->ops));
    } break;
    case HLL_BYTECODE_FIND:
      fprintf(file, "FIND\n");
      break;
    case HLL_BYTECODE_NIL:
      fprintf(file, "NIL\n");
      break;
    case HLL_BYTECODE_TRUE:
      fprintf(file, "TRUE\n");
      break;
    case HLL_BYTECODE_MAKEFUN: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t idx = ((uint16_t)high) << 8 | low;
      if (idx >= hll_sb_len(bytecode->constant_pool)) {
        fprintf(file, "MAKEFUN <err>\n");
      } else {
        fprintf(file, "MAKEFUN %" PRId16 " ", idx);
        hll_dump_value(file, bytecode->constant_pool[idx]);
        fprintf(file, "\n");
      }
    } break;
    case HLL_BYTECODE_CONST: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t idx = ((uint16_t)high) << 8 | low;
      if (idx >= hll_sb_len(bytecode->constant_pool)) {
        fprintf(file, "CONST <err>\n");
      } else {
        fprintf(file, "CONST %" PRId16 " ", idx);
        hll_dump_value(file, bytecode->constant_pool[idx]);
        fprintf(file, "\n");
      }
    } break;
    case HLL_BYTECODE_APPEND:
      fprintf(file, "APPEND\n");
      break;
    case HLL_BYTECODE_CALL:
      fprintf(file, "CALL\n");
      break;
    case HLL_BYTECODE_POP:
      fprintf(file, "POP\n");
      break;
    default:
      fprintf(file, "<err>%hhx\n", (char unsigned)op);
      break;
    }
  }

  fprintf(file, "%4llX:#%-4llX END\n", (long long unsigned)counter,
          (long long unsigned)(instruction - bytecode->ops - 1));
}

size_t hll_get_bytecode_op_body_size(enum hll_bytecode_op op) {
  size_t s = 0;
  if (op == HLL_BYTECODE_CONST || op == HLL_BYTECODE_MAKEFUN ||
      op == HLL_BYTECODE_JN) {
    s = 2;
  }

  return s;
}

struct hll_bytecode *hll_new_bytecode(void) {
  return hll_alloc(sizeof(struct hll_bytecode));
}

void hll_bytecode_inc_refcount(struct hll_bytecode *bytecode) {
  assert(bytecode->refcount != UINT32_MAX);
  ++bytecode->refcount;
}

void hll_bytecode_dec_refcount(struct hll_bytecode *bytecode) {
  assert(bytecode->refcount != 0);
  --bytecode->refcount;

  if (bytecode->refcount == 0) {
    hll_sb_free(bytecode->ops);
    hll_sb_free(bytecode->constant_pool);
    hll_free(bytecode, sizeof(struct hll_bytecode));
  }
}

size_t hll_bytecode_op_idx(struct hll_bytecode *bytecode) {
  return hll_sb_len(bytecode->ops);
}

size_t hll_bytecode_emit_u8(struct hll_bytecode *bytecode, uint8_t byte) {
  size_t idx = hll_bytecode_op_idx(bytecode);
  hll_sb_push(bytecode->ops, byte);
  return idx;
}

size_t hll_bytecode_emit_u16(struct hll_bytecode *bytecode, uint16_t value) {
  size_t idx = hll_bytecode_emit_u8(bytecode, (value >> 8) & 0xFF);
  hll_bytecode_emit_u8(bytecode, value & 0xFF);
  return idx;
}

size_t hll_bytecode_emit_op(struct hll_bytecode *bytecode,
                            enum hll_bytecode_op op) {
  assert(op <= 0xFF);
  return hll_bytecode_emit_u8(bytecode, op);
}
