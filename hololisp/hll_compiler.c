#include "hll_compiler.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_hololisp.h"
#include "hll_mem.h"
#include "hll_obj.h"
#include "hll_util.h"
#include "hll_vm.h"

hll_bytecode *hll_compile(hll_vm *vm, const char *source) {
  hll_memory_arena compilation_arena = {0};
  hll_lexer lexer;
  hll_lexer_init(&lexer, source, vm);
  hll_reader reader;
  hll_reader_init(&reader, &lexer, &compilation_arena, vm);

  hll_ast *ast = hll_read_ast(&reader);

  hll_bytecode *bytecode = calloc(1, sizeof(hll_bytecode));
  hll_compiler compiler = {0};
  compiler.vm = vm;
  compiler.bytecode = bytecode;
  hll_compile_ast(&compiler, ast);

  hll_memory_arena_clear(&compilation_arena);
  if (lexer.has_errors || reader.has_errors || compiler.has_errors) {
    free(bytecode);
    return NULL;
  }

  return bytecode;
}

typedef enum {
  HLL_LEX_EQC_OTHER = 0x0, // Anything not handled
  HLL_LEX_EQC_NUMBER,      // 0-9
  HLL_LEX_EQC_LPAREN,      // (
  HLL_LEX_EQC_RPAREN,      // )
  HLL_LEX_EQC_QUOTE,       // '
  HLL_LEX_EQC_DOT,         // .
  HLL_LEX_EQC_SIGNS,       // +-
  HLL_LEX_EQC_SYMB,        // All other characters that can be part of symbol
  HLL_LEX_EQC_EOF,
  HLL_LEX_EQC_SPACE,
  HLL_LEX_EQC_NEWLINE,
  HLL_LEX_EQC_COMMENT, // ;
} HLL_lexer_equivalence_class;

static inline HLL_lexer_equivalence_class get_equivalence_class(char symb) {
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
    default: break;
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

static inline HLL_lexer_state get_next_state(HLL_lexer_state state,
                                             HLL_lexer_equivalence_class eqc) {
  switch (state) {
  case HLL_LEX_START:
    switch (eqc) {
    case HLL_LEX_EQC_SPACE:
    case HLL_LEX_EQC_NEWLINE: /* nop */
      break;
    case HLL_LEX_EQC_OTHER:
      state = HLL_LEX_UNEXPECTED;
      break;
    case HLL_LEX_EQC_NUMBER:
      state = HLL_LEX_NUMBER;
      break;
    case HLL_LEX_EQC_LPAREN:
      state = HLL_LEX_FIN_LPAREN;
      break;
    case HLL_LEX_EQC_RPAREN:
      state = HLL_LEX_FIN_RPAREN;
      break;
    case HLL_LEX_EQC_QUOTE:
      state = HLL_LEX_FIN_QUOTE;
      break;
    case HLL_LEX_EQC_DOT:
      state = HLL_LEX_DOTS;
      break;
    case HLL_LEX_EQC_SYMB:
      state = HLL_LEX_SYMB;
      break;
    case HLL_LEX_EQC_EOF:
      state = HLL_LEX_FIN_EOF;
      break;
    case HLL_LEX_EQC_SIGNS:
      state = HLL_LEX_SEEN_SIGN;
      break;
    case HLL_LEX_EQC_COMMENT:
      state = HLL_LEX_COMMENT;
      break;
    default:
      assert(0);
    }
    break;
  case HLL_LEX_COMMENT:
    switch (eqc) {
    case HLL_LEX_EQC_EOF:
    case HLL_LEX_EQC_NEWLINE:
      state = HLL_LEX_FIN_COMMENT;
      break;
    default: /* nop */
      break;
    }
    break;
  case HLL_LEX_SEEN_SIGN:
    switch (eqc) {
    case HLL_LEX_EQC_NUMBER:
      state = HLL_LEX_NUMBER;
      break;
    case HLL_LEX_EQC_SIGNS:
    case HLL_LEX_EQC_DOT:
    case HLL_LEX_EQC_SYMB:
      state = HLL_LEX_SYMB;
      break;
    default:
      state = HLL_LEX_FIN_SYMB;
      break;
    }
    break;
  case HLL_LEX_NUMBER:
    switch (eqc) {
    case HLL_LEX_EQC_NUMBER: /* nop */
      break;
    case HLL_LEX_EQC_SIGNS:
    case HLL_LEX_EQC_DOT:
    case HLL_LEX_EQC_SYMB:
      state = HLL_LEX_SYMB;
      break;
    default:
      state = HLL_LEX_FIN_NUMBER;
      break;
    }
    break;
  case HLL_LEX_DOTS:
    switch (eqc) {
    case HLL_LEX_EQC_DOT: /* nop */
      break;
    case HLL_LEX_EQC_SIGNS:
    case HLL_LEX_EQC_NUMBER:
    case HLL_LEX_EQC_SYMB:
      state = HLL_LEX_SYMB;
      break;
    default:
      state = HLL_LEX_FIN_DOTS;
      break;
    }
    break;
  case HLL_LEX_SYMB:
    switch (eqc) {
    case HLL_LEX_EQC_SIGNS:
    case HLL_LEX_EQC_NUMBER:
    case HLL_LEX_EQC_DOT:
    case HLL_LEX_EQC_SYMB:
      break;
    default:
      state = HLL_LEX_FIN_SYMB;
      break;
    }
    break;
  case HLL_LEX_UNEXPECTED:
    if (eqc != HLL_LEX_EQC_OTHER) {
      state = HLL_LEX_FIN_UNEXPECTED;
    }
    break;
  default:
    assert(0);
  }

  return state;
}

HLL_ATTR(format(printf, 2, 3))
static void lexer_error(hll_lexer *lexer, const char *fmt, ...) {
  lexer->has_errors = true;
  if (lexer->vm == NULL) {
    return;
  }

  char buffer[4096];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  hll_report_error(lexer->vm, lexer->next.offset, lexer->next.length, buffer);
}

void hll_lexer_init(hll_lexer *lexer, const char *input, hll_vm *vm) {
  memset(lexer, 0, sizeof(hll_lexer));
  lexer->vm = vm;
  lexer->cursor = lexer->input = input;
}

void hll_lexer_next(hll_lexer *lexer) {
  // Pull it from structure so there is better chance in ends up in
  // register.
  const char *cursor = lexer->cursor;
  const char *token_start = cursor;
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
  case HLL_LEX_FIN_QUOTE:
    break;
  case HLL_LEX_FIN_DOTS:
  case HLL_LEX_FIN_NUMBER:
  case HLL_LEX_FIN_SYMB:
  case HLL_LEX_FIN_EOF:
  case HLL_LEX_FIN_COMMENT:
  case HLL_LEX_FIN_UNEXPECTED:
    --cursor;
    break;
  default:
    assert(0);
  }
  lexer->next.offset = token_start - lexer->input;
  lexer->next.length = cursor - token_start;
  lexer->cursor = cursor;

  // Now choose how to do per-token finalization
  switch (state) {
  case HLL_LEX_FIN_LPAREN:
    lexer->next.kind = HLL_TOK_LPAREN;
    break;
  case HLL_LEX_FIN_RPAREN:
    lexer->next.kind = HLL_TOK_RPAREN;
    break;
  case HLL_LEX_FIN_QUOTE:
    lexer->next.kind = HLL_TOK_QUOTE;
    break;
  case HLL_LEX_FIN_SYMB:
    lexer->next.kind = HLL_TOK_SYMB;
    break;
  case HLL_LEX_FIN_EOF:
    lexer->next.kind = HLL_TOK_EOF;
    break;
  case HLL_LEX_FIN_UNEXPECTED:
    lexer->next.kind = HLL_TOK_UNEXPECTED;
    break;
  case HLL_LEX_FIN_COMMENT:
    lexer->next.kind = HLL_TOK_COMMENT;
    break;
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
  default:
    assert(0);
  }
}

void hll_reader_init(hll_reader *reader, hll_lexer *lexer,
                     hll_memory_arena *arena, struct hll_vm *vm) {
  memset(reader, 0, sizeof(hll_reader));
  reader->lexer = lexer;
  reader->arena = arena;
  reader->vm = vm;
  reader->nil = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
  reader->nil->kind = HLL_AST_NIL;
  reader->true_ = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
  reader->true_->kind = HLL_AST_TRUE;
  reader->quote_symb = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
  reader->quote_symb->kind = HLL_AST_SYMB;
  reader->quote_symb->as.symb.str = "quote";
  reader->quote_symb->as.symb.length = strlen("quote");
}

HLL_ATTR(format(printf, 2, 3))
static void reader_error(hll_reader *reader, const char *fmt, ...) {
  reader->has_errors = true;
  if (reader->vm == NULL) {
    return;
  }

  char buffer[4096];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  hll_report_error(reader->vm, reader->lexer->next.offset,
                   reader->lexer->next.length, buffer);
}

static void peek_token(hll_reader *reader) {
  if (reader->should_return_old_token) {
    return;
  }

  bool is_done = false;
  while (!is_done) {
    hll_lexer_next(reader->lexer);
    if (reader->lexer->next.kind == HLL_TOK_COMMENT) {
    } else if (reader->lexer->next.kind == HLL_TOK_UNEXPECTED) {
      reader_error(reader, "Unexpected token");
    } else {
      is_done = true;
    }
  }

  reader->should_return_old_token = true;
}

static void eat_token(hll_reader *reader) {
  reader->should_return_old_token = false;
}

static hll_ast *make_cons(hll_reader *reader, hll_ast *car, hll_ast *cdr) {
  hll_ast *cons = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
  cons->kind = HLL_AST_CONS;
  cons->as.cons.car = car;
  cons->as.cons.cdr = cdr;
  return cons;
}

static hll_ast *read_expr(hll_reader *reader);

static hll_ast *read_list(hll_reader *reader) {
  peek_token(reader);
  assert(reader->lexer->next.kind == HLL_TOK_LPAREN);
  eat_token(reader);
  // Now we expect at least one list element followed by others ending either
  // with right paren or dot, element and right paren Now check if we don't
  // have first element
  peek_token(reader);
  if (reader->lexer->next.kind == HLL_TOK_RPAREN) {
    eat_token(reader);
    return reader->nil;
  }

  hll_ast *list_head;
  hll_ast *list_tail;
  list_head = list_tail = make_cons(reader, read_expr(reader), reader->nil);

  // Now enter the loop of parsing other list elements.
  for (;;) {
    peek_token(reader);
    if (reader->lexer->next.kind == HLL_TOK_EOF) {
      reader_error(reader, "Missing closing paren (eof encountered)");
      return list_head;
    } else if (reader->lexer->next.kind == HLL_TOK_RPAREN) {
      eat_token(reader);
      return list_head;
    } else if (reader->lexer->next.kind == HLL_TOK_DOT) {
      eat_token(reader);
      list_tail->as.cons.cdr = read_expr(reader);

      peek_token(reader);
      if (reader->lexer->next.kind != HLL_TOK_RPAREN) {
        reader_error(reader, "Missing closing paren after dot");
        return list_head;
      }
      eat_token(reader);
      return list_head;
    }

    hll_ast *ast = read_expr(reader);
    list_tail->as.cons.cdr = make_cons(reader, ast, reader->nil);
    list_tail = list_tail->as.cons.cdr;
  }

  assert(!"Unreachable");
}

static hll_ast *read_expr(hll_reader *reader) {
  hll_ast *ast = reader->nil;
  peek_token(reader);
  switch (reader->lexer->next.kind) {
  case HLL_TOK_EOF:
    break;
  case HLL_TOK_INT:
    eat_token(reader);
    ast = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
    ast->kind = HLL_AST_INT;
    ast->as.num = reader->lexer->next.value;
    break;
  case HLL_TOK_SYMB:
    eat_token(reader);
    if (reader->lexer->next.length == 1 &&
        reader->lexer->input[reader->lexer->next.offset] == 't') {
      ast = reader->true_;
      break;
    }

    ast = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
    ast->kind = HLL_AST_SYMB;
    ast->as.symb.str =
        hll_memory_arena_alloc(reader->arena, reader->lexer->next.length + 1);
    ast->as.symb.length = reader->lexer->next.length;
    strncpy((void *)ast->as.symb.str,
            reader->lexer->input + reader->lexer->next.offset,
            reader->lexer->next.length);
    break;
  case HLL_TOK_LPAREN:
    ast = read_list(reader);
    break;
  case HLL_TOK_QUOTE: {
    eat_token(reader);
    hll_ast *b = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
    b->kind = HLL_AST_CONS;
    b->as.cons.car = read_expr(reader);
    b->as.cons.cdr = reader->nil;
    hll_ast *a = hll_memory_arena_alloc(reader->arena, sizeof(hll_ast));
    a->kind = HLL_AST_CONS;
    a->as.cons.car = reader->quote_symb;
    a->as.cons.cdr = b;
    ast = a;
  } break;
  case HLL_TOK_COMMENT:
  case HLL_TOK_UNEXPECTED:
    // These code paths should be covered in next_token()
    assert(0);
    break;
  default:
    eat_token(reader);
    reader_error(reader, "Unexpected token when parsing expression");
    break;
  }
  return ast;
}

hll_ast *hll_read_ast(hll_reader *reader) {
  hll_ast *list_head = NULL;
  hll_ast *list_tail = NULL;

  for (;;) {
    peek_token(reader);
    if (reader->lexer->next.kind == HLL_TOK_EOF) {
      break;
    }

    hll_ast *ast = read_expr(reader);
    hll_ast *cons = make_cons(reader, ast, reader->nil);

    if (list_head == NULL) {
      list_head = list_tail = cons;
    } else {
      list_tail->as.cons.cdr = cons;
      list_tail = cons;
    }
  }

  if (list_head == NULL) {
    list_head = reader->nil;
  }

  return list_head;
}

static size_t ast_list_length(const hll_ast *ast) {
  size_t length = 0;

  for (; ast->kind == HLL_AST_CONS; ast = ast->as.cons.cdr) {
    ++length;
  }

  return length;
}

HLL_ATTR(format(printf, 3, 4))
static void compiler_error(hll_compiler *compiler, const hll_ast *ast,
                           const char *fmt, ...) {
  compiler->has_errors = true;
  if (compiler->vm == NULL) {
    return;
  }

  char buffer[4096];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  // TODO: Length
  hll_report_error(compiler->vm, ast->offset, 0, buffer);
}

#define HLL_ENUMERATE_CAR_CDR                                                  \
  HLL_CAR_CDR(a, A)                                                            \
  HLL_CAR_CDR(d, D)                                                            \
  HLL_CAR_CDR(aa, AA)                                                          \
  HLL_CAR_CDR(ad, AD)                                                          \
  HLL_CAR_CDR(da, DA)                                                          \
  HLL_CAR_CDR(dd, DD)                                                          \
  HLL_CAR_CDR(aaa, AAA)                                                        \
  HLL_CAR_CDR(aad, AAD)                                                        \
  HLL_CAR_CDR(ada, ADA)                                                        \
  HLL_CAR_CDR(add, ADD)                                                        \
  HLL_CAR_CDR(daa, DAA)                                                        \
  HLL_CAR_CDR(dad, DAD)                                                        \
  HLL_CAR_CDR(dda, DDA)                                                        \
  HLL_CAR_CDR(ddd, DDD)                                                        \
  HLL_CAR_CDR(aaaa, AAAA)                                                      \
  HLL_CAR_CDR(aaad, AAAD)                                                      \
  HLL_CAR_CDR(aada, AADA)                                                      \
  HLL_CAR_CDR(aadd, AADD)                                                      \
  HLL_CAR_CDR(adaa, ADAA)                                                      \
  HLL_CAR_CDR(adad, ADAD)                                                      \
  HLL_CAR_CDR(adda, ADDA)                                                      \
  HLL_CAR_CDR(addd, ADDD)                                                      \
  HLL_CAR_CDR(daaa, DAAA)                                                      \
  HLL_CAR_CDR(daad, DAAD)                                                      \
  HLL_CAR_CDR(dada, DADA)                                                      \
  HLL_CAR_CDR(dadd, DADD)                                                      \
  HLL_CAR_CDR(ddaa, DDAA)                                                      \
  HLL_CAR_CDR(ddad, DDAD)                                                      \
  HLL_CAR_CDR(ddda, DDDA)                                                      \
  HLL_CAR_CDR(dddd, DDDD)

// Denotes special forms in language.
// Special forms are different from other lisp constructs because they require
// special handling from compiler side.
// However, this VM is different from other language ones because language
// core contains semi-complex operations, like getting car and cdr.
// Thus, a number a forms can be easily expressed through already
// existent bytecode instructions. This is why things like car and cdr appear
// here.
typedef enum {
  HLL_FORM_REGULAR,
  // Quote returns unevaluated argument
  HLL_FORM_QUOTE,
  // If checks condition and executes one of its arms
  HLL_FORM_IF,
  // Lambda constructs a new function
  //  HLL_FORM_LAMBDA,
  // Setf sets value pointed to by location defined by first argument
  // as second argument. First argument is of special kind of form,
  // which denotes location and requires special handling from compiler.
  HLL_FORM_SETF,
  // Defines variable in current context.
  HLL_FORM_DEFVAR,
  // let is different from defvar because it creates new lexical context
  // and puts variables there.
  HLL_FORM_LET,
  HLL_FORM_LIST,
  HLL_FORM_CONS,
  HLL_FORM_SETCAR,
  HLL_FORM_SETCDR,
  HLL_FORM_DEFUN,
#define HLL_CAR_CDR(_, _letters) HLL_FORM_C##_letters##R,
  HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR
} hll_form_kind;

static hll_form_kind get_form_kind(const char *symb) {
  hll_form_kind kind = HLL_FORM_REGULAR;
  if (strcmp(symb, "quote") == 0) {
    kind = HLL_FORM_QUOTE;
  } else if (strcmp(symb, "if") == 0) {
    kind = HLL_FORM_IF;
    //  } else if (strcmp(symb, "lambda") == 0) {
    //    kind = HLL_FORM_LAMBDA;
  } else if (strcmp(symb, "setf") == 0) {
    kind = HLL_FORM_SETF;
  } else if (strcmp(symb, "defvar") == 0) {
    kind = HLL_FORM_DEFVAR;
  } else if (strcmp(symb, "let") == 0) {
    kind = HLL_FORM_LET;
  } else if (strcmp(symb, "list") == 0) {
    kind = HLL_FORM_LIST;
  } else if (strcmp(symb, "cons") == 0) {
    kind = HLL_FORM_CONS;
  } else if (strcmp(symb, "setcar") == 0) {
    kind = HLL_FORM_SETCAR;
  } else if (strcmp(symb, "setcdr") == 0) {
    kind = HLL_FORM_SETCDR;
  } else if (strcmp(symb, "defun") == 0) {
    kind = HLL_FORM_DEFUN;
  }
#define HLL_CAR_CDR(_lower, _upper)                                            \
  else if (strcmp(symb, "c" #_lower "r") == 0) {                               \
    kind = HLL_FORM_C##_upper##R;                                              \
  }
  HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR

  return kind;
}

static size_t get_current_op_idx(hll_bytecode *bytecode) {
  return hll_sb_len(bytecode->ops);
}

size_t emit_u8(hll_bytecode *bytecode, uint8_t byte) {
  size_t idx = get_current_op_idx(bytecode);
  hll_sb_push(bytecode->ops, byte);
  return idx;
}

size_t emit_op(hll_bytecode *bytecode, hll_bytecode_op op) {
  assert(op <= 0xFF);
  return emit_u8(bytecode, op);
}

static void write_u16_be(uint8_t *data, uint16_t value) {
  *data++ = (value >> 8) & 0xFF;
  *data = value & 0xFF;
}

static size_t emit_u16(hll_bytecode *bytecode, uint16_t value) {
  size_t idx = emit_u8(bytecode, (value >> 8) & 0xFF);
  emit_u8(bytecode, value & 0xFF);
  return idx;
}

static uint16_t add_int_constant_and_return_its_index(hll_compiler *compiler,
                                                      hll_num value) {
  for (size_t i = 0; i < hll_sb_len(compiler->bytecode->constant_pool); ++i) {
    hll_obj *test = compiler->bytecode->constant_pool[i];
    if (test->kind == HLL_OBJ_NUM && test->as.num == value) {
      uint16_t narrowed = i;
      assert(i == narrowed);
      return narrowed;
    }
  }

  hll_sb_push(compiler->bytecode->constant_pool,
              hll_new_num(compiler->vm, value));
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  return narrowed;
}

static uint16_t add_symbol_and_return_its_index(hll_compiler *compiler,
                                                const char *symb_,
                                                size_t length) {
  for (size_t i = 0; i < hll_sb_len(compiler->bytecode->constant_pool); ++i) {
    hll_obj *test = compiler->bytecode->constant_pool[i];
    if (test->kind == HLL_OBJ_SYMB &&
        strcmp(hll_unwrap_zsymb(test), symb_) == 0) {
      uint16_t narrowed = i;
      assert(i == narrowed);
      return narrowed;
    }
  }

  hll_obj *symb = hll_new_symbol(compiler->vm, symb_, length);

  hll_sb_push(compiler->bytecode->constant_pool, symb);
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  return narrowed;
}

static void compile_symbol(hll_compiler *compiler, const hll_ast *ast) {
  assert(ast->kind == HLL_AST_SYMB);
  emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
  emit_u16(compiler->bytecode,
           add_symbol_and_return_its_index(compiler, ast->as.symb.str,
                                           ast->as.symb.length));
}

static void compile_expression(hll_compiler *compiler, const hll_ast *ast);
static void compile_eval_expression(hll_compiler *compiler, const hll_ast *ast);

static void compile_function_call(hll_compiler *compiler, const hll_ast *list) {
  hll_ast *fn = list->as.cons.car;
  compile_eval_expression(compiler, fn);
  // Now make arguments
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  for (hll_ast *arg = list->as.cons.cdr; arg->kind != HLL_AST_NIL;
       arg = arg->as.cons.cdr) {
    assert(arg->kind == HLL_AST_CONS);
    hll_ast *obj = arg->as.cons.car;
    compile_eval_expression(compiler, obj);
    emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  }
  emit_op(compiler->bytecode, HLL_BYTECODE_POP);
  emit_op(compiler->bytecode, HLL_BYTECODE_CALL);
}

static void compile_quote(hll_compiler *compiler, const hll_ast *args) {
  if (args->kind != HLL_AST_CONS) {
    compiler_error(compiler, args, "'quote' form must have an argument");
    return;
  }
  if (args->as.cons.cdr->kind != HLL_AST_NIL) {
    compiler_error(compiler, args,
                   "'quote' form must have exactly one argument");
  }
  compile_expression(compiler, args->as.cons.car);
}

static void compile_if(hll_compiler *compiler, const hll_ast *args) {
  size_t length = ast_list_length(args);
  if (length != 2 && length != 3) {
    compiler_error(compiler, args, "'if' form expects 2 or 3 arguments");
    return;
  }

  hll_ast *cond = args->as.cons.car;
  compile_eval_expression(compiler, cond);
  hll_ast *pos_arm = args->as.cons.cdr;
  assert(pos_arm->kind == HLL_AST_CONS);
  hll_ast *neg_arm = pos_arm->as.cons.cdr;
  if (neg_arm->kind == HLL_AST_CONS) {
    neg_arm = neg_arm->as.cons.car;
  }

  pos_arm = pos_arm->as.cons.car;
  emit_op(compiler->bytecode, HLL_BYTECODE_JN);
  size_t jump_false = emit_u16(compiler->bytecode, 0);
  compile_eval_expression(compiler, pos_arm);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  emit_op(compiler->bytecode, HLL_BYTECODE_JN);
  size_t jump_out = emit_u16(compiler->bytecode, 0);
  write_u16_be(compiler->bytecode->ops + jump_false,
               get_current_op_idx(compiler->bytecode) - jump_false - 2);
  compile_eval_expression(compiler, neg_arm);
  write_u16_be(compiler->bytecode->ops + jump_out,
               get_current_op_idx(compiler->bytecode) - jump_out - 2);
}

static void compile_let(hll_compiler *compiler, const hll_ast *args) {
  size_t length = ast_list_length(args);
  if (length < 1) {
    compiler_error(compiler, args,
                   "'let' special form requires variable declarations");
    return;
  }

  emit_op(compiler->bytecode, HLL_BYTECODE_PUSHENV);
  for (const hll_ast *let = args->as.cons.car; let->kind != HLL_AST_NIL;
       let = let->as.cons.cdr) {
    const hll_ast *pair = let->as.cons.car;
    assert(pair->kind == HLL_AST_CONS);
    const hll_ast *name = pair->as.cons.car;
    const hll_ast *value = pair->as.cons.cdr;
    if (value->kind == HLL_AST_CONS) {
      assert(value->as.cons.cdr->kind == HLL_AST_NIL);
      value = value->as.cons.car;
    }

    assert(name->kind == HLL_AST_SYMB);
    compile_expression(compiler, name);
    compile_eval_expression(compiler, value);
    emit_op(compiler->bytecode, HLL_BYTECODE_LET);
  }

  for (const hll_ast *prog = args->as.cons.cdr; prog->kind == HLL_AST_CONS;
       prog = prog->as.cons.cdr) {
    compile_eval_expression(compiler, prog->as.cons.car);
    if (prog->as.cons.cdr->kind != HLL_AST_NIL) {
      emit_op(compiler->bytecode, HLL_BYTECODE_POP);
    }
  }

  emit_op(compiler->bytecode, HLL_BYTECODE_POPENV);
}

#if 0
static void compile_lambda(hll_compiler *compiler, hll_ast *args) {
  size_t length = ast_list_length(args);
  if (length != 2) {
    compiler_error(compiler, args, "'lambda' expects exactly 2 arguments");
    return;
  }
  hll_ast *params = args->as.cons.car;
  for (hll_ast *test = params; test->kind == HLL_AST_CONS;
       test = test->as.cons.cdr) {
    if (test->as.cons.car->kind != HLL_AST_SYMB) {
      compiler_error(compiler, args,
                     "'lambda' parameter list must consist only of symbols");
    }
  }
  hll_ast *body = args->as.cons.cdr;
  assert(body->kind == HLL_AST_CONS);
  assert(body->as.cons.cdr->kind == HLL_AST_NIL);
  body = body->as.cons.car;

  compile_expression(compiler, params);
  compile_expression(compiler, body);
  emit_op(compiler->bytecode, HLL_BYTECODE_MAKE_LAMBDA);
}
#endif

#define HLL_CAR_CDR(_lower, _)                                                 \
  static void compile_c##_lower##r(hll_compiler *compiler,                     \
                                   const hll_ast *args) {                      \
    if (ast_list_length(args) != 1) {                                          \
      compiler_error(compiler, args,                                           \
                     "'c" #_lower "r' expects exactly 1 argument");            \
    } else {                                                                   \
      compile_eval_expression(compiler, args->as.cons.car);                    \
      const char *ops = #_lower;                                               \
      const char *op = ops + sizeof(#_lower) - 2;                              \
      for (;;) {                                                               \
        if (*op == 'a') {                                                      \
          emit_op(compiler->bytecode, HLL_BYTECODE_CAR);                       \
        } else if (*op == 'd') {                                               \
          emit_op(compiler->bytecode, HLL_BYTECODE_CDR);                       \
        } else {                                                               \
          HLL_UNREACHABLE;                                                     \
        }                                                                      \
        if (op == ops) {                                                       \
          break;                                                               \
        }                                                                      \
        --op;                                                                  \
      }                                                                        \
    }                                                                          \
  }
HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR

typedef enum {
  HLL_LOC_NONE,
  HLL_LOC_FORM_SYMB,
  HLL_LOC_FORM_CAR,
  HLL_LOC_FORM_CDR,
} hll_location_form;

static hll_location_form get_location_form(const hll_ast *location) {
  hll_location_form kind = HLL_LOC_NONE;
  if (location->kind == HLL_AST_SYMB) {
    kind = HLL_LOC_FORM_SYMB;
  } else if (location->kind == HLL_AST_CONS) {
    hll_ast *first = location->as.cons.car;
    if (first->kind == HLL_AST_SYMB) {
      if (strcmp(first->as.symb.str, "car") == 0) {
        kind = HLL_LOC_FORM_CAR;
      } else if (strcmp(first->as.symb.str, "cdr") == 0) {
        kind = HLL_LOC_FORM_CDR;
      }
    }
  }

  return kind;
}

static void compile_set_location(hll_compiler *compiler,
                                 const hll_ast *location,
                                 const hll_ast *value) {
  hll_location_form kind = get_location_form(location);
  switch (kind) {
  case HLL_LOC_NONE:
    compiler_error(compiler, location, "location is not valid");
    break;
  case HLL_LOC_FORM_SYMB:
    compile_symbol(compiler, location);
    emit_op(compiler->bytecode, HLL_BYTECODE_FIND);
    compile_eval_expression(compiler, value);
    emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
    break;
  case HLL_LOC_FORM_CAR:
    assert(ast_list_length(location) == 2);
    compile_eval_expression(compiler, location->as.cons.cdr->as.cons.car);
    compile_eval_expression(compiler, value);
    emit_op(compiler->bytecode, HLL_BYTECODE_SETCAR);
    break;
  case HLL_LOC_FORM_CDR:
    assert(ast_list_length(location) == 2);
    compile_eval_expression(compiler, location->as.cons.cdr->as.cons.car);
    compile_eval_expression(compiler, value);
    emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

static void compile_setf(hll_compiler *compiler, const hll_ast *args) {
  if (ast_list_length(args) < 1) {
    compiler_error(compiler, args, "'setf' expects at least 1 argument");
    return;
  }

  hll_ast *location = args->as.cons.car;
  hll_ast *value = args->as.cons.cdr;
  if (value->kind == HLL_AST_CONS) {
    assert(value->as.cons.cdr->kind == HLL_AST_NIL);
    value = value->as.cons.car;
  }

  compile_set_location(compiler, location, value);
}

static void compile_setcar(hll_compiler *compiler, const hll_ast *args) {
  if (ast_list_length(args) != 2) {
    compiler_error(compiler, args, "'setcar' expects exactly 2 arguments");
    return;
  }

  hll_ast *location = args->as.cons.car;
  hll_ast *value = args->as.cons.cdr;
  if (value->kind == HLL_AST_CONS) {
    assert(value->as.cons.cdr->kind == HLL_AST_NIL);
    value = value->as.cons.car;
  }

  compile_eval_expression(compiler, location);
  compile_eval_expression(compiler, value);
  emit_op(compiler->bytecode, HLL_BYTECODE_SETCAR);
}

static void compile_setcdr(hll_compiler *compiler, const hll_ast *args) {
  if (ast_list_length(args) != 2) {
    compiler_error(compiler, args, "'setcar' expects exactly 2 arguments");
    return;
  }

  hll_ast *location = args->as.cons.car;
  hll_ast *value = args->as.cons.cdr;
  if (value->kind == HLL_AST_CONS) {
    assert(value->as.cons.cdr->kind == HLL_AST_NIL);
    value = value->as.cons.car;
  }

  compile_eval_expression(compiler, location);
  compile_eval_expression(compiler, value);
  emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
}

static void compile_defvar(hll_compiler *compiler, const hll_ast *args) {
  if (ast_list_length(args) < 1) {
    compiler_error(compiler, args, "'defvar' expects at least 1 argument");
    return;
  }

  hll_ast *name = args->as.cons.car;
  if (name->kind != HLL_AST_SYMB) {
    compiler_error(compiler, name, "'defvar' name should be a symbol");
    return;
  }
  hll_ast *value = args->as.cons.cdr;
  if (value->kind == HLL_AST_CONS) {
    assert(value->as.cons.cdr->kind == HLL_AST_NIL);
    value = value->as.cons.car;
  }

  compile_expression(compiler, name);
  compile_eval_expression(compiler, value);
  emit_op(compiler->bytecode, HLL_BYTECODE_LET);
}

static void compile_list(hll_compiler *compiler, const hll_ast *args) {
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  for (const hll_ast *arg = args; arg->kind != HLL_AST_NIL;
       arg = arg->as.cons.cdr) {
    assert(arg->kind == HLL_AST_CONS);
    hll_ast *obj = arg->as.cons.car;
    compile_eval_expression(compiler, obj);
    emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  }
  emit_op(compiler->bytecode, HLL_BYTECODE_POP);
}

static void compile_cons(hll_compiler *compiler, const hll_ast *args) {
  if (ast_list_length(args) != 2) {
    compiler_error(compiler, args, "'cons' expects exactly 2 arguments");
    return;
  }

  hll_ast *car = args->as.cons.car;
  hll_ast *cdr = args->as.cons.cdr->as.cons.car;
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  compile_eval_expression(compiler, car);
  emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  compile_eval_expression(compiler, cdr);
  emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
  emit_op(compiler->bytecode, HLL_BYTECODE_POP);
}

static bool compile_function(hll_compiler *compiler, const hll_ast *params,
                             const hll_ast *body, const char *name,
                             uint16_t *idx) {
  hll_bytecode *bytecode = calloc(1, sizeof(hll_bytecode));
  hll_compiler new_compiler = {0};
  new_compiler.vm = compiler->vm;
  new_compiler.bytecode = bytecode;
  for (const hll_ast *prog = body; prog->kind == HLL_AST_CONS;
       prog = prog->as.cons.cdr) {
    compile_eval_expression(&new_compiler, prog->as.cons.car);
    if (prog->as.cons.cdr->kind != HLL_AST_NIL) {
      emit_op(new_compiler.bytecode, HLL_BYTECODE_POP);
    }
  }
  emit_op(new_compiler.bytecode, HLL_BYTECODE_END);
  if (new_compiler.has_errors) {
    compiler->has_errors = true;
    return true;
  }

  hll_obj *param_list = NULL;
  hll_obj *param_list_tail = NULL;
  for (const hll_ast *obj = params; obj->kind == HLL_AST_CONS;
       obj = obj->as.cons.cdr) {
    hll_ast *car = obj->as.cons.car;
    if (car->kind != HLL_AST_SYMB) {
      compiler_error(compiler, car, "function param name is not a symbol");
      return true;
    }

    hll_obj *cons =
        hll_new_cons(compiler->vm,
                     bytecode->constant_pool[add_symbol_and_return_its_index(
                         &new_compiler, car->as.symb.str, car->as.symb.length)],
                     compiler->vm->nil);
    if (param_list == NULL) {
      param_list = param_list_tail = cons;
    } else {
      hll_unwrap_cons(param_list_tail)->cdr = cons;
      param_list_tail = cons;
    }
  }

  if (param_list == NULL) {
    param_list = compiler->vm->nil;
  }

  hll_obj *func = hll_new_func(compiler->vm, param_list, bytecode, name);
  hll_sb_push(compiler->bytecode->constant_pool, func);
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  *idx = narrowed;

  return false;
}

static void compile_defun(hll_compiler *compiler, const hll_ast *args) {
  if (ast_list_length(args) < 3) {
    compiler_error(compiler, args, "'defun' expects at least 3 arguments");
    return;
  }

  hll_ast *name = args->as.cons.car;
  if (name->kind != HLL_AST_SYMB) {
    compiler_error(compiler, name, "'defun' name should be a symbol");
    return;
  }

  args = args->as.cons.cdr;
  const hll_ast *params = args->as.cons.car;
  args = args->as.cons.cdr;
  const hll_ast *body = args;

  compile_expression(compiler, name);

  uint16_t function_idx;
  if (compile_function(compiler, params, body, "func", &function_idx)) {
    return;
  }

  emit_op(compiler->bytecode, HLL_BYTECODE_MAKEFUN);
  emit_u16(compiler->bytecode, function_idx);
  emit_op(compiler->bytecode, HLL_BYTECODE_LET);
}

static void compile_form(hll_compiler *compiler, const hll_ast *args,
                         hll_form_kind kind) {
  if (kind != HLL_FORM_REGULAR) {
    assert(args->kind == HLL_AST_CONS);
    args = args->as.cons.cdr;
  }

  switch (kind) {
  case HLL_FORM_REGULAR:
    compile_function_call(compiler, args);
    break;
  case HLL_FORM_QUOTE:
    compile_quote(compiler, args);
    break;
  case HLL_FORM_IF:
    compile_if(compiler, args);
    break;
  case HLL_FORM_LET:
    compile_let(compiler, args);
    break;
    //  case HLL_FORM_LAMBDA:
    //    compile_lambda(compiler, args);
    //    break;
  case HLL_FORM_SETF:
    compile_setf(compiler, args);
    break;
  case HLL_FORM_DEFVAR:
    compile_defvar(compiler, args);
    break;
  case HLL_FORM_LIST:
    compile_list(compiler, args);
    break;
  case HLL_FORM_CONS:
    compile_cons(compiler, args);
    break;
  case HLL_FORM_SETCAR:
    compile_setcar(compiler, args);
    break;
  case HLL_FORM_SETCDR:
    compile_setcdr(compiler, args);
    break;
  case HLL_FORM_DEFUN:
    compile_defun(compiler, args);
    break;
#define HLL_CAR_CDR(_lower, _upper)                                            \
  case HLL_FORM_C##_upper##R:                                                  \
    compile_c##_lower##r(compiler, args);                                      \
    break;
    HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR
  }
}

static void compile_eval_expression(hll_compiler *compiler,
                                    const hll_ast *ast) {
  switch (ast->kind) {
  case HLL_AST_NIL:
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    break;
  case HLL_AST_TRUE:
    emit_op(compiler->bytecode, HLL_BYTECODE_TRUE);
    break;
  case HLL_AST_INT:
    emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
    emit_u16(compiler->bytecode,
             add_int_constant_and_return_its_index(compiler, ast->as.num));
    break;
  case HLL_AST_CONS: {
    hll_ast *fn = ast->as.cons.car;
    hll_form_kind kind = HLL_FORM_REGULAR;
    if (fn->kind == HLL_AST_SYMB) {
      kind = get_form_kind(fn->as.symb.str);
    }

    compile_form(compiler, ast, kind);
  } break;
  case HLL_AST_SYMB:
    compile_symbol(compiler, ast);
    emit_op(compiler->bytecode, HLL_BYTECODE_FIND);
    emit_op(compiler->bytecode, HLL_BYTECODE_CDR);
    break;
  default:
    assert(!"Unreachable");
    break;
  }
}

// Compiles expression as getting its value.
// Does not evaluate it.
// After it one value is located on top of the stack.
static void compile_expression(hll_compiler *compiler, const hll_ast *ast) {
  switch (ast->kind) {
  case HLL_AST_NIL:
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    break;
  case HLL_AST_TRUE:
    emit_op(compiler->bytecode, HLL_BYTECODE_TRUE);
    break;
  case HLL_AST_INT:
    emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
    emit_u16(compiler->bytecode,
             add_int_constant_and_return_its_index(compiler, ast->as.num));
    break;
  case HLL_AST_CONS: {
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);

    const hll_ast *obj = ast;
    while (obj->kind != HLL_AST_NIL) {
      assert(obj->kind == HLL_AST_CONS);
      compile_expression(compiler, obj->as.cons.car);
      emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);

      hll_ast *cdr = obj->as.cons.cdr;
      if (cdr->kind != HLL_AST_NIL && cdr->kind != HLL_AST_CONS) {
        compile_expression(compiler, cdr);
        emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
        break;
      }

      obj = cdr;
    }
    // pop the tail
    emit_op(compiler->bytecode, HLL_BYTECODE_POP);
  } break;
  case HLL_AST_SYMB:
    compile_symbol(compiler, ast);
    break;
  default:
    assert(!"Unreachable");
    break;
  }
}

void hll_compile_ast(hll_compiler *compiler, const hll_ast *ast) {
  if (ast->kind == HLL_AST_NIL) {
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  } else {
    for (const hll_ast *obj = ast; obj->kind != HLL_AST_NIL;
         obj = obj->as.cons.cdr) {
      assert(obj->kind == HLL_AST_CONS);
      hll_ast *toplevel = obj->as.cons.car;
      switch (toplevel->kind) {
      case HLL_AST_NIL:
      case HLL_AST_TRUE:
      case HLL_AST_INT:
      case HLL_AST_SYMB:
        compile_expression(compiler, toplevel);
        break;
      case HLL_AST_CONS:
        compile_eval_expression(compiler, toplevel);
        break;
      default:
        assert(!"Unreachable");
        break;
      }

      if (obj->as.cons.cdr->kind != HLL_AST_NIL) {
        emit_op(compiler->bytecode, HLL_BYTECODE_POP);
      }
    }
  }
  emit_op(compiler->bytecode, HLL_BYTECODE_END);
}
