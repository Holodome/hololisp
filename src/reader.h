#ifndef __READER_H__
#define __READER_H__

#include <stdint.h>

struct hll_lexer;
struct hll_ctx;
struct hll_obj;

/// Reader can be used to produce lisp objects using lexer results.
typedef struct hll_reader {
    struct hll_lexer *lexer;
    struct hll_ctx *ctx;
} hll_reader;

/// Enumeration of result codes of reading.
/// This is the way read results should be handled.
///
/// A litle rationale on exit codes: It could be enought to use just three
/// results: ok, eof, error. Different error codes have little reason to exist
/// since they all will be handled it same way: Abort the program. But error
/// codes are quite handy for testing error reporting so are left as is for now
/// (instead of scary non-feature-pron string comparisons).
typedef enum {
    /// Read succeeded, some object is written in head.
    HLL_READ_OK = 0x0,
    /// EOF, lexer has nothing more to parse.
    HLL_READ_EOF = 0x1,
    /// Lexing failed.
    HLL_READ_LEX_FAILED = 0x2,
    /// Unexpected token (at top level)
    HLL_READ_UNEXPECTED_TOKEN = 0x3,
    /// Missing closing paren in list definition
    HLL_READ_MISSING_RPAREN = 0x4,
    /// Stray dot in list definition
    HLL_READ_STRAY_DOT = 0x5
} hll_read_result;

/// @brief Produces string of given read result
/// @param res Result to give description for
/// @returns String of result description
char const *hll_read_result_str(hll_read_result res);

hll_reader hll_reader_create(struct hll_lexer *lexer, struct hll_ctx *ctx);

hll_read_result hll_read(hll_reader *reader, struct hll_obj **head)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__warn_unused_result__))
#endif
    ;

#endif
