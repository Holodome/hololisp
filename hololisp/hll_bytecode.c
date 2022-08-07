#include "hll_bytecode.h"

#include <inttypes.h>
#include <stdio.h>

#include "hll_mem.h"
#include "hll_obj.h"

static void dump_object(void *file, hll_obj *obj) {
  switch (obj->kind) {
  case HLL_OBJ_CONS:
    fprintf(file, "cons(");
    dump_object(file, ((hll_obj_cons *)obj->as.body)->car);
    fprintf(file, ", ");
    dump_object(file, ((hll_obj_cons *)obj->as.body)->cdr);
    fprintf(file, ")");
    break;
  case HLL_OBJ_SYMB:
    fprintf(file, "symb(%s)", ((hll_obj_symb *)obj->as.body)->symb);
    break;
  case HLL_OBJ_NIL:
    fprintf(file, "nil");
    break;
  case HLL_OBJ_NUM:
    fprintf(file, "num(%f)", obj->as.num);
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
  }
}

void hll_dump_bytecode(void *file, const hll_bytecode *bytecode) {
  uint8_t *instruction = bytecode->ops;
  if (instruction == NULL) {
    fprintf(file, "(null)\n");
    return;
  }

  hll_bytecode_op op;
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
    case HLL_BYTECODE_JN: {
      uint8_t high = *instruction++;
      uint8_t low = *instruction++;
      uint16_t offset = ((uint16_t)high) << 8 | low;
      fprintf(file, "JN %" PRIx16 " (->#%llX)\n", offset,
              (long long unsigned)(instruction + offset - bytecode->ops - 2));
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
        dump_object(file, bytecode->constant_pool[idx]);
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
        dump_object(file, bytecode->constant_pool[idx]);
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
