#ifndef __HLLVM_OPCODES_H__
#define __HLLVM_OPCODES_H__

#include <stddef.h>
#include <stdint.h>

typedef enum {
    // Mark end of the program
    HLLVM_OP_END,
    // Load constant on the stack
    HLLVM_OP_CONST,
    // Loads nil on the stack
    HLLVM_OP_NIL,
    // Loads true on the stack
    HLLVM_OP_TRUE,
    // Makes cons out of car and cdr on stack
    HLLVM_OP_CONS,

    HLLVM_OP_PUSH,
    HLLVM_OP_POP,
    HLLVM_OP_CALL
} hllvm_opcode;

typedef struct {
    size_t size;
    size_t capacity;
    void *bytecode;
} hllvm_bytecode_chunk;

void hllvm_chunk_write(hllvm_bytecode_chunk *chunk, uint8_t byte);

#endif
