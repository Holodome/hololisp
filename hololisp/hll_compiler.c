#include "hll_compiler.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hll_hololisp.h"
#include "hll_mem.h"
#include "hll_vm.h"

void *
hll_compile(struct hll_vm *vm, char const *source) {
    hll_memory_arena compilation_arena = { 0 };

    hll_lexer lexer;
    hll_lexer_init(&lexer, source, vm);

    hll_reader reader = { 0 };
    reader.vm = vm;
    reader.error_fn = vm->config.error_fn;
    reader.lexer = &lexer;
    reader.arena = &compilation_arena;

    hll_ast *ast = hll_read_ast(&reader);

    hll_compiler compiler = { 0 };

    void *result = hll_compile_ast(&compiler, ast);

    hll_memory_arena_clear(&compilation_arena);

    return result;
}

typedef enum {
    _HLL_LEX_EQC_OTHER = 0x0,  // Anything not handled
    _HLL_LEX_EQC_NUMBER,       // 0-9
    _HLL_LEX_EQC_LPAREN,       // (
    _HLL_LEX_EQC_RPAREN,       // )
    _HLL_LEX_EQC_QUOTE,        // '
    _HLL_LEX_EQC_DOT,          // .
    _HLL_LEX_EQC_SIGNS,        // +-
    _HLL_LEX_EQC_SYMB,  // All other characters that can be part of symbol
    _HLL_LEX_EQC_EOF,
    _HLL_LEX_EQC_SPACE,
    _HLL_LEX_EQC_NEWLINE,
    _HLL_LEX_EQC_COMMENT,  // ;
} _hll_lexer_equivalence_class;

static inline _hll_lexer_equivalence_class
_get_equivalence_class(char symb) {
    _hll_lexer_equivalence_class eqc = _HLL_LEX_EQC_OTHER;
    // clang-format off
    switch (symb) {
    case '(': eqc = _HLL_LEX_EQC_LPAREN; break;
    case ')': eqc = _HLL_LEX_EQC_RPAREN; break;
    case '\'': eqc = _HLL_LEX_EQC_QUOTE; break;
    case '.': eqc = _HLL_LEX_EQC_DOT; break;
    case '\0': eqc = _HLL_LEX_EQC_EOF; break;
    case '1': case '2': case '3':
    case '4': case '5': case '6':
    case '7': case '8': case '9':
    case '0':
        eqc = _HLL_LEX_EQC_NUMBER; break;
    case '-': case '+':
        eqc = _HLL_LEX_EQC_SIGNS; break;
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
    case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
    case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
    case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B':
    case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I':
    case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W':
    case 'X': case 'Y': case 'Z': case '*': case '/': case '@': case '$':
    case '%': case '^': case '&': case '_': case '=': case '<': case '>':
    case '~': case '?': case '!': case '[': case ']': case '{': case '}':
        eqc = _HLL_LEX_EQC_SYMB; break;
    case ' ': case '\t': case '\f': case '\r': case '\v':
        eqc = _HLL_LEX_EQC_SPACE; break;
    case '\n':
        eqc = _HLL_LEX_EQC_NEWLINE; break;
    case ';':
        eqc = _HLL_LEX_EQC_COMMENT; break;
    }

    // clang-format on
    return eqc;
}

typedef enum {
    _HLL_LEX_START,
    _HLL_LEX_COMMENT,
    _HLL_LEX_NUMBER,
    _HLL_LEX_SEEN_SIGN,
    _HLL_LEX_DOTS,
    _HLL_LEX_SYMB,
    _HLL_LEX_UNEXPECTED,

    _HLL_LEX_FIN,
    _HLL_LEX_FIN_LPAREN,
    _HLL_LEX_FIN_RPAREN,
    _HLL_LEX_FIN_QUOTE,
    _HLL_LEX_FIN_NUMBER,
    _HLL_LEX_FIN_DOTS,
    _HLL_LEX_FIN_SYMB,
    _HLL_LEX_FIN_EOF,
    _HLL_LEX_FIN_UNEXPECTED,
} _hll_lexer_state;

static inline _hll_lexer_state
_get_next_state(_hll_lexer_state state, _hll_lexer_equivalence_class eqc) {
    switch (state) {
    default: assert(0); break;
    case _HLL_LEX_START:
        switch (eqc) {
        case _HLL_LEX_EQC_OTHER: state = _HLL_LEX_UNEXPECTED; break;
        case _HLL_LEX_EQC_NUMBER: state = _HLL_LEX_NUMBER; break;
        case _HLL_LEX_EQC_LPAREN: state = _HLL_LEX_FIN_LPAREN; break;
        case _HLL_LEX_EQC_RPAREN: state = _HLL_LEX_FIN_RPAREN; break;
        case _HLL_LEX_EQC_QUOTE: state = _HLL_LEX_FIN_QUOTE; break;
        case _HLL_LEX_EQC_DOT: state = _HLL_LEX_DOTS; break;
        case _HLL_LEX_EQC_SYMB: state = _HLL_LEX_SYMB; break;
        case _HLL_LEX_EQC_SPACE: /* nop */ break;
        case _HLL_LEX_EQC_NEWLINE: /* nop */ break;
        case _HLL_LEX_EQC_EOF: state = _HLL_LEX_FIN_EOF; break;
        case _HLL_LEX_EQC_SIGNS: state = _HLL_LEX_SEEN_SIGN; break;
        case _HLL_LEX_EQC_COMMENT: state = _HLL_LEX_COMMENT; break;
        }
        break;
    case _HLL_LEX_COMMENT:
        switch (eqc) {
        default: /* nop */ break;
        case _HLL_LEX_EQC_EOF: state = _HLL_LEX_FIN_EOF; break;
        case _HLL_LEX_EQC_NEWLINE: state = _HLL_LEX_START; break;
        }
        break;
    case _HLL_LEX_SEEN_SIGN:
        switch (eqc) {
        default: state = _HLL_LEX_FIN_SYMB; break;
        case _HLL_LEX_EQC_NUMBER: state = _HLL_LEX_NUMBER; break;
        case _HLL_LEX_EQC_SIGNS:
        case _HLL_LEX_EQC_DOT:
        case _HLL_LEX_EQC_SYMB: state = _HLL_LEX_SYMB; break;
        }
        break;
    case _HLL_LEX_NUMBER:
        switch (eqc) {
        default: state = _HLL_LEX_FIN_NUMBER; break;
        case _HLL_LEX_EQC_NUMBER: /* nop */ break;
        case _HLL_LEX_EQC_SIGNS:
        case _HLL_LEX_EQC_DOT:
        case _HLL_LEX_EQC_SYMB: state = _HLL_LEX_SYMB; break;
        }
        break;
    case _HLL_LEX_DOTS:
        switch (eqc) {
        default: state = _HLL_LEX_FIN_DOTS; break;
        case _HLL_LEX_EQC_DOT: /* nop */ break;
        case _HLL_LEX_EQC_SIGNS:
        case _HLL_LEX_EQC_NUMBER:
        case _HLL_LEX_EQC_SYMB: state = _HLL_LEX_SYMB; break;
        }
        break;
    case _HLL_LEX_SYMB:
        switch (eqc) {
        default: state = _HLL_LEX_FIN_SYMB; break;
        case _HLL_LEX_EQC_SIGNS:
        case _HLL_LEX_EQC_NUMBER:
        case _HLL_LEX_EQC_DOT:
        case _HLL_LEX_EQC_SYMB: break;
        }
        break;
    case _HLL_LEX_UNEXPECTED:
        switch (eqc) {
        default: state = _HLL_LEX_FIN_UNEXPECTED; break;
        case _HLL_LEX_EQC_OTHER: /* nop */ break;
        }
        break;
    }

    return state;
}

static void
_lexer_error(hll_lexer *lexer, char const *fmt, ...) {
    lexer->has_errors = true;
    if (lexer->vm != NULL) {
        char buffer[4096];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        // TODO: Line and column numbers
        lexer->vm->config.error_fn(lexer->vm, 0, 0, buffer);
    }
}

void
hll_lexer_init(hll_lexer *lexer, char const *input, hll_vm *vm) {
    memset(lexer, 0, sizeof(hll_lexer));
    lexer->vm = vm;
    lexer->cursor = lexer->input = input;
}

void
hll_lexer_next(hll_lexer *lexer) {
    /// Pull it from structure so there is better chance in ends up in
    /// register.
    char const *cursor = lexer->cursor;
    char const *token_start = cursor;
    _hll_lexer_state state = _HLL_LEX_START;
    do {
        // Basic state machine-based lexing.
        // It has been decided to use character equivalence classes in hope that
        // lexical grammar stays simple. In cases like C grammar, there are a
        // lot of special cases that are better be handled by per-codepoint
        // lookup.
        _hll_lexer_equivalence_class eqc = _get_equivalence_class(*cursor++);
        // Decides what state should go next based on current state and new
        // character.
        state = _get_next_state(state, eqc);
        // If character we just parsed means that token is not yet started (for
        // example, if we encounter spaces), increment token start to adjust for
        // that. If it happens that we allow other non-significant characters,
        // this should better be turned into an array lookup.
        token_start += state == _HLL_LEX_START;
    } while (state < _HLL_LEX_FIN);

    // First, decrement cursor if needed.
    switch (state) {
    default: assert(0); break;
    case _HLL_LEX_FIN_LPAREN:
    case _HLL_LEX_FIN_RPAREN:
    case _HLL_LEX_FIN_QUOTE: break;
    case _HLL_LEX_FIN_DOTS:
    case _HLL_LEX_FIN_NUMBER:
    case _HLL_LEX_FIN_SYMB:
    case _HLL_LEX_FIN_EOF:
    case _HLL_LEX_FIN_UNEXPECTED: --cursor; break;
    }
    lexer->next.offset = token_start - lexer->input;
    lexer->next.length = cursor - token_start;
    lexer->cursor = cursor;

    // Now choose how to do per-token finalization
    switch (state) {
    default: assert(0); break;
    case _HLL_LEX_FIN_LPAREN: lexer->next.kind = HLL_TOK_LPAREN; break;
    case _HLL_LEX_FIN_RPAREN: lexer->next.kind = HLL_TOK_RPAREN; break;
    case _HLL_LEX_FIN_QUOTE: lexer->next.kind = HLL_TOK_QUOTE; break;
    case _HLL_LEX_FIN_DOTS: {
        if (lexer->next.length == 1) {
            lexer->next.kind = HLL_TOK_DOT;
        } else {
            _lexer_error(lexer, "Symbol consists of only dots");
            lexer->next.kind = HLL_TOK_SYMB;
        }
    } break;
    case _HLL_LEX_FIN_NUMBER: {
        intmax_t value = strtoimax(token_start, NULL, 10);
        if (errno == ERANGE || ((int64_t)value != value)) {
            _lexer_error(lexer, "Number literal is too large");
            value = 0;
        }

        lexer->next.kind = HLL_TOK_INT;
        lexer->next.value = value;
    } break;
    case _HLL_LEX_FIN_SYMB: lexer->next.kind = HLL_TOK_SYMB; break;
    case _HLL_LEX_FIN_EOF: lexer->next.kind = HLL_TOK_EOF; break;
    case _HLL_LEX_FIN_UNEXPECTED: lexer->next.kind = HLL_TOK_UNEXPECTED; break;
    }
}

hll_ast *
hll_read_ast(hll_reader *reader) {
    (void)reader;
    return NULL;
}

void *
hll_compile_ast(hll_compiler *compiler, hll_ast *ast) {
    (void)compiler;
    (void)ast;
    return NULL;
}
