#include "hll_bytecode.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "hll_mem.h"
#include "hll_util.h"
#include "hll_value.h"

static const char *get_op_str(hll_bytecode_op op) {
  static const char *strs[] = {
      "END",  "NIL",  "TRUE",   "CONST",  "APPEND",  "POP",
      "FIND", "CALL", "JN",     "LET",    "PUSHENV", "POPENV",
      "CAR",  "CDR",  "SETCAR", "SETCDR", "MAKEFUN", "DUP",
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
  while ((op = *instruction++) != HLL_BYTECODE_END) {
    fprintf(file, "%4llX:#%-4llX ", (long long unsigned)op_idx,
            (long long unsigned)(instruction - bytecode->ops - 1));
    ++op_idx;

    fprintf(file, "%s", get_op_str(op));
    switch (op) {
    case HLL_BYTECODE_JN: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t offset = ((uint16_t)high) << 8 | low;
      fprintf(file, "%" PRIx16 " (->#%llX)", offset,
              (long long unsigned)(instruction + offset - bytecode->ops));
    } break;
    case HLL_BYTECODE_MAKEFUN: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t idx = ((uint16_t)high) << 8 | low;
      if (idx >= hll_sb_len(bytecode->constant_pool)) {
        fprintf(file, "<err>");
      } else {
        fprintf(file, "%" PRId16 " ", idx);
        hll_dump_value(file, bytecode->constant_pool[idx]);
      }
    } break;
    case HLL_BYTECODE_CONST: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t idx = ((uint16_t)high) << 8 | low;
      if (idx >= hll_sb_len(bytecode->constant_pool)) {
        fprintf(file, "<err>");
      } else {
        fprintf(file, "%" PRId16 " ", idx);
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

size_t hll_get_bytecode_op_body_size(hll_bytecode_op op) {
  size_t s = 0;
  if (op == HLL_BYTECODE_CONST || op == HLL_BYTECODE_MAKEFUN ||
      op == HLL_BYTECODE_JN) {
    s = 2;
  }

  return s;
}

hll_bytecode *hll_new_bytecode(void) { return hll_alloc(sizeof(hll_bytecode)); }

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
    hll_sb_free(bytecode->locs);
    hll_sb_free(bytecode->loc_rle);
    hll_free(bytecode, sizeof(hll_bytecode));
  }
}

size_t hll_bytecode_op_idx(hll_bytecode *bytecode) {
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

void hll_bytecode_add_loc(hll_bytecode *bc, size_t op_length,
                          uint32_t compilation_unit, uint32_t offset,
                          uint32_t length) {
  hll_bytecode_location_entry bc_loc = {
      .translation_unit = compilation_unit,
      .offset = offset,
      .length = length,
  };
  hll_sb_push(bc->locs, bc_loc);

  hll_bytecode_rle rle = {.length = op_length,
                          .loc_idx = hll_sb_len(bc->locs) - 1};
  hll_sb_push(bc->loc_rle, rle);
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
  case HLL_VALUE_MACRO:
    // TODO
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
          "\"bytecode_stats\": { \"ops\": %zu, \"constants\": %zu, \"locs\": "
          "%zu, \"loc_rles\": %zu },\n",
          hll_sb_len(bc->ops), hll_sb_len(bc->constant_pool),
          hll_sb_len(bc->locs), hll_sb_len(bc->loc_rle));

  fprintf(file, "\"ops\": [");
  const uint8_t *instruction = bc->ops;
  size_t op_idx = 0;
  hll_bytecode_op op;
  while ((op = *instruction++)) {
    fprintf(file, "{ \"idx\": %zu, \"offset\": %zu, \"op\": \"%s\"", op_idx++,
            instruction - bc->ops, get_op_str(op));
    switch (op) {
    case HLL_BYTECODE_CONST:
    case HLL_BYTECODE_MAKEFUN:
    case HLL_BYTECODE_JN: {
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

  fprintf(file, "], "); // consts
  fprintf(file, "\"locs\": [");
  for (size_t i = 0; i < hll_sb_len(bc->locs); ++i) {
    hll_bytecode_location_entry *loc = bc->locs + i;
    fprintf(file,
            "{ \"translation_unit\": %" PRIu32 ", \"offset\": %" PRIu32
            ", \"length\": %" PRIu32 "}",
            loc->translation_unit, loc->offset, loc->length);
    if (i != hll_sb_len(bc->locs) - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "], "); // locs

  fprintf(file, "\"loc_rle\": [");
  for (size_t i = 0; i < hll_sb_len(bc->loc_rle); ++i) {
    hll_bytecode_rle *rle = bc->loc_rle + i;
    fprintf(file, "{ \"length\": %" PRIu32 ", \"loc_idx\": %" PRIu32 " }",
            rle->length, rle->loc_idx);
    if (i != hll_sb_len(bc->loc_rle) - 1) {
      fprintf(file, ", ");
    }
  }
  fprintf(file, "]"); // loc_rle
}

void hll_dump_program_info(void *file, hll_value program) {
  hll_dump_value(file, program);
}
