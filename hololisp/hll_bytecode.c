#include "hll_bytecode.h"

#include <inttypes.h>
#include <stdio.h>

#include "hll_mem.h"

void
hll_dump_bytecode(void *file, hll_bytecode *bytecode) {
    uint8_t *instruction = bytecode->ops;
    if (instruction == NULL) {
        fprintf(file, "(null)\n");
        return;
    }

    hll_bytecode_op op;
    size_t counter = 0;
    while ((op = *instruction++) != HLL_BYTECODE_END) {
        fprintf(file, "%-4llX", (long long unsigned)counter++);
        switch (op) {
        case HLL_BYTECODE_NIL: fprintf(file, "NIL\n"); break;
        case HLL_BYTECODE_TRUE: fprintf(file, "TRUE\n"); break;
        case HLL_BYTECODE_CONST: {
            uint8_t high = *instruction++;
            uint8_t low = *instruction++;
            uint16_t idx = ((uint16_t)high) << 8 | low;
            if (idx > +hll_sb_len(bytecode->constant_pool)) {
                fprintf(file, "CONST <err>\n");
            } else {
                fprintf(file, "CONST %" PRId64 "\n",
                        bytecode->constant_pool[idx]);
            }
        } break;
        case HLL_BYTECODE_SYMB: {
            uint8_t high = *instruction++;
            uint8_t low = *instruction++;
            uint16_t idx = ((uint16_t)high) << 8 | low;
            if (idx > +hll_sb_len(bytecode->symbol_pool)) {
                fprintf(file, "SYMB <err>\n");
            } else {
                fprintf(file, "SYMB %s\n",
                        bytecode->symbol_pool[idx]);
            }
        } break;
        case HLL_BYTECODE_MAKE_CONS: fprintf(file, "MAKECONS\n"); break;
        case HLL_BYTECODE_SETCAR: fprintf(file, "SETCAR\n"); break;
        case HLL_BYTECODE_SETCDR: fprintf(file, "SETCDR\n"); break;
        case HLL_BYTECODE_CALL: fprintf(file, "CALL\n"); break;
        case HLL_BYTECODE_CAR: fprintf(file, "CAR\n"); break;
        case HLL_BYTECODE_CDR: fprintf(file, "CDR\n"); break;
        case HLL_BYTECODE_EVAL: fprintf(file, "EVAL\n"); break;
        case HLL_BYTECODE_POP: fprintf(file, "POP\n"); break;
        default: fprintf(file, "<err>%hhx\n", (char unsigned)op); break;
        }
    }
}
