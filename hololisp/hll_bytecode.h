#ifndef HLL_BYTECODE_H
#define HLL_BYTECODE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    /// Bytecode must be terminated with 0.
    HLL_BYTECODE_END = 0x0,

    HLL_BYTECODE_NIL,
    HLL_BYTECODE_TRUE,
    HLL_BYTECODE_CONST,
    HLL_BYTECODE_MAKE_CONS,

    HLL_BYTECODE_SYMB,

    HLL_BYTECODE_SETCAR,
    HLL_BYTECODE_SETCDR,
    HLL_BYTECODE_CALL,

    HLL_BYTECODE_CAR,
    HLL_BYTECODE_CDR,
    HLL_BYTECODE_EVAL,
    HLL_BYTECODE_POP,
} hll_bytecode_op;

typedef struct hll_bytecode {
    uint8_t *ops;
    int64_t *constant_pool;
    char const **symbol_pool;
} hll_bytecode;

void hll_dump_bytecode(void *file, hll_bytecode *bytecode);

#endif
