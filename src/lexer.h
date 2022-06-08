#ifndef __HLL_LEXER_H__
#define __HLL_LEXER_H__

#include <stdint.h>

/** Lisp lexer token kind. */
typedef enum {
    HLL_LTOK_EOF = 0x0,    /**< End of file */
    HLL_LTOK_NUMI = 0x1,   /**< Integer */
    HLL_LTOK_SYMB = 0x2,   /**< lisp symbol */
    HLL_LTOK_DOT = 0x3,    /**< . */
    HLL_LTOK_LPAREN = 0x4, /**< ( */
    HLL_LTOK_RPAREN = 0x5, /**< ) */
    HLL_LTOK_QUOTE = 0x6,  /**< ' */
} hll_ltoken_kind;

/** Structure storing data related to performing lexical analysis on lisp code.
 */
typedef struct hll_lexer {
    /** Current parsing location. Buffer has to be zero-terminated. */
    char const *cursor;
    /** Line and character information, characters are UTF8 lexemes. */
    uint32_t line, column;
    /** Buffer, must be supplied on structure creation. Are used for writing
     * strings when encountered during lexing. It is possible to perform
     * parsing without saving strings. */
    char *buffer;
    /** Size of buffer defined earlier. When overflowed, report error. */
    uint32_t buffer_size;
    /** Token info */
    hll_ltoken_kind token_kind;
    int64_t token_int;
    uint32_t token_length;
    char const *token_start;
    /**
     * This is hacky way to implement peeking and returning same token for
     * multiple times. Ideally we should implement wrapper structure, like
     * token_iterator that should support peeking using stack or something, but
     * this is too complicated and unnecessary.
     */
    int should_return_old;
    /** Flag that we should always return eof after buffer is fully parsed. */
    int already_met_eof;
    char const *line_start;
} hll_lexer;

/** Result of lexing. This return code can either be ignored or used to report
 * error. */
typedef enum {
    HLL_LEX_OK = 0x0,           /**< OK */
    HLL_LEX_BUF_OVERFLOW = 0x1, /**< String buffer overflow. Note that this is
                                   not reported if buffer is NULL. */
    /// Symbol consisting entirely of dots (more than one). Common lisp
    /// considers such tokens invalid.
    HLL_LEX_ALL_DOT_SYMB = 0x2,
} hll_lex_result;

/** Call to initialize the lexer. */
hll_lexer hll_lexer_create(char const *cursor, char *buf, uint32_t buffer_size);

/** Generate token */
hll_lex_result hll_lexer_peek(hll_lexer *lexer)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__warn_unused_result__))
#endif
    ;

void hll_lexer_eat(hll_lexer *lexer);

hll_lex_result hll_lexer_eat_peek(hll_lexer *lexer)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__warn_unused_result__))
#endif
    ;

#endif
