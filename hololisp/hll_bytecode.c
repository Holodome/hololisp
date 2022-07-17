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
        fprintf(file, "%4llX:#%-4llX ", (long long unsigned)counter,
                (long long unsigned)(instruction - bytecode->ops - 1));
        ++counter;
        switch (op) {
        case HLL_BYTECODE_SETCAR: fprintf(file, "SETCAR\n"); break;
        case HLL_BYTECODE_SETCDR: fprintf(file, "SETCDR\n"); break;
        case HLL_BYTECODE_CAR: fprintf(file, "CAR\n"); break;
        case HLL_BYTECODE_CDR: fprintf(file, "CDR\n"); break;
        case HLL_BYTECODE_PUSHENV: fprintf(file, "PUSHENV\n"); break;
        case HLL_BYTECODE_POPENV: fprintf(file, "POPENV\n"); break;
        case HLL_BYTECODE_LET: fprintf(file, "LET\n"); break;
        case HLL_BYTECODE_MAKE_LAMBDA: fprintf(file, "MAKELAMBDA\n"); break;
        case HLL_BYTECODE_JN: {
            uint8_t high = *instruction++;
            uint8_t low = *instruction++;
            uint16_t offset = ((uint16_t)high) << 8 | low;
            fprintf(
                file, "JN %" PRIx16 " (->#%llX)\n", offset,
                (long long unsigned)(instruction + offset - bytecode->ops - 2));
        } break;
        case HLL_BYTECODE_FIND: fprintf(file, "FIND\n"); break;
        case HLL_BYTECODE_NIL: fprintf(file, "NIL\n"); break;
        case HLL_BYTECODE_TRUE: fprintf(file, "TRUE\n"); break;
        case HLL_BYTECODE_CONST: {
            uint8_t high = *instruction++;
            uint8_t low = *instruction++;
            uint16_t idx = ((uint16_t)high) << 8 | low;
            if (idx > +hll_sb_len(bytecode->constant_pool)) {
                fprintf(file, "CONST <err>\n");
            } else {
                fprintf(file, "CONST %" PRId64 " (%" PRId16 ")\n",
                        bytecode->constant_pool[idx], idx);
            }
        } break;
        case HLL_BYTECODE_SYMB: {
            uint8_t high = *instruction++;
            uint8_t low = *instruction++;
            uint16_t idx = ((uint16_t)high) << 8 | low;
            if (idx > +hll_sb_len(bytecode->symbol_pool)) {
                fprintf(file, "SYMB <err>\n");
            } else {
                fprintf(file, "SYMB %s (%" PRId16 ")\n",
                        bytecode->symbol_pool[idx], idx);
            }
        } break;
        case HLL_BYTECODE_APPEND: fprintf(file, "APPEND\n"); break;
        case HLL_BYTECODE_CALL: fprintf(file, "CALL\n"); break;
        case HLL_BYTECODE_POP: fprintf(file, "POP\n"); break;
        default: fprintf(file, "<err>%hhx\n", (char unsigned)op); break;
        }
    }

    fprintf(file, "%4llX:#%-4llX END\n", (long long unsigned)counter,
            (long long unsigned)(instruction - bytecode->ops - 1));
}
