#include "hll_compiler.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_mem.h"
#include "hll_obj.h"
#include "hll_util.h"
#include "hll_vm.h"

struct hll_obj *hll_compile(struct hll_vm *vm, const char *source) {
  struct hll_lexer lexer;
  hll_lexer_init(&lexer, source, vm);
  struct hll_reader reader;
  hll_reader_init(&reader, &lexer, vm);

  ++vm->forbid_gc;
  struct hll_obj *ast = hll_read_ast(&reader);
  struct hll_compiler compiler;
  hll_compiler_init(&compiler, vm, vm->env);
  struct hll_obj *result = hll_compile_ast(&compiler, ast);
  --vm->forbid_gc;

  if (lexer.has_errors || reader.has_errors || compiler.has_errors) {
    return NULL;
  }

  return result;
}

enum hll_lexer_equivalence_class {
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
};

static inline enum hll_lexer_equivalence_class
get_equivalence_class(char symb) {
  enum hll_lexer_equivalence_class eqc = HLL_LEX_EQC_OTHER;
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

enum hll_lexer_state {
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
};

static inline enum hll_lexer_state
get_next_state(enum hll_lexer_state state,
               enum hll_lexer_equivalence_class eqc) {
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
static void lexer_error(struct hll_lexer *lexer, const char *fmt, ...) {
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

void hll_lexer_init(struct hll_lexer *lexer, const char *input,
                    struct hll_vm *vm) {
  memset(lexer, 0, sizeof(struct hll_lexer));
  lexer->vm = vm;
  lexer->cursor = lexer->input = input;
}

void hll_lexer_next(struct hll_lexer *lexer) {
  // Pull it from structure so there is better chance in ends up in
  // register.
  const char *cursor = lexer->cursor;
  const char *token_start = cursor;
  enum hll_lexer_state state = HLL_LEX_START;
  do {
    // Basic state machine-based lexing.
    // It has been decided to use character equivalence classes in hope that
    // lexical grammar stays simple. In cases like C grammar, there are a
    // lot of special cases that are better be handled by per-codepoint
    // lookup.
    enum hll_lexer_equivalence_class eqc = get_equivalence_class(*cursor++);
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

void hll_reader_init(struct hll_reader *reader, struct hll_lexer *lexer,
                     struct hll_vm *vm) {
  memset(reader, 0, sizeof(struct hll_reader));
  reader->lexer = lexer;
  reader->vm = vm;
}

HLL_ATTR(format(printf, 2, 3))
static void reader_error(struct hll_reader *reader, const char *fmt, ...) {
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

static void peek_token(struct hll_reader *reader) {
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

static void eat_token(struct hll_reader *reader) {
  reader->should_return_old_token = false;
}

static struct hll_obj *read_expr(struct hll_reader *reader);

static struct hll_obj *read_list(struct hll_reader *reader) {
  peek_token(reader);
  assert(reader->lexer->next.kind == HLL_TOK_LPAREN);
  eat_token(reader);
  // Now we expect at least one list element followed by others ending either
  // with right paren or dot, element and right paren Now check if we don't
  // have first element
  peek_token(reader);
  if (reader->lexer->next.kind == HLL_TOK_RPAREN) {
    eat_token(reader);
    return reader->vm->nil;
  }

  struct hll_obj *list_head;
  struct hll_obj *list_tail;
  list_head = list_tail =
      hll_new_cons(reader->vm, read_expr(reader), reader->vm->nil);

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
      hll_unwrap_cons(list_tail)->cdr = read_expr(reader);

      peek_token(reader);
      if (reader->lexer->next.kind != HLL_TOK_RPAREN) {
        reader_error(reader, "Missing closing paren after dot");
        return list_head;
      }
      eat_token(reader);
      return list_head;
    }

    struct hll_obj *ast = read_expr(reader);
    hll_unwrap_cons(list_tail)->cdr =
        hll_new_cons(reader->vm, ast, reader->vm->nil);
    list_tail = hll_unwrap_cdr(list_tail);
  }

  assert(!"Unreachable");
}

static struct hll_obj *read_expr(struct hll_reader *reader) {
  struct hll_obj *ast = reader->vm->nil;
  peek_token(reader);
  switch (reader->lexer->next.kind) {
  case HLL_TOK_EOF:
    break;
  case HLL_TOK_INT:
    eat_token(reader);
    ast = hll_new_num(reader->vm, reader->lexer->next.value);
    break;
  case HLL_TOK_SYMB:
    eat_token(reader);
    if (reader->lexer->next.length == 1 &&
        reader->lexer->input[reader->lexer->next.offset] == 't') {
      ast = reader->vm->true_;
      break;
    }

    ast = hll_new_symbol(reader->vm,
                         reader->lexer->input + reader->lexer->next.offset,
                         reader->lexer->next.length);
    break;
  case HLL_TOK_LPAREN:
    ast = read_list(reader);
    break;
  case HLL_TOK_QUOTE: {
    eat_token(reader);
    ast = hll_new_cons(
        reader->vm, reader->vm->quote_symb,
        hll_new_cons(reader->vm, read_expr(reader), reader->vm->nil));
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

struct hll_obj *hll_read_ast(struct hll_reader *reader) {
  struct hll_obj *list_head = NULL;
  struct hll_obj *list_tail = NULL;

  for (;;) {
    peek_token(reader);
    if (reader->lexer->next.kind == HLL_TOK_EOF) {
      break;
    }

    struct hll_obj *ast = read_expr(reader);
    struct hll_obj *cons = hll_new_cons(reader->vm, ast, reader->vm->nil);

    if (list_head == NULL) {
      list_head = list_tail = cons;
    } else {
      hll_unwrap_cons(list_tail)->cdr = cons;
      list_tail = cons;
    }
  }

  if (list_head == NULL) {
    list_head = reader->vm->nil;
  }

  return list_head;
}

void hll_compiler_init(struct hll_compiler *compiler, struct hll_vm *vm,
                       struct hll_obj *env) {
  memset(compiler, 0, sizeof(struct hll_compiler));
  compiler->vm = vm;
  compiler->env = env;
  compiler->bytecode = hll_alloc(sizeof(struct hll_bytecode));
}

HLL_ATTR(format(printf, 3, 4))
static void compiler_error(struct hll_compiler *compiler, struct hll_obj *ast,
                           const char *fmt, ...) {
  (void)ast;
  compiler->has_errors = true;
  if (compiler->vm == NULL) {
    return;
  }

  char buffer[4096];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  // TODO: Length, offset
  hll_report_error(compiler->vm, 0, 0, buffer);
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
enum hll_form_kind {
  // Regular form. This means function invocation, either ffi or
  // language-defined
  HLL_FORM_REGULAR,
  // Quote returns unevaluated argument
  HLL_FORM_QUOTE,
  // If checks condition and executes one of its arms
  HLL_FORM_IF,
  // Lambda constructs a new function
  // Set sets value pointed to by location defined by first argument
  // as second argument. First argument is of special kind of form,
  // which denotes location and requires special handling from compiler.
  // It is usually implemented via macros using functions for setting different
  // kinds of locations, but we handle all of that here.
  HLL_FORM_SET,
  // let form
  HLL_FORM_LET,
  // list returns all of its evaluated arguments
  HLL_FORM_LIST,
  // makes new cons
  HLL_FORM_CONS,
  // Sets car. This is made a compiler inlined function because we have the same
  // bytecode instruction
  HLL_FORM_SETCAR,
  // Sets cdr. Similar to car
  HLL_FORM_SETCDR,
  HLL_FORM_PROGN,
  HLL_FORM_LAMBDA,
  HLL_FORM_OR,
  HLL_FORM_AND,
  HLL_FORM_DEFINE,
  HLL_FORM_DEFMACRO,
  HLL_FORM_MACROEXPAND,
#define HLL_CAR_CDR(_, _letters) HLL_FORM_C##_letters##R,
  HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR
};

static enum hll_form_kind get_form_kind(const char *symb) {
  enum hll_form_kind kind = HLL_FORM_REGULAR;
  if (strcmp(symb, "quote") == 0) {
    kind = HLL_FORM_QUOTE;
  } else if (strcmp(symb, "if") == 0) {
    kind = HLL_FORM_IF;
  } else if (strcmp(symb, "set!") == 0) {
    kind = HLL_FORM_SET;
  } else if (strcmp(symb, "let") == 0) {
    kind = HLL_FORM_LET;
  } else if (strcmp(symb, "list") == 0) {
    kind = HLL_FORM_LIST;
  } else if (strcmp(symb, "cons") == 0) {
    kind = HLL_FORM_CONS;
  } else if (strcmp(symb, "setcar!") == 0) {
    kind = HLL_FORM_SETCAR;
  } else if (strcmp(symb, "setcdr!") == 0) {
    kind = HLL_FORM_SETCDR;
  } else if (strcmp(symb, "define") == 0) {
    kind = HLL_FORM_DEFINE;
  } else if (strcmp(symb, "progn") == 0) {
    kind = HLL_FORM_PROGN;
  } else if (strcmp(symb, "lambda") == 0) {
    kind = HLL_FORM_LAMBDA;
  } else if (strcmp(symb, "or") == 0) {
    kind = HLL_FORM_OR;
  } else if (strcmp(symb, "and") == 0) {
    kind = HLL_FORM_AND;
  } else if (strcmp(symb, "defmacro") == 0) {
    kind = HLL_FORM_DEFMACRO;
  } else if (strcmp(symb, "macroexpand") == 0) {
    kind = HLL_FORM_MACROEXPAND;
  }
#define HLL_CAR_CDR(_lower, _upper)                                            \
  else if (strcmp(symb, "c" #_lower "r") == 0) {                               \
    kind = HLL_FORM_C##_upper##R;                                              \
  }
  HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR

  return kind;
}

static size_t get_current_op_idx(struct hll_bytecode *bytecode) {
  return hll_sb_len(bytecode->ops);
}

size_t emit_u8(struct hll_bytecode *bytecode, uint8_t byte) {
  size_t idx = get_current_op_idx(bytecode);
  hll_sb_push(bytecode->ops, byte);
  return idx;
}

size_t emit_op(struct hll_bytecode *bytecode, enum hll_bytecode_op op) {
  assert(op <= 0xFF);
  return emit_u8(bytecode, op);
}

static void write_u16_be(uint8_t *data, uint16_t value) {
  *data++ = (value >> 8) & 0xFF;
  *data = value & 0xFF;
}

static size_t emit_u16(struct hll_bytecode *bytecode, uint16_t value) {
  size_t idx = emit_u8(bytecode, (value >> 8) & 0xFF);
  emit_u8(bytecode, value & 0xFF);
  return idx;
}

static uint16_t
add_int_constant_and_return_its_index(struct hll_compiler *compiler,
                                      double value) {
  for (size_t i = 0; i < hll_sb_len(compiler->bytecode->constant_pool); ++i) {
    struct hll_obj *test = compiler->bytecode->constant_pool[i];
    if (hll_get_obj_kind(test) == HLL_OBJ_NUM &&
        hll_unwrap_num(test) == value) {
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

static uint16_t add_symbol_and_return_its_index(struct hll_compiler *compiler,
                                                const char *symb_,
                                                size_t length) {
  for (size_t i = 0; i < hll_sb_len(compiler->bytecode->constant_pool); ++i) {
    struct hll_obj *test = compiler->bytecode->constant_pool[i];
    if (hll_get_obj_kind(test) == HLL_OBJ_SYMB &&
        strcmp(hll_unwrap_zsymb(test), symb_) == 0) {
      uint16_t narrowed = i;
      assert(i == narrowed);
      return narrowed;
    }
  }

  struct hll_obj *symb = hll_new_symbol(compiler->vm, symb_, length);
  hll_sb_push(compiler->bytecode->constant_pool, symb);
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  return narrowed;
}

static void compile_symbol(struct hll_compiler *compiler, struct hll_obj *ast) {
  assert(hll_get_obj_kind(ast) == HLL_OBJ_SYMB);
  emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
  emit_u16(compiler->bytecode,
           add_symbol_and_return_its_index(compiler, hll_unwrap_zsymb(ast),
                                           hll_unwrap_symb(ast)->length));
}

static void compile_expression(struct hll_compiler *compiler,
                               struct hll_obj *ast);
static void compile_eval_expression(struct hll_compiler *compiler,
                                    struct hll_obj *ast);

static void compile_function_call_internal(struct hll_compiler *compiler,
                                           struct hll_obj *list) {
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  for (struct hll_obj *arg = list; hll_get_obj_kind(arg) != HLL_OBJ_NIL;
       arg = hll_unwrap_cdr(arg)) {
    assert(hll_get_obj_kind(arg) == HLL_OBJ_CONS);
    struct hll_obj *obj = hll_unwrap_car(arg);
    compile_eval_expression(compiler, obj);
    emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  }
  emit_op(compiler->bytecode, HLL_BYTECODE_POP);
  emit_op(compiler->bytecode, HLL_BYTECODE_CALL);
}

static struct hll_obj *expand_macro(struct hll_compiler *compiler,
                                    struct hll_obj *macro,
                                    struct hll_obj *args) {
  if (hll_get_obj_kind(macro) != HLL_OBJ_SYMB) {
    return NULL;
  }

  struct hll_obj *macro_body = hll_find_var(compiler->vm, compiler->env, macro);
  if (macro_body == NULL ||
      hll_get_obj_kind(hll_unwrap_cdr(macro_body)) != HLL_OBJ_MACRO) {
    return NULL;
  } else {
    macro_body = hll_unwrap_cdr(macro_body);
  }

  return hll_expand_macro(compiler->vm, macro_body, (struct hll_obj *)args);
}

static void compile_function_call(struct hll_compiler *compiler,
                                  struct hll_obj *list) {
  struct hll_obj *fn = hll_unwrap_car(list);
  struct hll_obj *args = hll_unwrap_cdr(list);
  struct hll_obj *expanded;
  if ((expanded = expand_macro(compiler, fn, args))) {
    compile_eval_expression(compiler, expanded);
    return;
  }
  compile_eval_expression(compiler, fn);
  compile_function_call_internal(compiler, args);
}

static void compile_quote(struct hll_compiler *compiler, struct hll_obj *args) {
  if (hll_get_obj_kind(args) != HLL_OBJ_CONS) {
    compiler_error(compiler, args, "'quote' form must have an argument");
    return;
  }
  if (hll_get_obj_kind(hll_unwrap_cdr(args)) != HLL_OBJ_NIL) {
    compiler_error(compiler, args,
                   "'quote' form must have exactly one argument");
  }
  compile_expression(compiler, hll_unwrap_car(args));
}

static void compile_progn(struct hll_compiler *compiler, struct hll_obj *prog) {
  if (hll_get_obj_kind(prog) == HLL_OBJ_NIL) {
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    return;
  }

  for (; hll_get_obj_kind(prog) == HLL_OBJ_CONS; prog = hll_unwrap_cdr(prog)) {
    compile_eval_expression(compiler, hll_unwrap_car(prog));
    if (hll_get_obj_kind(hll_unwrap_cdr(prog)) != HLL_OBJ_NIL) {
      emit_op(compiler->bytecode, HLL_BYTECODE_POP);
    }
  }
}

static void compile_if(struct hll_compiler *compiler, struct hll_obj *args) {
  size_t length = hll_list_length(args);
  if (length < 2) {
    compiler_error(compiler, args, "'if' form expects at least 2 arguments");
    return;
  }

  struct hll_obj *cond = hll_unwrap_car(args);
  compile_eval_expression(compiler, cond);
  struct hll_obj *pos_arm = hll_unwrap_cdr(args);
  assert(hll_get_obj_kind(pos_arm) == HLL_OBJ_CONS);
  struct hll_obj *neg_arm = hll_unwrap_cdr(pos_arm);
  pos_arm = hll_unwrap_car(pos_arm);

  emit_op(compiler->bytecode, HLL_BYTECODE_JN);
  size_t jump_false = emit_u16(compiler->bytecode, 0);
  compile_eval_expression(compiler, pos_arm);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  emit_op(compiler->bytecode, HLL_BYTECODE_JN);
  size_t jump_out = emit_u16(compiler->bytecode, 0);
  write_u16_be(compiler->bytecode->ops + jump_false,
               get_current_op_idx(compiler->bytecode) - jump_false - 2);
  compile_progn(compiler, neg_arm);
  write_u16_be(compiler->bytecode->ops + jump_out,
               get_current_op_idx(compiler->bytecode) - jump_out - 2);
}

static void compile_let(struct hll_compiler *compiler, struct hll_obj *args) {
  size_t length = hll_list_length(args);
  if (length < 1) {
    compiler_error(compiler, args,
                   "'let' special form requires variable declarations");
    return;
  }

  emit_op(compiler->bytecode, HLL_BYTECODE_PUSHENV);
  for (struct hll_obj *let = hll_unwrap_car(args);
       hll_get_obj_kind(let) != HLL_OBJ_NIL; let = hll_unwrap_cdr(let)) {
    struct hll_obj *pair = hll_unwrap_car(let);
    assert(hll_get_obj_kind(pair) == HLL_OBJ_CONS);
    struct hll_obj *name = hll_unwrap_car(pair);
    struct hll_obj *value = hll_unwrap_cdr(pair);
    if (hll_get_obj_kind(value) == HLL_OBJ_CONS) {
      assert(hll_get_obj_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
      value = hll_unwrap_car(value);
    }

    assert(hll_get_obj_kind(name) == HLL_OBJ_SYMB);
    compile_expression(compiler, name);
    compile_eval_expression(compiler, value);
    emit_op(compiler->bytecode, HLL_BYTECODE_LET);
    emit_op(compiler->bytecode, HLL_BYTECODE_POP);
  }

  compile_progn(compiler, hll_unwrap_cdr(args));
  emit_op(compiler->bytecode, HLL_BYTECODE_POPENV);
}

#define HLL_CAR_CDR(_lower, _)                                                 \
  static void compile_c##_lower##r(struct hll_compiler *compiler,              \
                                   struct hll_obj *args) {                     \
    if (hll_list_length(args) != 1) {                                          \
      compiler_error(compiler, args,                                           \
                     "'c" #_lower "r' expects exactly 1 argument");            \
    } else {                                                                   \
      compile_eval_expression(compiler, hll_unwrap_car(args));                 \
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

enum hll_location_form {
  HLL_LOC_NONE,
  HLL_LOC_FORM_SYMB,
  HLL_LOC_FORM_NTH,
#define HLL_CAR_CDR(_, _letters) HLL_LOC_FORM_C##_letters##R,
  HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR
};

static enum hll_location_form get_location_form(struct hll_obj *location) {
  enum hll_location_form kind = HLL_LOC_NONE;
  if (hll_get_obj_kind(location) == HLL_OBJ_SYMB) {
    kind = HLL_LOC_FORM_SYMB;
  } else if (hll_get_obj_kind(location) == HLL_OBJ_CONS) {
    struct hll_obj *first = hll_unwrap_car(location);
    if (hll_get_obj_kind(first) == HLL_OBJ_SYMB) {
      const char *symb = hll_unwrap_zsymb(first);
      if (strcmp(symb, "nth") == 0) {
        kind = HLL_LOC_FORM_NTH;
      }
#define HLL_CAR_CDR(_lower, _upper)                                            \
  else if (strcmp(symb, "c" #_lower "r") == 0) {                               \
    kind = HLL_LOC_FORM_C##_upper##R;                                          \
  }
      HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR
    }
  }

  return kind;
}

static void compile_set_location(struct hll_compiler *compiler,
                                 struct hll_obj *location,
                                 struct hll_obj *value) {
  enum hll_location_form kind = get_location_form(location);
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
  case HLL_LOC_FORM_NTH: {
    if (hll_list_length(location) != 3) {
      compiler_error(compiler, location, "'nth' expects exactly 2 arguments");
      break;
    }
    // get the nth function
    compile_symbol(compiler, compiler->vm->nthcdr_symb);
    emit_op(compiler->bytecode, HLL_BYTECODE_FIND);
    emit_op(compiler->bytecode, HLL_BYTECODE_CDR);
    // call nth
    compile_function_call_internal(compiler, hll_unwrap_cdr(location));
    compile_eval_expression(compiler, value);
    emit_op(compiler->bytecode, HLL_BYTECODE_SETCAR);
  } break;
#define HLL_CAR_CDR(_lower, _upper)                                            \
  case HLL_LOC_FORM_C##_upper##R: {                                            \
    if (hll_list_length(location) != 2) {                                      \
      compiler_error(compiler, location,                                       \
                     "'c" #_lower "r' expects exactly 1 argument");            \
      break;                                                                   \
    }                                                                          \
    compile_eval_expression(compiler,                                          \
                            hll_unwrap_car(hll_unwrap_cdr(location)));         \
    const char *ops = #_lower;                                                 \
    const char *op = ops + sizeof(#_lower) - 2;                                \
    for (;;) {                                                                 \
      if (op == ops) {                                                         \
        compile_eval_expression(compiler, value);                              \
        if (*op == 'a') {                                                      \
          emit_op(compiler->bytecode, HLL_BYTECODE_SETCAR);                    \
        } else if (*op == 'd') {                                               \
          emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);                    \
        } else {                                                               \
          HLL_UNREACHABLE;                                                     \
        }                                                                      \
        break;                                                                 \
      } else {                                                                 \
        if (*op == 'a') {                                                      \
          emit_op(compiler->bytecode, HLL_BYTECODE_CAR);                       \
        } else if (*op == 'd') {                                               \
          emit_op(compiler->bytecode, HLL_BYTECODE_CDR);                       \
        } else {                                                               \
          HLL_UNREACHABLE;                                                     \
        }                                                                      \
      }                                                                        \
      --op;                                                                    \
    }                                                                          \
  } break;
    HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR
  default:
    HLL_UNREACHABLE;
    break;
  }
}

static void compile_setf(struct hll_compiler *compiler, struct hll_obj *args) {
  if (hll_list_length(args) < 1) {
    compiler_error(compiler, args, "'set!' expects at least 1 argument");
    return;
  }

  struct hll_obj *location = hll_unwrap_car(args);
  struct hll_obj *value = hll_unwrap_cdr(args);
  if (hll_get_obj_kind(value) == HLL_OBJ_CONS) {
    assert(hll_get_obj_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
    value = hll_unwrap_car(value);
  }

  compile_set_location(compiler, location, value);
}

static void compile_setcar(struct hll_compiler *compiler,
                           struct hll_obj *args) {
  if (hll_list_length(args) != 2) {
    compiler_error(compiler, args, "'setcar!' expects exactly 2 arguments");
    return;
  }

  struct hll_obj *location = hll_unwrap_car(args);
  struct hll_obj *value = hll_unwrap_cdr(args);
  if (hll_get_obj_kind(value) == HLL_OBJ_CONS) {
    assert(hll_get_obj_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
    value = hll_unwrap_car(value);
  }

  compile_eval_expression(compiler, location);
  compile_eval_expression(compiler, value);
  emit_op(compiler->bytecode, HLL_BYTECODE_SETCAR);
}

static void compile_setcdr(struct hll_compiler *compiler,
                           struct hll_obj *args) {
  if (hll_list_length(args) != 2) {
    compiler_error(compiler, args, "'setcar!' expects exactly 2 arguments");
    return;
  }

  struct hll_obj *location = hll_unwrap_car(args);
  struct hll_obj *value = hll_unwrap_cdr(args);
  if (hll_get_obj_kind(value) == HLL_OBJ_CONS) {
    assert(hll_get_obj_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
    value = hll_unwrap_car(value);
  }

  compile_eval_expression(compiler, location);
  compile_eval_expression(compiler, value);
  emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
}

static void compile_list(struct hll_compiler *compiler, struct hll_obj *args) {
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  for (struct hll_obj *arg = args; hll_get_obj_kind(arg) != HLL_OBJ_NIL;
       arg = hll_unwrap_cdr(arg)) {
    assert(hll_get_obj_kind(arg) == HLL_OBJ_CONS);
    struct hll_obj *obj = hll_unwrap_car(arg);
    compile_eval_expression(compiler, obj);
    emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  }
  emit_op(compiler->bytecode, HLL_BYTECODE_POP);
}

static void compile_cons(struct hll_compiler *compiler, struct hll_obj *args) {
  if (hll_list_length(args) != 2) {
    compiler_error(compiler, args, "'cons' expects exactly 2 arguments");
    return;
  }

  struct hll_obj *car = hll_unwrap_car(args);
  struct hll_obj *cdr = hll_unwrap_car(hll_unwrap_cdr(args));
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  compile_eval_expression(compiler, car);
  emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  compile_eval_expression(compiler, cdr);
  emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
  emit_op(compiler->bytecode, HLL_BYTECODE_POP);
}

static void add_symbol_to_function_param_list(
    struct hll_compiler *compiler, struct hll_obj *car,
    struct hll_obj **param_list, struct hll_obj **param_list_tail) {
  struct hll_obj *symb = car;
  if (hll_get_obj_kind(car) != HLL_OBJ_NIL) {
    uint16_t symb_idx = add_symbol_and_return_its_index(
        compiler, hll_unwrap_zsymb(car), hll_unwrap_symb(car)->length);
    symb = compiler->bytecode->constant_pool[symb_idx];
    assert(symb != NULL);
  }

  struct hll_obj *cons =
      hll_new_cons(compiler->vm, (struct hll_obj *)symb, compiler->vm->nil);
  if (*param_list == NULL) {
    *param_list = *param_list_tail = cons;
  } else {
    hll_unwrap_cons(*param_list_tail)->cdr = cons;
    *param_list_tail = cons;
  }
}

static struct hll_obj *compile_function_internal(struct hll_compiler *compiler,
                                                 struct hll_obj *params,
                                                 struct hll_obj *body,
                                                 const char *name,
                                                 bool is_macro) {
  (void)name;
  struct hll_compiler new_compiler = {0};
  hll_compiler_init(&new_compiler, compiler->vm, compiler->vm->env);
  struct hll_obj *compiled = hll_compile_ast(&new_compiler, body);
  if (new_compiler.has_errors) {
    compiler->has_errors = true;
    return NULL;
  }

  hll_sb_push(compiler->vm->temp_roots, compiled);

  struct hll_obj *param_list = NULL;
  struct hll_obj *param_list_tail = NULL;
  if (hll_get_obj_kind(params) == HLL_OBJ_SYMB) {
    add_symbol_to_function_param_list(&new_compiler, compiler->vm->nil,
                                      &param_list, &param_list_tail);
    add_symbol_to_function_param_list(&new_compiler, params, &param_list,
                                      &param_list_tail);
  } else {
    if (hll_get_obj_kind(params) != HLL_OBJ_NIL &&
        hll_get_obj_kind(params) != HLL_OBJ_CONS) {
      compiler_error(compiler, params, "param list must be a list");
      return NULL;
    }

    struct hll_obj *obj = params;
    for (; hll_get_obj_kind(obj) == HLL_OBJ_CONS; obj = hll_unwrap_cdr(obj)) {
      struct hll_obj *car = hll_unwrap_car(obj);
      if (hll_get_obj_kind(car) != HLL_OBJ_SYMB) {
        compiler_error(compiler, car, "function param name is not a symbol");
        return NULL;
      }

      add_symbol_to_function_param_list(&new_compiler, car, &param_list,
                                        &param_list_tail);
    }

    if (hll_get_obj_kind(obj) != HLL_OBJ_NIL) {
      if (hll_get_obj_kind(obj) != HLL_OBJ_SYMB) {
        compiler_error(compiler, obj, "function param name is not a symbol");
        return NULL;
      }
      assert(hll_get_obj_kind(param_list_tail) == HLL_OBJ_CONS);
      hll_unwrap_cons(param_list_tail)->cdr = (struct hll_obj *)obj;
    }
  }

  if (param_list == NULL) {
    param_list = compiler->vm->nil;
  }

  struct hll_obj *func;
  if (!is_macro) {
    func = compiled;
    hll_unwrap_func(func)->param_names = param_list;
  } else {
    func = compiled;
    // TODO: Remove ub
    *(enum hll_object_kind *)func = HLL_OBJ_MACRO;
#if 0
    struct hll_obj_func *unwrapped = hll_unwrap_func(compiled);
    func = hll_new_macro(compiler->vm, unwrapped->param_names,
                         unwrapped->bytecode, unwrapped->name);
#endif
    hll_unwrap_macro(func)->param_names = param_list;
  }

  (void)hll_sb_pop(compiler->vm->temp_roots); // compiled
  return func;
}

static bool compile_function(struct hll_compiler *compiler,
                             struct hll_obj *params, struct hll_obj *body,
                             const char *name, uint16_t *idx) {
  struct hll_obj *func =
      compile_function_internal(compiler, params, body, name, false);
  if (func == NULL) {
    return true;
  }

  hll_sb_push(compiler->bytecode->constant_pool, func);
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  *idx = narrowed;

  return false;
}

static void compile_lambda(struct hll_compiler *compiler,
                           struct hll_obj *args) {
  if (hll_list_length(args) < 2) {
    compiler_error(compiler, args, "'lambda' expects at least 2 arguments");
    return;
  }

  struct hll_obj *params = hll_unwrap_car(args);
  args = hll_unwrap_cdr(args);
  struct hll_obj *body = args;

  uint16_t function_idx;
  if (compile_function(compiler, params, body, "lambda", &function_idx)) {
    return;
  }

  emit_op(compiler->bytecode, HLL_BYTECODE_MAKEFUN);
  emit_u16(compiler->bytecode, function_idx);
}

static void compile_and(struct hll_compiler *compiler, struct hll_obj *args) {
  if (hll_list_length(args) == 0) {
    emit_op(compiler->bytecode, HLL_BYTECODE_TRUE);
    return;
  }

  size_t last_jump = 0;
  size_t original_idx = get_current_op_idx(compiler->bytecode);
  for (struct hll_obj *arg_slot = args;
       hll_get_obj_kind(arg_slot) == HLL_OBJ_CONS;
       arg_slot = hll_unwrap_cdr(arg_slot)) {
    struct hll_obj *item = hll_unwrap_car(arg_slot);
    compile_eval_expression(compiler, item);
    if (hll_get_obj_kind(hll_unwrap_cdr(arg_slot)) != HLL_OBJ_CONS) {
      emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    }
    emit_op(compiler->bytecode, HLL_BYTECODE_JN);
    last_jump = emit_u16(compiler->bytecode, 0);
  }
  size_t short_circuit = get_current_op_idx(compiler->bytecode);
  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  size_t total_out = get_current_op_idx(compiler->bytecode);

  assert(last_jump != 0);

  uint8_t *cursor = compiler->bytecode->ops + original_idx;
  while (cursor < compiler->bytecode->ops + total_out) {
    uint8_t op = *cursor++;
    if (op == HLL_BYTECODE_JN) {
      uint16_t value;
      if (cursor != compiler->bytecode->ops + last_jump) {
        value = short_circuit - (cursor - compiler->bytecode->ops) - 2;
      } else {
        value = total_out - (cursor - compiler->bytecode->ops) - 2;
      }
      write_u16_be(cursor, value);
    }
    cursor += hll_get_bytecode_op_body_size(op);
  }
}

static void compile_or(struct hll_compiler *compiler, struct hll_obj *args) {
  if (hll_list_length(args) == 0) {
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    return;
  }

  size_t previous_jump = 0;
  size_t original_idx = get_current_op_idx(compiler->bytecode);
  for (struct hll_obj *arg_slot = args;
       hll_get_obj_kind(arg_slot) == HLL_OBJ_CONS;
       arg_slot = hll_unwrap_cdr(arg_slot)) {
    struct hll_obj *item = hll_unwrap_car(arg_slot);

    if (arg_slot != args) {
      assert(previous_jump != 0);
      write_u16_be(compiler->bytecode->ops + previous_jump,
                   get_current_op_idx(compiler->bytecode) - previous_jump - 2);
      emit_op(compiler->bytecode, HLL_BYTECODE_POP);
    }

    compile_eval_expression(compiler, item);
    emit_op(compiler->bytecode, HLL_BYTECODE_DUP);
    emit_op(compiler->bytecode, HLL_BYTECODE_JN); // jump to next
    previous_jump = emit_u16(compiler->bytecode, 0);
    if (hll_get_obj_kind(hll_unwrap_cdr(arg_slot)) != HLL_OBJ_NIL) {
      emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
      emit_op(compiler->bytecode, HLL_BYTECODE_JN);
      emit_u16(compiler->bytecode, 0);
    }
  }

  size_t total_out = get_current_op_idx(compiler->bytecode);

  uint8_t *cursor = compiler->bytecode->ops + original_idx;
  while (cursor < compiler->bytecode->ops + total_out) {
    uint8_t op = *cursor++;
    if (op == HLL_BYTECODE_JN) {
      uint16_t current;
      memcpy(&current, cursor, sizeof(uint16_t));
      if (current == 0) {
        write_u16_be(cursor,
                     total_out - (cursor - compiler->bytecode->ops) - 2);
      }
    }
    cursor += hll_get_bytecode_op_body_size(op);
  }
}

static void process_defmacro(struct hll_compiler *compiler,
                             struct hll_obj *args) {
  if (hll_list_length(args) < 2) {
    compiler_error(compiler, args, "'defmacro' expects at least 2 arguments");
    return;
  }

  struct hll_obj *control = hll_unwrap_car(args);
  if (hll_get_obj_kind(control) != HLL_OBJ_CONS) {
    compiler_error(compiler, control,
                   "'defmacro' first argument must be list of macro name and "
                   "its parameters");
    return;
  }

  struct hll_obj *name = hll_unwrap_car(control);
  if (hll_get_obj_kind(name) != HLL_OBJ_SYMB) {
    compiler_error(compiler, name, "'defmacro' name should be a symbol");
    return;
  }

  emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  struct hll_obj *params = hll_unwrap_cdr(control);
  struct hll_obj *body = hll_unwrap_cdr(args);

  struct hll_obj *macro_expansion =
      compile_function_internal(compiler, params, body, "defmacro", true);
  hll_sb_push(compiler->vm->temp_roots, macro_expansion);
  if (macro_expansion != NULL) {
    // TODO: Test if macro with same name exists
    hll_add_variable(compiler->vm, compiler->env, name, macro_expansion);
  }
  (void)hll_sb_pop(compiler->vm->temp_roots);
}

static void compile_macroexpand(struct hll_compiler *compiler,
                                struct hll_obj *args) {
  if (hll_list_length(args) != 1) {
    compiler_error(compiler, args,
                   "'macroexpand' expects exactly one argument");
    return;
  }

  struct hll_obj *macro_list = hll_unwrap_car(args);
  if (hll_list_length(macro_list) < 1) {
    compiler_error(compiler, macro_list,
                   "'macroexpand' argument is not a list");
    return;
  }

  struct hll_obj *expanded = expand_macro(compiler, hll_unwrap_car(macro_list),
                                          hll_unwrap_cdr(macro_list));
  assert(expanded != NULL);
  compile_expression(compiler, expanded);
}

static void compile_define(struct hll_compiler *compiler,
                           struct hll_obj *args) {
  if (hll_list_length(args) == 0) {
    compiler_error(compiler, args,
                   "'define' form expects at least one argument");
    return;
  }

  struct hll_obj *decide = hll_unwrap_car(args);
  struct hll_obj *rest = hll_unwrap_cdr(args);

  if (hll_get_obj_kind(decide) == HLL_OBJ_CONS) {
    struct hll_obj *name = hll_unwrap_car(decide);
    if (hll_get_obj_kind(name) != HLL_OBJ_SYMB) {
      compiler_error(compiler, name, "'define' name should be a symbol");
      return;
    }

    struct hll_obj *params = hll_unwrap_cdr(decide);
    struct hll_obj *body = rest;

    compile_expression(compiler, name);

    uint16_t function_idx;
    if (compile_function(compiler, params, body, "define", &function_idx)) {
      return;
    }

    emit_op(compiler->bytecode, HLL_BYTECODE_MAKEFUN);
    emit_u16(compiler->bytecode, function_idx);
    emit_op(compiler->bytecode, HLL_BYTECODE_LET);
  } else if (hll_get_obj_kind(decide) == HLL_OBJ_SYMB) {
    struct hll_obj *value = rest;
    if (hll_get_obj_kind(value) == HLL_OBJ_CONS) {
      assert(hll_get_obj_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
      value = hll_unwrap_car(value);
    }

    compile_expression(compiler, decide);
    compile_eval_expression(compiler, value);
    emit_op(compiler->bytecode, HLL_BYTECODE_LET);
  } else {
    compiler_error(compiler, args,
                   "'define' first argument must either be a function name and "
                   "arguments or a variable name");
    return;
  }
}

static void compile_form(struct hll_compiler *compiler, struct hll_obj *args,
                         enum hll_form_kind kind) {
  if (kind != HLL_FORM_REGULAR) {
    assert(hll_get_obj_kind(args) == HLL_OBJ_CONS);
    args = hll_unwrap_cdr(args);
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
  case HLL_FORM_LAMBDA:
    compile_lambda(compiler, args);
    break;
  case HLL_FORM_SET:
    compile_setf(compiler, args);
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
  case HLL_FORM_DEFINE:
    compile_define(compiler, args);
    break;
  case HLL_FORM_PROGN:
    compile_progn(compiler, args);
    break;
#define HLL_CAR_CDR(_lower, _upper)                                            \
  case HLL_FORM_C##_upper##R:                                                  \
    compile_c##_lower##r(compiler, args);                                      \
    break;
    HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR
  case HLL_FORM_OR:
    compile_or(compiler, args);
    break;
  case HLL_FORM_AND:
    compile_and(compiler, args);
    break;
  case HLL_FORM_DEFMACRO:
    process_defmacro(compiler, args);
    break;
  case HLL_FORM_MACROEXPAND:
    compile_macroexpand(compiler, args);
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

static void compile_eval_expression(struct hll_compiler *compiler,
                                    struct hll_obj *ast) {
  switch (hll_get_obj_kind(ast)) {
  case HLL_OBJ_NIL:
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    break;
  case HLL_OBJ_TRUE:
    emit_op(compiler->bytecode, HLL_BYTECODE_TRUE);
    break;
  case HLL_OBJ_NUM:
    emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
    emit_u16(compiler->bytecode, add_int_constant_and_return_its_index(
                                     compiler, hll_unwrap_num(ast)));
    break;
  case HLL_OBJ_CONS: {
    struct hll_obj *fn = hll_unwrap_car(ast);
    enum hll_form_kind kind = HLL_FORM_REGULAR;
    if (hll_get_obj_kind(fn) == HLL_OBJ_SYMB) {
      kind = get_form_kind(hll_unwrap_zsymb(fn));
    }

    compile_form(compiler, ast, kind);
  } break;
  case HLL_OBJ_SYMB:
    compile_symbol(compiler, ast);
    emit_op(compiler->bytecode, HLL_BYTECODE_FIND);
    emit_op(compiler->bytecode, HLL_BYTECODE_CDR);
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

// Compiles expression as getting its value.
// Does not evaluate it.
// After it one value is located on top of the stack.
static void compile_expression(struct hll_compiler *compiler,
                               struct hll_obj *ast) {
  switch (hll_get_obj_kind(ast)) {
  case HLL_OBJ_NIL:
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    break;
  case HLL_OBJ_TRUE:
    emit_op(compiler->bytecode, HLL_BYTECODE_TRUE);
    break;
  case HLL_OBJ_NUM:
    emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
    emit_u16(compiler->bytecode, add_int_constant_and_return_its_index(
                                     compiler, hll_unwrap_num(ast)));
    break;
  case HLL_OBJ_CONS: {
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    emit_op(compiler->bytecode, HLL_BYTECODE_NIL);

    struct hll_obj *obj = ast;
    while (hll_get_obj_kind(obj) != HLL_OBJ_NIL) {
      assert(hll_get_obj_kind(obj) == HLL_OBJ_CONS);
      compile_expression(compiler, hll_unwrap_car(obj));
      emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);

      struct hll_obj *cdr = hll_unwrap_cdr(obj);
      if (hll_get_obj_kind(cdr) != HLL_OBJ_NIL &&
          hll_get_obj_kind(cdr) != HLL_OBJ_CONS) {
        compile_expression(compiler, cdr);
        emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
        break;
      }

      obj = cdr;
    }
    emit_op(compiler->bytecode, HLL_BYTECODE_POP);
  } break;
  case HLL_OBJ_SYMB:
    compile_symbol(compiler, ast);
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

struct hll_obj *hll_compile_ast(struct hll_compiler *compiler,
                                struct hll_obj *ast) {
  struct hll_obj *result = hll_new_func(compiler->vm, compiler->vm->nil,
                                        compiler->bytecode, "bytecode");
  hll_sb_push(compiler->vm->temp_roots, result);
  compile_progn(compiler, ast);
  emit_op(compiler->bytecode, HLL_BYTECODE_END);
  (void)hll_sb_pop(compiler->vm->temp_roots);
  return result;
}
