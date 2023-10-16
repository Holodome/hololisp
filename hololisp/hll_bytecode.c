#include "hll_bytecode.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "hll_mem.h"
#include "hll_value.h"

static const char *get_op_str(hll_bytecode_op op) {
  static const char *strs[] = {
      "END",    "NIL",  "TRUE",     "CONST",  "APPEND", "POP",
      "FIND",   "CALL", "MBTRCALL", "JN",     "LET",    "PUSHENV",
      "POPENV", "CAR",  "CDR",      "SETCAR", "SETCDR", "MAKEFUN",
  };

  assert(op < sizeof(strs) / sizeof(strs[0]));
  return strs[op];
}

void hll_dump_bytecode(void *file, const hll_bytecode *bytecode) {
  uint8_t *instruction = bytecode->ops;
  if (instruction == NULL) {
    fprintf(file, "(null)\n");
    return;
  }

  hll_bytecode_op op;
  size_t op_idx = 0;
  while ((op = *instruction++) != HLL_BC_END) {
    fprintf(file, "%4llX:#%-4llX ", (long long unsigned)op_idx,
            (long long unsigned)(instruction - bytecode->ops - 1));
    ++op_idx;

    fprintf(file, "%s", get_op_str(op));
    switch (op) {
    case HLL_BC_JN: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t offset = ((uint16_t)high) << 8 | low;
      fprintf(file, " 0x%" PRIx16 " (->#%llX)", offset,
              (long long unsigned)(instruction + offset - bytecode->ops));
    } break;
    case HLL_BC_MAKEFUN: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t idx = ((uint16_t)high) << 8 | low;
      if (idx >= hll_sb_len(bytecode->constant_pool)) {
        fprintf(file, "<err>");
      } else {
        fprintf(file, " 0x%" PRIx16 " ", idx);
        hll_dump_value(file, bytecode->constant_pool[idx]);
      }
    } break;
    case HLL_BC_CONST: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t idx = ((uint16_t)high) << 8 | low;
      if (idx >= hll_sb_len(bytecode->constant_pool)) {
        fprintf(file, "<err>");
      } else {
        fprintf(file, " 0x%" PRIx16 " ", idx);
        hll_dump_value(file, bytecode->constant_pool[idx]);
      }
    } break;
    default:
      break;
    }
    fprintf(file, "\n");
  }

  fprintf(file, "%4llX:#%-4llX END\n", (long long unsigned)op_idx,
          (long long unsigned)(instruction - bytecode->ops - 1));
}

size_t hll_bytecode_op_body_size(hll_bytecode_op op) {
  size_t s = 0;
  if (op == HLL_BC_CONST || op == HLL_BC_MAKEFUN || op == HLL_BC_JN) {
    s = 2;
  }

  return s;
}

hll_bytecode *hll_new_bytecode(hll_value name) {
  hll_bytecode *bc = hll_alloc(sizeof(hll_bytecode));
  bc->name = name;
  return bc;
}

void hll_bytecode_inc_refcount(hll_bytecode *bytecode) {
  assert(bytecode->refcount != UINT32_MAX);
  ++bytecode->refcount;
}

void hll_bytecode_dec_refcount(hll_bytecode *bytecode) {
  assert(bytecode->refcount != 0);
  --bytecode->refcount;

  if (bytecode->refcount == 0) {
    hll_sb_free(bytecode->ops);
    hll_sb_free(bytecode->constant_pool);
    hll_free(bytecode, sizeof(hll_bytecode));
  }
}

size_t hll_bytecode_op_idx(const hll_bytecode *bytecode) {
  return hll_sb_len(bytecode->ops);
}

size_t hll_bytecode_emit_u8(hll_bytecode *bytecode, uint8_t byte) {
  size_t idx = hll_bytecode_op_idx(bytecode);
  hll_sb_push(bytecode->ops, byte);
  return idx;
}

size_t hll_bytecode_emit_u16(hll_bytecode *bytecode, uint16_t value) {
  size_t idx = hll_bytecode_emit_u8(bytecode, (value >> 8) & 0xFF);
  hll_bytecode_emit_u8(bytecode, value & 0xFF);
  return idx;
}

size_t hll_bytecode_emit_op(hll_bytecode *bytecode, hll_bytecode_op op) {
  assert(op <= 0xFF);
  return hll_bytecode_emit_u8(bytecode, op);
}

void make_ident(void *file, size_t ident) {
  while (ident--) {
    putc(' ', file);
  }
}

void dump_function_info(void *file, hll_value value);

void hll_dump_value(void *file, hll_value value) {
  hll_value_kind kind = hll_get_value_kind(value);
  fprintf(file, "{ \"kind\": \"%s\"", hll_get_value_kind_str(kind));
  switch (kind) {
  case HLL_VALUE_NIL:
  case HLL_VALUE_TRUE:
    break;
  case HLL_VALUE_NUM:
    fprintf(file, ", \"value\": %lf", hll_unwrap_num(value));
    break;
  case HLL_VALUE_CONS:
    fprintf(file, ", \"car\": ");
    hll_dump_value(file, hll_unwrap_car(value));
    fprintf(file, ", \"cdr\": ");
    hll_dump_value(file, hll_unwrap_cdr(value));
    break;
  case HLL_VALUE_SYMB:
    fprintf(file, ", \"symb\": \"%s\"", hll_unwrap_zsymb(value));
    break;
  case HLL_VALUE_BIND:
    // TODO
    break;
  case HLL_VALUE_ENV:
    // TODO
    break;
  case HLL_VALUE_FUNC:
    fprintf(file, ", \"func\": {");
    dump_function_info(file, value);
    fprintf(file, "}");
    break;
  }
  fprintf(file, "}");
}

void dump_function_info(void *file, hll_value value) {
  hll_obj_func *func = hll_unwrap_func(value);
  fprintf(file, "\"params\": [");
  for (hll_value param = func->param_names; hll_is_cons(param);
       param = hll_unwrap_cdr(param)) {
    hll_dump_value(file, hll_unwrap_car(param));
    if (!hll_is_nil(hll_unwrap_cdr(param))) {
      fprintf(file, ",");
    }
  }
  fprintf(file, "],"); // params

  hll_bytecode *bc = func->bytecode;
  fprintf(file,
          "\"bytecode_stats\": { \"ops\": %zu, \"constants\": %zu}, ",
          hll_sb_len(bc->ops), hll_sb_len(bc->constant_pool));

  fprintf(file, "\"ops\": [");
  const uint8_t *instruction = bc->ops;
  hll_bytecode_op op;
  while ((op = *instruction++)) {
    fprintf(file, "{ \"offset\": %zu, \"op\": \"%s\"",
            instruction - bc->ops, get_op_str(op));
    switch (op) {
    case HLL_BC_CONST:
    case HLL_BC_MAKEFUN:
    case HLL_BC_JN: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t offset = ((uint16_t)high) << 8 | low;
      fprintf(file, ", \"value\": %" PRId16, offset);
    } break;
    default:
      break;
    }
    fprintf(file, "}"); // op
    if (*instruction) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "], "); // ops

  fprintf(file, "\"consts\": [");
  for (size_t i = 0; i < hll_sb_len(bc->constant_pool); ++i) {
    hll_value v = bc->constant_pool[i];
    hll_dump_value(file, v);
    if (i != hll_sb_len(bc->constant_pool) - 1) {
      fprintf(file, ", ");
    }
  }

  fprintf(file, "]"); // consts
}

void hll_dump_program_info(void *file, hll_value program) {
  hll_dump_value(file, program);
}

static void investigate_call_op(hll_bytecode *bytecode, size_t call_idx) {
  assert(bytecode->ops[call_idx] == HLL_BC_CALL);
  uint8_t *cursor = bytecode->ops + call_idx + 1;
  uint8_t *end = &hll_sb_last(bytecode->ops) + 1;
  while (cursor < end) {
    hll_bytecode_op op = *cursor;
    switch (op) {
    case HLL_BC_NIL:
      // unconditional jump like seen in if's
      if (cursor[1] == HLL_BC_JN) {
        cursor += 2;
        uint8_t high = *cursor++;
        uint8_t low = *cursor++;
        uint16_t offset = ((uint16_t)high) << 8 | low;
        cursor += offset;
        continue;
      }
      // fallthrough
    case HLL_BC_TRUE:
    case HLL_BC_CONST:
    case HLL_BC_APPEND:
    case HLL_BC_POP:
    case HLL_BC_FIND:
    case HLL_BC_CALL:
    case HLL_BC_MBTRCALL:
    case HLL_BC_JN:
    case HLL_BC_LET:
    case HLL_BC_PUSHENV:
    case HLL_BC_CAR:
    case HLL_BC_CDR:
    case HLL_BC_SETCAR:
    case HLL_BC_SETCDR:
    case HLL_BC_MAKEFUN:
      return;
    case HLL_BC_END:
    case HLL_BC_POPENV:
      ++cursor;
      break;
    }
  }

  bytecode->ops[call_idx] = HLL_BC_MBTRCALL;
}

static void mark_tail_calls(hll_bytecode *bytecode) {
  size_t len = hll_sb_len(bytecode->ops);
  for (size_t i = 0; i < len; ++i) {
    hll_bytecode_op op = bytecode->ops[i];
    if (op == HLL_BC_CALL) {
      investigate_call_op(bytecode, i);
    }
    i += hll_bytecode_op_body_size(op);
  }
}

void hll_optimize_bytecode(hll_bytecode *bytecode) {
  if (!hll_is_nil(bytecode->name)) {
    mark_tail_calls(bytecode);
  }
}
