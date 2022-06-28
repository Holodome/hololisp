#ifndef __HLL_LEXER_H__
#define __HLL_LEXER_H__

#include <stdint.h>

/// Result of lexing. This return code can either be ignored or used to report
/// error.
typedef enum {
    /// Ok.
    HLL_LEX_OK = 0x0,
    /// Buffer overflow for strings. Typically this should not happen,
    /// but it is better to have check than assume.
    /// If string buffer supplied to lexer is NULL this is not reported
    HLL_LEX_BUF_OVERFLOW = 0x1,
    /// Symbol consisting entirely of dots (more than one). Common lisp
    /// considers such tokens invalid.
    HLL_LEX_ALL_DOT_SYMB = 0x2,
    /// Integer overflow happened.
    /// TODO: The way we check for this is a bit janky, but it definetely works
    /// for really big numbers, but some close to boundary numbers may not be
    /// handled correctly.
    HLL_LEX_INT_TOO_BIG = 0x3,
} hll_lex_result;

/// Lisp lexer token kind.
typedef enum {
    /// End of file
    HLL_TOK_EOF = 0x0,
    /// Integer
    HLL_TOK_NUMI = 0x1,
    /// Symbol
    HLL_TOK_SYMB = 0x2,
    /// .
    HLL_TOK_DOT = 0x3,
    /// (
    HLL_TOK_LPAREN = 0x4,
    /// )
    HLL_TOK_RPAREN = 0x5,
    /// '
    HLL_TOK_QUOTE = 0x6,
} hll_token_kind;

/// Structure storing data related to performing lexical analysis on lisp code.
typedef struct hll_lexer {
    /// Current parsing location. Buffer has to be zero-terminated.
    char const *cursor;
    /// Line and character information, characters are UTF8 lexemes.
    uint32_t line;
    /// Buffer, must be supplied on structure creation. Are used for writing
    /// strings when encountered during lexing. It is possible to perform
    /// parsing without saving strings.
    char *buffer;
    /// Size of buffer defined earlier. When overflowed, report error.
    uint32_t buffer_size;
    /// Token info
    hll_token_kind token_kind;
    int64_t token_int;
    uint32_t token_length;
    uint32_t token_line;
    uint32_t token_column;
    /// This is hacky way to implement peeking and returning same token for
    /// multiple times. Ideally we should implement wrapper structure, like
    /// token_iterator that should support peeking using stack or something, but
    /// this is too complicated and unnecessary.
    int should_return_old;
    hll_lex_result old_result;
    /// Flag that we should always return eof after buffer is fully parsed.
    int already_met_eof;
    char const *line_start;
} hll_lexer;

char const *hll_lex_result_str(hll_lex_result result);

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
