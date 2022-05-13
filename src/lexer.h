#ifndef __HLL_LEXER_H__
#define __HLL_LEXER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lisp lexer token kind. */
typedef enum {
    HLL_TOK_EOF    = 0x0, /* End of file */
    HLL_TOK_NUMI   = 0x1, /* Integer */
    HLL_TOK_SYMB   = 0x2, /* lisp symbol */
    HLL_TOK_DOT    = 0x3, /* . */ 
    HLL_TOK_LPAREN = 0x4, /* ( */ 
    HLL_TOK_RPAREN = 0x5, /* ) */
    HLL_TOK_NUMR   = 0x6, /* Real number */
    HLL_TOK_STR    = 0x7, /* String literal (UTF8) */
} hll_ltoken_kind;

/* Structure storing data related to perfoming lexical analysis on lisp code. */
typedef struct hll_lexer {
    /* Current parsing location */
    char const *cursor;
    /* End of file. We don't force buffers to be zero-terminated. */
    char const *eof;
    /* Line and character information, characters are UTF8 lexems. */
    uint32_t line, column;
    /* Buffer, must be supplied on structure creation. Are used for writing strings when encountered during lexing. */
    char *buffer;
    /* Size of buffer defined earlier. When overflowed, report error. */ 
    uint32_t buffer_size;

} hll_lexer;

#ifdef __cplusplus
}
#endif

#endif
