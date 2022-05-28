#ifndef __HLL_LEXER_H__
#define __HLL_LEXER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Lisp lexer token kind. */
typedef enum {
    HLL_LTOK_EOF    = 0x0, /** End of file */
    HLL_LTOK_NUMI   = 0x1, /** Integer */
    HLL_LTOK_SYMB   = 0x2, /** lisp symbol */
    HLL_LTOK_DOT    = 0x3, /** . */
    HLL_LTOK_LPAREN = 0x4, /** ( */
    HLL_LTOK_RPAREN = 0x5, /** ) */
    HLL_LTOK_NUMR   = 0x6, /** Real number */
    HLL_LTOK_STR    = 0x7, /** String literal (UTF8) */
    HLL_LTOK_QUOTE  = 0x8, /** ' */
    HLL_LTOK_OTHER  = 0x9  /** Unrecognized token */
} hll_ltoken_kind;

/** Structure storing data related to perfoming lexical analysis on lisp code.
 */
typedef struct hll_lexer {
    /** Current parsing location. Buffer has to be zero-teminated. */
    char const *cursor;
    /** Line and character information, characters are UTF8 lexems. */
    uint32_t line, column;
    /** Buffer, must be supplied on structure creation. Are used for writing
     * strings when encountered during lexing. It is possiblle to perform
     * parsing without saving strings. */
    char *buffer;
    /** Size of buffer defined earlier. When overflowed, report error. */
    uint32_t buffer_size;
    /** Token info */
    hll_ltoken_kind token_kind;
    int64_t         token_int;
    uint32_t        token_length;
    double          token_real;
    char const     *token_start;
    /**
     * This is hacky way to implement peeking and returning same token for
     * multiple times. Ideally we should implement wrapper structure, like
     * token_iterator that should support peeking using stack or something, but
     * this is too complicated and unnecessary.
     */
    int should_return_old;
    /** Flag that we should always return eof after buffer is fully parsed. */
    int already_met_eof;
} hll_lexer;

/** Result of lexing. This return code can either be ignored or used to report
 * error. */
typedef enum {
    HLL_LEX_OK                  = 0x0, /**< OK */
    HLL_LEX_BUF_OVERFLOW        = 0x1, /**< String buffer overflow */
    HLL_LEX_UTF8_ERROR          = 0x2, /**< UTF8 decoding error */
    HLL_LEX_UNTERMINATED_STRING = 0x3  /**< Unterminated string literal */
} hll_lex_result;

/** Call to initialize the lexer. */
hll_lexer hll_lexer_create(char const *cursor, char *buf, uint32_t buffer_size);

/** Generate token */
hll_lex_result hll_lexer_peek(hll_lexer *lexer);

void hll_lexer_eat(hll_lexer *lexer);

hll_lex_result hll_lexer_eat_peek(hll_lexer *lexer);

#ifdef __cplusplus
}
#endif

#endif
