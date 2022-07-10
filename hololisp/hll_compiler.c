#include "hll_compiler.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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
    reader.lexer = &lexer;
    reader.arena = &compilation_arena;

    hll_ast *ast = hll_read_ast(&reader);

    hll_compiler compiler = { 0 };

    void *result = hll_compile_ast(&compiler, ast);

    hll_memory_arena_clear(&compilation_arena);

    return result;
}

typedef enum {
    HLL_LEX_EQC_OTHER = 0x0,  // Anything not handled
    HLL_LEX_EQC_NUMBER,       // 0-9
    HLL_LEX_EQC_LPAREN,       // (
    HLL_LEX_EQC_RPAREN,       // )
    HLL_LEX_EQC_QUOTE,        // '
    HLL_LEX_EQC_DOT,          // .
    HLL_LEX_EQC_SIGNS,        // +-
    HLL_LEX_EQC_SYMB,         // All other characters that can be part of symbol
    HLL_LEX_EQC_EOF,
    HLL_LEX_EQC_SPACE,
    HLL_LEX_EQC_NEWLINE,
    HLL_LEX_EQC_COMMENT,  // ;
} HLL_lexer_equivalence_class;

static inline HLL_lexer_equivalence_class
get_equivalence_class(char symb) {
    HLL_lexer_equivalence_class eqc = HLL_LEX_EQC_OTHER;
    // clang-format off
    switch (symb) {
    case '(': eqc = HLL_LEX_EQC_LPAREN; break;
    case ')': eqc = HLL_LEX_EQC_RPAREN; break;
    case '\'': eqc = HLL_LEX_EQC_QUOTE; break;
    case '.': eqc = HLL_LEX_EQC_DOT; break;
    case '\0': eqc = HLL_LEX_EQC_EOF; break;
    case '\n': eqc = HLL_LEX_EQC_NEWLINE; break;
    case ';': eqc = HLL_LEX_EQC_COMMENT; break;
    case '1': case '2': case '3':
    case '4': case '5': case '6':
    case '7': case '8': case '9':
    case '0':
        eqc = HLL_LEX_EQC_NUMBER; break;
    case '-': case '+':
        eqc = HLL_LEX_EQC_SIGNS; break;
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
        eqc = HLL_LEX_EQC_SYMB; break;
    case ' ': case '\t': case '\f': case '\r': case '\v':
        eqc = HLL_LEX_EQC_SPACE; break;
    default: assert(0);
    }

    // clang-format on
    return eqc;
}

typedef enum {
    HLL_LEX_START,
    HLL_LEX_COMMENT,
    HLL_LEX_NUMBER,
    HLL_LEX_SEEN_SIGN,
    HLL_LEX_DOTS,
    HLL_LEX_SYMB,
    HLL_LEX_UNEXPECTED,

    HLL_LEX_FIN,
    HLL_LEX_FIN_LPAREN,
    HLL_LEX_FIN_RPAREN,
    HLL_LEX_FIN_QUOTE,
    HLL_LEX_FIN_NUMBER,
    HLL_LEX_FIN_DOTS,
    HLL_LEX_FIN_SYMB,
    HLL_LEX_FIN_EOF,
    HLL_LEX_FIN_UNEXPECTED,
    HLL_LEX_FIN_COMMENT,
} HLL_lexer_state;

static inline HLL_lexer_state
get_next_state(HLL_lexer_state state, HLL_lexer_equivalence_class eqc) {
    switch (state) {
    case HLL_LEX_START:
        switch (eqc) {
        case HLL_LEX_EQC_SPACE:
        case HLL_LEX_EQC_NEWLINE: /* nop */ break;
        case HLL_LEX_EQC_OTHER: state = HLL_LEX_UNEXPECTED; break;
        case HLL_LEX_EQC_NUMBER: state = HLL_LEX_NUMBER; break;
        case HLL_LEX_EQC_LPAREN: state = HLL_LEX_FIN_LPAREN; break;
        case HLL_LEX_EQC_RPAREN: state = HLL_LEX_FIN_RPAREN; break;
        case HLL_LEX_EQC_QUOTE: state = HLL_LEX_FIN_QUOTE; break;
        case HLL_LEX_EQC_DOT: state = HLL_LEX_DOTS; break;
        case HLL_LEX_EQC_SYMB: state = HLL_LEX_SYMB; break;
        case HLL_LEX_EQC_EOF: state = HLL_LEX_FIN_EOF; break;
        case HLL_LEX_EQC_SIGNS: state = HLL_LEX_SEEN_SIGN; break;
        case HLL_LEX_EQC_COMMENT: state = HLL_LEX_COMMENT; break;
        default: assert(0);
        }
        break;
    case HLL_LEX_COMMENT:
        switch (eqc) {
        case HLL_LEX_EQC_EOF:
        case HLL_LEX_EQC_NEWLINE: state = HLL_LEX_FIN_COMMENT; break;
        default: /* nop */ break;
        }
        break;
    case HLL_LEX_SEEN_SIGN:
        switch (eqc) {
        case HLL_LEX_EQC_NUMBER: state = HLL_LEX_NUMBER; break;
        case HLL_LEX_EQC_SIGNS:
        case HLL_LEX_EQC_DOT:
        case HLL_LEX_EQC_SYMB: state = HLL_LEX_SYMB; break;
        default: state = HLL_LEX_FIN_SYMB; break;
        }
        break;
    case HLL_LEX_NUMBER:
        switch (eqc) {
        case HLL_LEX_EQC_NUMBER: /* nop */ break;
        case HLL_LEX_EQC_SIGNS:
        case HLL_LEX_EQC_DOT:
        case HLL_LEX_EQC_SYMB: state = HLL_LEX_SYMB; break;
        default: state = HLL_LEX_FIN_NUMBER; break;
        }
        break;
    case HLL_LEX_DOTS:
        switch (eqc) {
        case HLL_LEX_EQC_DOT: /* nop */ break;
        case HLL_LEX_EQC_SIGNS:
        case HLL_LEX_EQC_NUMBER:
        case HLL_LEX_EQC_SYMB: state = HLL_LEX_SYMB; break;
        default: state = HLL_LEX_FIN_DOTS; break;
        }
        break;
    case HLL_LEX_SYMB:
        switch (eqc) {
        case HLL_LEX_EQC_SIGNS:
        case HLL_LEX_EQC_NUMBER:
        case HLL_LEX_EQC_DOT:
        case HLL_LEX_EQC_SYMB: break;
        default: state = HLL_LEX_FIN_SYMB; break;
        }
        break;
    case HLL_LEX_UNEXPECTED:
        if (eqc != HLL_LEX_EQC_OTHER) {
            state = HLL_LEX_FIN_UNEXPECTED;
        }
        break;
    default: assert(0);
    }

    return state;
}

static void
lexer_error(hll_lexer *lexer, char const *fmt, ...) {
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
    HLL_lexer_state state = HLL_LEX_START;
    do {
        // Basic state machine-based lexing.
        // It has been decided to use character equivalence classes in hope that
        // lexical grammar stays simple. In cases like C grammar, there are a
        // lot of special cases that are better be handled by per-codepoint
        // lookup.
        HLL_lexer_equivalence_class eqc = get_equivalence_class(*cursor++);
        // Decides what state should go next based on current state and new
        // character.
        state = get_next_state(state, eqc);
        // If character we just parsed means that token is not yet started (for
        // example, if we encounter spaces), increment token start to adjust for
        // that. If it happens that we allow other non-significant characters,
        // this should better be turned into an array lookup.
        token_start += state == HLL_LEX_START;
    } while (state < HLL_LEX_FIN);

    // First, decrement cursor if needed.
    switch (state) {
    case HLL_LEX_FIN_LPAREN:
    case HLL_LEX_FIN_RPAREN:
    case HLL_LEX_FIN_QUOTE: break;
    case HLL_LEX_FIN_DOTS:
    case HLL_LEX_FIN_NUMBER:
    case HLL_LEX_FIN_SYMB:
    case HLL_LEX_FIN_EOF:
    case HLL_LEX_FIN_COMMENT:
    case HLL_LEX_FIN_UNEXPECTED: --cursor; break;
    default: assert(0);
    }
    lexer->next.offset = token_start - lexer->input;
    lexer->next.length = cursor - token_start;
    lexer->cursor = cursor;

    // Now choose how to do per-token finalization
    switch (state) {
    case HLL_LEX_FIN_LPAREN: lexer->next.kind = HLL_TOK_LPAREN; break;
    case HLL_LEX_FIN_RPAREN: lexer->next.kind = HLL_TOK_RPAREN; break;
    case HLL_LEX_FIN_QUOTE: lexer->next.kind = HLL_TOK_QUOTE; break;
    case HLL_LEX_FIN_SYMB: lexer->next.kind = HLL_TOK_SYMB; break;
    case HLL_LEX_FIN_EOF: lexer->next.kind = HLL_TOK_EOF; break;
    case HLL_LEX_FIN_UNEXPECTED: lexer->next.kind = HLL_TOK_UNEXPECTED; break;
    case HLL_LEX_FIN_COMMENT: lexer->next.kind = HLL_TOK_COMMENT; break;
    case HLL_LEX_FIN_DOTS: {
        if (lexer->next.length == 1) {
            lexer->next.kind = HLL_TOK_DOT;
        } else {
            lexer_error(lexer, "Symbol consists of only dots");
            lexer->next.kind = HLL_TOK_SYMB;
        }
    } break;
    case HLL_LEX_FIN_NUMBER: {
        intmax_t value = strtoimax(token_start, NULL, 10);
        if (errno == ERANGE || ((int64_t)value != value)) {
            lexer_error(lexer, "Number literal is too large");
            value = 0;
        }

        lexer->next.kind = HLL_TOK_INT;
        lexer->next.value = value;
    } break;
    default: assert(0);
    }
}

void
hll_reader_init(hll_reader *reader, hll_lexer *lexer, hll_memory_arena *arena,
                struct hll_vm *vm) {
    memset(reader, 0, sizeof(hll_reader));
    reader->lexer = lexer;
    reader->arena = arena;
    reader->vm = vm;
}

typedef enum {
    HLL_READ_LIST,
    HLL_READ_QUOTE,
    HLL_READ_DOT,
} hll_ast_read_entry_kind;

// This represents non-finished linked list in terms of lisp objects.
typedef struct {
    hll_ast_read_entry_kind kind;
    hll_ast *first;
    hll_ast *last;
} hll_ast_read_entry;

typedef struct {
    hll_ast_read_entry *stack;
    uint32_t stack_idx;
} hll_parse_state;

static bool
append_to_entry(hll_ast_read_entry *entry, hll_ast *cons) {
    if (entry->first != NULL) {
        entry->last->as.cons.cdr = cons;
        entry->last = cons;
    } else {
        entry->first = entry->last = cons;
    }

    return entry->kind != HLL_READ_LIST;
}

static void
finalize_entry(hll_ast_read_entry *target, hll_ast_read_entry *src) {
    switch (src->kind) {
    case HLL_READ_LIST: {
        hll_ast *list = stack[stack_index--].first;
        if (list == NULL) {
            list = nil;
        }
        _APPEND_TO_LIST(list);
    } break;
    }
}

static void
reader_error(hll_reader *reader, char const *fmt, ...) {
    reader->has_errors = true;
    if (reader->vm != NULL) {
        char buffer[4096];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        // TODO: Line and column numbers
        reader->vm->config.error_fn(reader->vm, 0, 0, buffer);
    }
}

hll_ast *
hll_read_ast(hll_reader *reader) {
    hll_ast_read_entry *stack = NULL;
    uint32_t stack_index = 0;
    // Global scope is made a cons linked list not to introduce too big
    // difference from all other ast nodes.
    // Note that despite this being a correct lisp object, compiler treats
    // it simply as list
    hll_sb_push(stack, (hll_ast_read_entry){ 0 });

    hll_ast *nil = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
    nil->kind = HLL_AST_NIL;
    hll_ast *true_ = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
    true_->kind = HLL_AST_TRUE;
    (void)true_;
    hll_ast *quote_symb =
        hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
    quote_symb->kind = HLL_AST_SYMB;
    quote_symb->as.symb = "quote";

#define FINALIZE_ENTRY                                     \
    do {                                                   \
        assert(stack_index);                               \
        hll_ast_read_entry *entry = stack + stack_index--; \
        finalize_entry(stack + stack_index, entry);        \
    } while (0)

#define APPEND_TO_LIST(_ast)                                       \
    do {                                                            \
        hll_ast *cons =                                             \
            hll_memory_arena_alloc(reader->arena, sizeof(hll_ast)); \
        cons->kind = HLL_AST_CONS;                                  \
        cons->as.cons.car = _ast;                                   \
        cons->as.cons.cdr = nil;                                    \
        if (append_to_entry(stack + stack_index, cons)) {           \
            FINALIZE_ENTRY;                                         \
        }                                                           \
    } while (0)

    bool is_finished = false;
    while (!is_finished) {
        hll_lexer_next(reader->lexer);

        switch (reader->lexer->next.kind) {
        case HLL_TOK_EOF: is_finished = true; break;
        case HLL_TOK_INT: {
            hll_ast *ast =
                hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
            ast->kind = HLL_AST_INT;
            ast->as.num = reader->lexer->next.value;
            APPEND_TO_LIST(ast);
        } break;
        case HLL_TOK_SYMB: {
            if (reader->lexer->next.length == 1 &&
                reader->lexer->input[reader->lexer->next.offset] == 't') {
                APPEND_TO_LIST(true_);
                break;
            }

            hll_ast *ast =
                hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
            ast->kind = HLL_AST_SYMB;
            ast->as.symb = hll_memory_arena_alloc(
                reader->arena, reader->lexer->next.length + 1);
            strncpy((void *)ast->as.symb,
                    reader->lexer->input + reader->lexer->next.offset,
                    reader->lexer->next.length);
            ast->as.num = reader->lexer->next.value;
            APPEND_TO_LIST(ast);
        } break;
        case HLL_TOK_LPAREN: {
            hll_sb_push(stack, (hll_ast_read_entry){ 0 });
            ++stack_index;
        } break;
        case HLL_TOK_RPAREN: {
            if (stack_index == 0) {
                reader_error(reader, "Stray right parenthesis");
                break;
            }

            FINALIZE_ENTRY;

        } break;
        case HLL_TOK_QUOTE: {
            hll_sb_push(stack, (hll_ast_read_entry){ .kind = HLL_READ_QUOTE });
        } break;
        case HLL_TOK_DOT:
            if (stack[stack_index - 1].first == NULL) {
                reader_error(reader, "Dot in list without first element");
            }
            hll_sb_push(stack, (hll_ast_read_entry){ .kind = HLL_READ_DOT });
            break;
        case HLL_TOK_COMMENT: /* nop */ break;
        case HLL_TOK_UNEXPECTED:
            reader_error(reader, "Unexpected characters");
            break;
        }
    }

    if (stack_index) {
        reader_error(reader, "Unterminated list definitions: %u", stack_index);
    }

#undef FINALIZE_ENTRY
#undef APPEND_TO_LIST

    hll_ast *result = stack->first;
    if (result == NULL) {
        result = nil;
    }
    hll_sb_free(stack);

    return result;
}

void *
hll_compile_ast(hll_compiler *compiler, hll_ast *ast) {
    (void)compiler;
    (void)ast;
    return NULL;
}
