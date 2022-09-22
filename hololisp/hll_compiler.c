#include "hll_compiler.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_debug.h"
#include "hll_mem.h"
#include "hll_util.h"
#include "hll_value.h"
#include "hll_vm.h"

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
} hll_lexer_equivalence_class;

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
} hll_lexer_state;

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
} hll_form_kind;

typedef enum {
  HLL_LOC_NONE,
  HLL_LOC_FORM_SYMB,
  HLL_LOC_FORM_NTH,
  HLL_LOC_FORM_NTHCDR,
#define HLL_CAR_CDR(_, _letters) HLL_LOC_FORM_C##_letters##R,
  HLL_ENUMERATE_CAR_CDR
#undef HLL_CAR_CDR
} hll_location_form;

static size_t *get_location_entry_idx(hll_location_table *table,
                                      uint64_t hash) {
  size_t entry_count = sizeof(table->hash_table) / sizeof(table->hash_table[0]);
  size_t hash_mask = entry_count - 1;
  assert(!(hash_mask & entry_count));
  size_t hash_slot = hash & hash_mask;
  size_t *entry = table->hash_table + hash_slot;
  while (*entry) {
    uint64_t entry_hash = (table->entries + *entry)->hash;
    if (entry_hash == hash) {
      break;
    }

    entry = &(table->entries + *entry)->next;
  }

  return entry;
}

static void set_location_entry(hll_location_table *table, uint64_t hash,
                               uint32_t offset, uint32_t length) {
  size_t *entry_idx = get_location_entry_idx(table, hash);

  assert(!*entry_idx);
  hll_location_entry new_entry = {
      .hash = hash, .offset = offset, .length = length};
  hll_sb_push(table->entries, new_entry);
  hll_location_entry *e = &hll_sb_last(table->entries);
  e->next = *entry_idx;
  *entry_idx = e - table->entries;
}

static hll_location_entry *get_location_entry(hll_location_table *table,
                                              uint64_t hash) {
  size_t *entry_idx = get_location_entry_idx(table, hash);
  if (entry_idx == NULL) {
    return NULL;
  }
  return table->entries + *entry_idx;
}

bool hll_compile(struct hll_vm *vm, const char *source, hll_value *compiled) {
  bool result = true;

  hll_compilation_unit cu = {0};

  hll_lexer lexer;
  hll_lexer_init(&lexer, source, vm);

  hll_reader reader;
  hll_reader_init(&reader, &lexer, vm, &cu);

  ++vm->forbid_gc;
  hll_value ast = hll_read_ast(&reader);

  hll_compiler compiler;
  hll_compiler_init(&compiler, vm, vm->env, &cu);
  *compiled = hll_compile_ast(&compiler, ast);
  --vm->forbid_gc;

  if (compiler.error_count != 0) {
    result = false;
  }

  hll_sb_free(cu.locs.entries);
  hll_sb_free(compiler.loc_stack);

  return result;
}

static inline hll_lexer_equivalence_class get_equivalence_class(char symb) {
  hll_lexer_equivalence_class eqc = HLL_LEX_EQC_OTHER;
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

static inline hll_lexer_state get_next_state(hll_lexer_state state,
                                             hll_lexer_equivalence_class eqc) {
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

__attribute__((format(printf, 2, 3))) static void
lexer_error(hll_lexer *lexer, const char *fmt, ...) {
  ++lexer->error_count;

  va_list args;
  va_start(args, fmt);
  hll_report_error(lexer->vm, lexer->next.offset, lexer->next.length, fmt,
                   args);
  va_end(args);
}

void hll_lexer_init(hll_lexer *lexer, const char *input, struct hll_vm *vm) {
  memset(lexer, 0, sizeof(hll_lexer));
  lexer->vm = vm;
  lexer->cursor = lexer->input = input;
}

const hll_token *hll_lexer_next(hll_lexer *lexer) {
  // Pull it from structure so there is better chance in ends up in
  // register.
  const char *cursor = lexer->cursor;
  const char *token_start = cursor;
  hll_lexer_state state = HLL_LEX_START;
  do {
    // Basic state machine-based lexing.
    // It has been decided to use character equivalence classes in hope that
    // lexical grammar stays simple. In cases like C grammar, there are a
    // lot of special cases that are better be handled by per-codepoint
    // lookup.
    hll_lexer_equivalence_class eqc = get_equivalence_class(*cursor++);
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
    HLL_UNREACHABLE;
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
      lexer_error(lexer, "Number literal is out of range");
      value = 0;
    }

    lexer->next.kind = HLL_TOK_NUM;
    lexer->next.value = value;
  } break;
  default:
    HLL_UNREACHABLE;
  }

  return &lexer->next;
}

void hll_reader_init(hll_reader *reader, hll_lexer *lexer, struct hll_vm *vm,
                     hll_compilation_unit *cu) {
  memset(reader, 0, sizeof(hll_reader));
  reader->lexer = lexer;
  reader->vm = vm;
  reader->cu = cu;
}

__attribute__((format(printf, 2, 3))) static void
reader_error(hll_reader *reader, const char *fmt, ...) {
  ++reader->error_count;

  va_list args;
  va_start(args, fmt);
  hll_report_errorv(reader->vm, reader->token->offset, reader->token->length,
                    fmt, args);
  va_end(args);
}

static void peek_token(hll_reader *reader) {
  if (reader->should_return_old_token) {
    return;
  }

  bool is_done = false;
  while (!is_done) {
    const hll_token *token = hll_lexer_next(reader->lexer);
    reader->token = token;

    if (token->kind == HLL_TOK_COMMENT) {
    } else if (token->kind == HLL_TOK_UNEXPECTED) {
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

static hll_value read_expr(hll_reader *reader);

static void add_reader_meta_info(hll_reader *reader, hll_value value) {
  if (reader->cu == NULL) {
    return;
  }

  assert(hll_is_obj(value));
  struct hll_obj *obj = hll_unwrap_obj(value);
  uint64_t hash = (uint64_t)(uintptr_t)obj;

  uint32_t offset = reader->token->offset;
  uint32_t length = reader->token->length;
  set_location_entry(&reader->cu->locs, hash, offset, length);
}

static hll_value read_list(hll_reader *reader) {
  peek_token(reader);
  // This should be guaranteed by caller.
  assert(reader->token->kind == HLL_TOK_LPAREN);
  eat_token(reader);
  // Now we expect at least one list element followed by others ending either
  // with right paren or dot, element and right paren Now check if we don't
  // have first element
  peek_token(reader);

  // Handle nil
  if (reader->token->kind == HLL_TOK_RPAREN) {
    eat_token(reader);
    return hll_nil();
  }

  hll_value list_head;
  hll_value list_tail;
  list_head = list_tail =
      hll_new_cons(reader->vm, read_expr(reader), hll_nil());
  add_reader_meta_info(reader, list_head);

  // Now enter the loop of parsing other list elements.
  for (;;) {
    peek_token(reader);
    if (reader->token->kind == HLL_TOK_EOF) {
      reader_error(reader,
                   "Missing closing paren when reading list (eof encountered)");
      return list_head;
    } else if (reader->token->kind == HLL_TOK_RPAREN) {
      eat_token(reader);
      return list_head;
    } else if (reader->token->kind == HLL_TOK_DOT) {
      eat_token(reader);
      hll_unwrap_cons(list_tail)->cdr = read_expr(reader);

      peek_token(reader);
      if (reader->token->kind != HLL_TOK_RPAREN) {
        reader_error(reader,
                     "Missing closing paren after dot when reading list");
        return list_head;
      }
      eat_token(reader);
      return list_head;
    }

    hll_value ast = read_expr(reader);
    hll_unwrap_cons(list_tail)->cdr = hll_new_cons(reader->vm, ast, hll_nil());
    list_tail = hll_unwrap_cdr(list_tail);
    add_reader_meta_info(reader, list_tail);
  }

  HLL_UNREACHABLE;
}

static hll_value read_expr(hll_reader *reader) {
  hll_value ast = hll_nil();
  peek_token(reader);
  switch (reader->token->kind) {
  case HLL_TOK_EOF:
    break;
  case HLL_TOK_NUM:
    eat_token(reader);
    ast = hll_num(reader->token->value);
    break;
  case HLL_TOK_SYMB:
    eat_token(reader);
    if (reader->token->length == 1 &&
        reader->lexer->input[reader->token->offset] == 't') {
      ast = hll_true();
      break;
    }

    ast =
        hll_new_symbol(reader->vm, reader->lexer->input + reader->token->offset,
                       reader->token->length);
    break;
  case HLL_TOK_LPAREN:
    ast = read_list(reader);
    break;
  case HLL_TOK_QUOTE: {
    eat_token(reader);
    ast = hll_new_cons(reader->vm, reader->vm->quote_symb,
                       hll_new_cons(reader->vm, read_expr(reader), hll_nil()));
    add_reader_meta_info(reader, ast);
  } break;
  case HLL_TOK_COMMENT:
  case HLL_TOK_UNEXPECTED:
    // These code paths should be covered in next_token()
    HLL_UNREACHABLE;
    break;
  default:
    eat_token(reader);
    reader_error(reader, "Unexpected token when parsing s-expression");
    break;
  }
  return ast;
}

hll_value hll_read_ast(hll_reader *reader) {
  hll_value list_head = hll_nil();
  hll_value list_tail = hll_nil();

  for (;;) {
    peek_token(reader);
    if (reader->token->kind == HLL_TOK_EOF) {
      break;
    }

    hll_value ast = read_expr(reader);
    hll_value cons = hll_new_cons(reader->vm, ast, hll_nil());

    if (hll_is_nil(list_head)) {
      list_head = list_tail = cons;
    } else {
      hll_unwrap_cons(list_tail)->cdr = cons;
      list_tail = cons;
    }
  }

  return list_head;
}

void hll_compiler_init(hll_compiler *compiler, struct hll_vm *vm, hll_value env,
                       hll_compilation_unit *cu) {
  memset(compiler, 0, sizeof(hll_compiler));
  compiler->vm = vm;
  compiler->env = env;
  compiler->bytecode = hll_new_bytecode();
  compiler->cu = cu;
}

__attribute__((format(printf, 3, 4))) static void
compiler_error(hll_compiler *compiler, hll_value ast, const char *fmt, ...) {
  ++compiler->error_count;

  va_list args;
  va_start(args, fmt);
  hll_report_error_valuev(compiler->vm, ast, fmt, args);
  va_end(args);
}

static void compiler_push_location(hll_compiler *compiler, hll_value value) {
  if (compiler->cu == NULL) {
    return;
  }
  assert(hll_is_obj(value));
  struct hll_obj *obj = hll_unwrap_obj(value);
  uint64_t hash = (uint64_t)(uintptr_t)obj;

  hll_location_entry *loc = get_location_entry(&compiler->cu->locs, hash);
  assert(loc);
  hll_compiler_loc_stack_entry e = {.cu = compiler->cu->compilatuon_unit,
                                    .offset = loc->offset,
                                    .length = loc->length};
  hll_sb_push(compiler->loc_stack, e);
}

static void compiler_pop_location(hll_compiler *compiler) {
  if (compiler->cu == NULL) {
    return;
  }

  assert(hll_sb_len(compiler->loc_stack));
  hll_sb_pop(compiler->loc_stack);
}

static hll_form_kind get_form_kind(const char *symb) {
  hll_form_kind kind = HLL_FORM_REGULAR;
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

static void write_u16_be(uint8_t *data, uint16_t value) {
  *data++ = (value >> 8) & 0xFF;
  *data = value & 0xFF;
}

static uint16_t add_num_const(hll_compiler *compiler, double value) {
  for (size_t i = 0; i < hll_sb_len(compiler->bytecode->constant_pool); ++i) {
    hll_value test = compiler->bytecode->constant_pool[i];
    if (hll_is_num(test) && hll_unwrap_num(test) == value) {
      uint16_t narrowed = i;
      assert(i == narrowed);
      return narrowed;
    }
  }

  hll_sb_push(compiler->bytecode->constant_pool, hll_num(value));
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  return narrowed;
}

static uint16_t add_symb_const(hll_compiler *compiler, const char *symb_,
                               size_t length) {
  for (size_t i = 0; i < hll_sb_len(compiler->bytecode->constant_pool); ++i) {
    hll_value test = compiler->bytecode->constant_pool[i];
    // TODO: Hashes
    if (hll_is_symb(test) && strcmp(hll_unwrap_zsymb(test), symb_) == 0) {
      uint16_t narrowed = i;
      assert(i == narrowed);
      return narrowed;
    }
  }

  hll_value symb = hll_new_symbol(compiler->vm, symb_, length);
  hll_sb_push(compiler->bytecode->constant_pool, symb);
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  return narrowed;
}

static void compile_symbol(hll_compiler *compiler, hll_value ast) {
  assert(hll_get_value_kind(ast) == HLL_OBJ_SYMB);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
  hll_bytecode_emit_u16(compiler->bytecode,
                        add_symb_const(compiler, hll_unwrap_zsymb(ast),
                                       hll_unwrap_symb(ast)->length));
}

static void compile_expression(hll_compiler *compiler, hll_value ast);
static void compile_eval_expression(hll_compiler *compiler, hll_value ast);

static void compile_function_call_internal(hll_compiler *compiler,
                                           hll_value list) {
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  for (hll_value arg = list; !hll_is_nil(arg); arg = hll_unwrap_cdr(arg)) {
    assert(hll_is_cons(arg));
    hll_value obj = hll_unwrap_car(arg);
    compile_eval_expression(compiler, obj);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  }
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_POP);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CALL);
}

static bool expand_macro(hll_compiler *compiler, hll_value macro,
                         hll_value args, hll_value *expanded) {
  if (hll_get_value_kind(macro) != HLL_OBJ_SYMB) {
    return false;
  }

  hll_value macro_body;
  if (!hll_find_var(compiler->vm, compiler->env, macro, &macro_body) ||
      hll_get_value_kind(hll_unwrap_cdr(macro_body)) != HLL_OBJ_MACRO) {
    return false;
  } else {
    macro_body = hll_unwrap_cdr(macro_body);
  }

  *expanded = hll_expand_macro(compiler->vm, macro_body, args);
  return true;
}

static void compile_function_call(hll_compiler *compiler, hll_value list) {
  hll_value fn = hll_unwrap_car(list);
  hll_value args = hll_unwrap_cdr(list);
  hll_value expanded;
  if (expand_macro(compiler, fn, args, &expanded)) {
    compile_eval_expression(compiler, expanded);
    return;
  }
  compile_eval_expression(compiler, fn);
  compile_function_call_internal(compiler, args);
}

static void compile_quote(hll_compiler *compiler, hll_value args) {
  if (!hll_is_cons(args)) {
    compiler_error(compiler, args, "'quote' form must have an argument");
    return;
  }
  if (hll_get_value_kind(hll_unwrap_cdr(args)) != HLL_OBJ_NIL) {
    compiler_error(compiler, args,
                   "'quote' form must have exactly one argument");
  }
  compile_expression(compiler, hll_unwrap_car(args));
}

static void compile_progn(hll_compiler *compiler, hll_value prog) {
  if (hll_is_nil(prog)) {
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    return;
  }

  for (; hll_is_cons(prog); prog = hll_unwrap_cdr(prog)) {
    compile_eval_expression(compiler, hll_unwrap_car(prog));
    if (hll_get_value_kind(hll_unwrap_cdr(prog)) != HLL_OBJ_NIL) {
      hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_POP);
    }
  }
}

static void compile_if(hll_compiler *compiler, hll_value args) {
  size_t length = hll_list_length(args);
  if (length < 2) {
    compiler_error(compiler, args, "'if' form expects at least 2 arguments");
    return;
  }

  hll_value cond = hll_unwrap_car(args);
  compile_eval_expression(compiler, cond);
  hll_value pos_arm = hll_unwrap_cdr(args);
  assert(hll_is_cons(pos_arm));
  hll_value neg_arm = hll_unwrap_cdr(pos_arm);
  pos_arm = hll_unwrap_car(pos_arm);

  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_JN);
  size_t jump_false = hll_bytecode_emit_u16(compiler->bytecode, 0);
  compile_eval_expression(compiler, pos_arm);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_JN);
  size_t jump_out = hll_bytecode_emit_u16(compiler->bytecode, 0);
  write_u16_be(compiler->bytecode->ops + jump_false,
               hll_bytecode_op_idx(compiler->bytecode) - jump_false - 2);
  compile_progn(compiler, neg_arm);
  write_u16_be(compiler->bytecode->ops + jump_out,
               hll_bytecode_op_idx(compiler->bytecode) - jump_out - 2);
}

static void compile_let(hll_compiler *compiler, hll_value args) {
  size_t length = hll_list_length(args);
  if (length < 1) {
    compiler_error(compiler, args,
                   "'let' special form requires variable declarations");
    return;
  }

  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_PUSHENV);
  for (hll_value let = hll_unwrap_car(args); !hll_is_nil(let);
       let = hll_unwrap_cdr(let)) {
    hll_value pair = hll_unwrap_car(let);
    assert(hll_is_cons(pair));
    hll_value name = hll_unwrap_car(pair);
    hll_value value = hll_unwrap_cdr(pair);
    if (hll_is_cons(value)) {
      assert(hll_get_value_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
      value = hll_unwrap_car(value);
    }

    assert(hll_get_value_kind(name) == HLL_OBJ_SYMB);
    compile_expression(compiler, name);
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_LET);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_POP);
  }

  compile_progn(compiler, hll_unwrap_cdr(args));
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_POPENV);
}

#define HLL_CAR_CDR(_lower, _)                                                 \
  static void compile_c##_lower##r(hll_compiler *compiler, hll_value args) {   \
    if (hll_list_length(args) != 1) {                                          \
      compiler_error(compiler, args,                                           \
                     "'c" #_lower "r' expects exactly 1 argument");            \
    } else {                                                                   \
      compile_eval_expression(compiler, hll_unwrap_car(args));                 \
      const char *ops = #_lower;                                               \
      const char *op = ops + sizeof(#_lower) - 2;                              \
      for (;;) {                                                               \
        if (*op == 'a') {                                                      \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CAR);          \
        } else if (*op == 'd') {                                               \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CDR);          \
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

static hll_location_form get_location_form(hll_value location) {
  hll_location_form kind = HLL_LOC_NONE;
  if (hll_get_value_kind(location) == HLL_OBJ_SYMB) {
    kind = HLL_LOC_FORM_SYMB;
  } else if (hll_is_cons(location)) {
    hll_value first = hll_unwrap_car(location);
    if (hll_get_value_kind(first) == HLL_OBJ_SYMB) {
      const char *symb = hll_unwrap_zsymb(first);
      if (strcmp(symb, "nth") == 0) {
        kind = HLL_LOC_FORM_NTH;
      } else if (strcmp(symb, "nthcdr") == 0) {
        kind = HLL_LOC_FORM_NTHCDR;
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

static void compile_set_location(hll_compiler *compiler, hll_value location,
                                 hll_value value) {
  hll_location_form kind = get_location_form(location);
  switch (kind) {
  case HLL_LOC_NONE:
    compiler_error(compiler, location, "location is not valid");
    break;
  case HLL_LOC_FORM_SYMB:
    compile_symbol(compiler, location);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_FIND);
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
    break;
  case HLL_LOC_FORM_NTHCDR:
    if (hll_list_length(location) != 3) {
      compiler_error(compiler, location,
                     "'nthcdr' expects exactly 2 arguments");
      break;
    }
    // get the nth function
    compile_symbol(compiler, compiler->vm->nthcdr_symb);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_FIND);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CDR);
    // call nth
    compile_function_call_internal(compiler, hll_unwrap_cdr(location));
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
    break;
  case HLL_LOC_FORM_NTH: {
    if (hll_list_length(location) != 3) {
      compiler_error(compiler, location, "'nth' expects exactly 2 arguments");
      break;
    }
    // get the nth function
    compile_symbol(compiler, compiler->vm->nthcdr_symb);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_FIND);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CDR);
    // call nth
    compile_function_call_internal(compiler, hll_unwrap_cdr(location));
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCAR);
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
          hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCAR);       \
        } else if (*op == 'd') {                                               \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);       \
        } else {                                                               \
          HLL_UNREACHABLE;                                                     \
        }                                                                      \
        break;                                                                 \
      } else {                                                                 \
        if (*op == 'a') {                                                      \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CAR);          \
        } else if (*op == 'd') {                                               \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CDR);          \
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

static void compile_setf(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) < 1) {
    compiler_error(compiler, args, "'set!' expects at least 1 argument");
    return;
  }

  hll_value location = hll_unwrap_car(args);
  hll_value value = hll_unwrap_cdr(args);
  if (hll_is_cons(value)) {
    assert(hll_get_value_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
    value = hll_unwrap_car(value);
  }

  compile_set_location(compiler, location, value);
}

static void compile_setcar(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) != 2) {
    compiler_error(compiler, args, "'setcar!' expects exactly 2 arguments");
    return;
  }

  hll_value location = hll_unwrap_car(args);
  hll_value value = hll_unwrap_cdr(args);
  if (hll_is_cons(value)) {
    assert(hll_get_value_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
    value = hll_unwrap_car(value);
  }

  compile_eval_expression(compiler, location);
  compile_eval_expression(compiler, value);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCAR);
}

static void compile_setcdr(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) != 2) {
    compiler_error(compiler, args, "'setcar!' expects exactly 2 arguments");
    return;
  }

  hll_value location = hll_unwrap_car(args);
  hll_value value = hll_unwrap_cdr(args);
  if (hll_is_cons(value)) {
    assert(hll_get_value_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
    value = hll_unwrap_car(value);
  }

  compile_eval_expression(compiler, location);
  compile_eval_expression(compiler, value);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
}

static void compile_list(hll_compiler *compiler, hll_value args) {
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  for (hll_value arg = args; !hll_is_nil(arg); arg = hll_unwrap_cdr(arg)) {
    assert(hll_is_cons(arg));
    hll_value obj = hll_unwrap_car(arg);
    compile_eval_expression(compiler, obj);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  }
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_POP);
}

static void compile_cons(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) != 2) {
    compiler_error(compiler, args, "'cons' expects exactly 2 arguments");
    return;
  }

  hll_value car = hll_unwrap_car(args);
  hll_value cdr = hll_unwrap_car(hll_unwrap_cdr(args));
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  compile_eval_expression(compiler, car);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);
  compile_eval_expression(compiler, cdr);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_POP);
}

static void add_symbol_to_function_param_list(hll_compiler *compiler,
                                              hll_value car,
                                              hll_value *param_list,
                                              hll_value *param_list_tail) {
  hll_value symb = car;
  if (!hll_is_nil(car)) {
    uint16_t symb_idx = add_symb_const(compiler, hll_unwrap_zsymb(car),
                                       hll_unwrap_symb(car)->length);
    symb = compiler->bytecode->constant_pool[symb_idx];
  }

  hll_value cons = hll_new_cons(compiler->vm, (hll_value)symb, hll_nil());
  if (hll_is_nil(*param_list)) {
    *param_list = *param_list_tail = cons;
  } else {
    hll_unwrap_cons(*param_list_tail)->cdr = cons;
    *param_list_tail = cons;
  }
}

static bool compile_function_internal(hll_compiler *compiler, hll_value params,
                                      hll_value body, bool is_macro,
                                      hll_value *compiled_) {
  hll_compiler new_compiler = {0};
  // TODO: Compilation unit
  hll_compiler_init(&new_compiler, compiler->vm, compiler->vm->env, NULL);
  hll_value compiled = hll_compile_ast(&new_compiler, body);
  if (new_compiler.error_count != 0) {
    compiler->error_count += new_compiler.error_count;
    return false;
  }

  hll_sb_push(compiler->vm->temp_roots, compiled);

  hll_value param_list = hll_nil();
  hll_value param_list_tail = hll_nil();
  if (hll_is_symb(params)) {
    add_symbol_to_function_param_list(&new_compiler, hll_nil(), &param_list,
                                      &param_list_tail);
    add_symbol_to_function_param_list(&new_compiler, params, &param_list,
                                      &param_list_tail);
  } else {
    if (!hll_is_list(params)) {
      compiler_error(compiler, params, "param list must be a list");
      return false;
    }

    hll_value obj = params;
    for (; hll_is_cons(obj); obj = hll_unwrap_cdr(obj)) {
      hll_value car = hll_unwrap_car(obj);
      if (!hll_is_symb(car)) {
        compiler_error(compiler, car, "function param name is not a symbol");
        return false;
      }

      add_symbol_to_function_param_list(&new_compiler, car, &param_list,
                                        &param_list_tail);
    }

    if (!hll_is_nil(obj)) {
      if (!hll_is_symb(obj)) {
        compiler_error(compiler, obj, "function param name is not a symbol");
        return false;
      }
      assert(hll_is_cons(param_list_tail));
      hll_unwrap_cons(param_list_tail)->cdr = (hll_value)obj;
    }
  }

  hll_value func;
  if (!is_macro) {
    func = compiled;
    hll_unwrap_func(func)->param_names = param_list;
  } else {
    func = compiled;
    // TODO: Remove ub
    ((struct hll_obj *)((char *)hll_unwrap_func(func) - sizeof(struct hll_obj)))
        ->kind = HLL_OBJ_MACRO;
    hll_unwrap_macro(func)->param_names = param_list;
  }

  (void)hll_sb_pop(compiler->vm->temp_roots); // compiled
  *compiled_ = func;
  return true;
}

static bool compile_function(hll_compiler *compiler, hll_value params,
                             hll_value body, uint16_t *idx) {
  hll_value func;
  if (!compile_function_internal(compiler, params, body, false, &func)) {
    return true;
  }

  hll_sb_push(compiler->bytecode->constant_pool, func);
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  *idx = narrowed;

  return false;
}

static void compile_lambda(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) < 2) {
    compiler_error(compiler, args, "'lambda' expects at least 2 arguments");
    return;
  }

  hll_value params = hll_unwrap_car(args);
  args = hll_unwrap_cdr(args);
  hll_value body = args;

  uint16_t function_idx;
  if (compile_function(compiler, params, body, &function_idx)) {
    return;
  }

  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_MAKEFUN);
  hll_bytecode_emit_u16(compiler->bytecode, function_idx);
}

static void compile_and(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) == 0) {
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_TRUE);
    return;
  }

  size_t last_jump = 0;
  size_t original_idx = hll_bytecode_op_idx(compiler->bytecode);
  for (hll_value arg_slot = args; hll_is_cons(arg_slot);
       arg_slot = hll_unwrap_cdr(arg_slot)) {
    hll_value item = hll_unwrap_car(arg_slot);
    compile_eval_expression(compiler, item);
    if (hll_get_value_kind(hll_unwrap_cdr(arg_slot)) != HLL_OBJ_CONS) {
      hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    }
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_JN);
    last_jump = hll_bytecode_emit_u16(compiler->bytecode, 0);
  }
  size_t short_circuit = hll_bytecode_op_idx(compiler->bytecode);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  size_t total_out = hll_bytecode_op_idx(compiler->bytecode);

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

static void compile_or(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) == 0) {
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    return;
  }

  size_t previous_jump = 0;
  size_t original_idx = hll_bytecode_op_idx(compiler->bytecode);
  for (hll_value arg_slot = args; hll_is_cons(arg_slot);
       arg_slot = hll_unwrap_cdr(arg_slot)) {
    hll_value item = hll_unwrap_car(arg_slot);

    if (arg_slot != args) {
      assert(previous_jump != 0);
      write_u16_be(compiler->bytecode->ops + previous_jump,
                   hll_bytecode_op_idx(compiler->bytecode) - previous_jump - 2);
      hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_POP);
    }

    compile_eval_expression(compiler, item);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_DUP);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_JN); // jump to next
    previous_jump = hll_bytecode_emit_u16(compiler->bytecode, 0);
    if (hll_get_value_kind(hll_unwrap_cdr(arg_slot)) != HLL_OBJ_NIL) {
      hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
      hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_JN);
      hll_bytecode_emit_u16(compiler->bytecode, 0);
    }
  }

  size_t total_out = hll_bytecode_op_idx(compiler->bytecode);

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

static void process_defmacro(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) < 2) {
    compiler_error(compiler, args, "'defmacro' expects at least 2 arguments");
    return;
  }

  hll_value control = hll_unwrap_car(args);
  if (!hll_is_cons(control)) {
    compiler_error(compiler, control,
                   "'defmacro' first argument must be list of macro name and "
                   "its parameters");
    return;
  }

  hll_value name = hll_unwrap_car(control);
  if (hll_get_value_kind(name) != HLL_OBJ_SYMB) {
    compiler_error(compiler, name, "'defmacro' name should be a symbol");
    return;
  }

  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
  hll_value params = hll_unwrap_cdr(control);
  hll_value body = hll_unwrap_cdr(args);

  hll_value macro_expansion;
  ;
  if (compile_function_internal(compiler, params, body, true,
                                &macro_expansion)) {
    // TODO: Test if macro with same name exists
    hll_sb_push(compiler->vm->temp_roots, macro_expansion);
    hll_add_variable(compiler->vm, compiler->env, name, macro_expansion);
    (void)hll_sb_pop(compiler->vm->temp_roots);
  }
}

static void compile_macroexpand(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) != 1) {
    compiler_error(compiler, args,
                   "'macroexpand' expects exactly one argument");
    return;
  }

  hll_value macro_list = hll_unwrap_car(args);
  if (hll_list_length(macro_list) < 1) {
    compiler_error(compiler, macro_list,
                   "'macroexpand' argument is not a list");
    return;
  }

  hll_value expanded;
  bool ok = expand_macro(compiler, hll_unwrap_car(macro_list),
                         hll_unwrap_cdr(macro_list), &expanded);
  (void)ok;
  assert(ok);
  compile_expression(compiler, expanded);
}

static void compile_define(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) == 0) {
    compiler_error(compiler, args,
                   "'define' form expects at least one argument");
    return;
  }

  hll_value decide = hll_unwrap_car(args);
  hll_value rest = hll_unwrap_cdr(args);

  if (hll_is_cons(decide)) {
    hll_value name = hll_unwrap_car(decide);
    if (hll_get_value_kind(name) != HLL_OBJ_SYMB) {
      compiler_error(compiler, name, "'define' name should be a symbol");
      return;
    }

    hll_value params = hll_unwrap_cdr(decide);
    hll_value body = rest;

    compile_expression(compiler, name);

    uint16_t function_idx;
    if (compile_function(compiler, params, body, &function_idx)) {
      return;
    }

    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_MAKEFUN);
    hll_bytecode_emit_u16(compiler->bytecode, function_idx);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_LET);
  } else if (hll_get_value_kind(decide) == HLL_OBJ_SYMB) {
    hll_value value = rest;
    if (hll_is_cons(value)) {
      assert(hll_get_value_kind(hll_unwrap_cdr(value)) == HLL_OBJ_NIL);
      value = hll_unwrap_car(value);
    }

    compile_expression(compiler, decide);
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_LET);
  } else {
    compiler_error(compiler, args,
                   "'define' first argument must either be a function name and "
                   "arguments or a variable name");
    return;
  }
}

static void compile_form(hll_compiler *compiler, hll_value args,
                         hll_form_kind kind) {
  if (kind != HLL_FORM_REGULAR) {
    assert(hll_is_cons(args));
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

static void compile_eval_expression(hll_compiler *compiler, hll_value ast) {
  switch (hll_get_value_kind(ast)) {
  case HLL_OBJ_NIL:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    break;
  case HLL_OBJ_TRUE:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_TRUE);
    break;
  case HLL_OBJ_NUM:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
    hll_bytecode_emit_u16(compiler->bytecode,
                          add_num_const(compiler, hll_unwrap_num(ast)));
    break;
  case HLL_OBJ_CONS: {
    compiler_push_location(compiler, ast);
    hll_value fn = hll_unwrap_car(ast);
    hll_form_kind kind = HLL_FORM_REGULAR;
    if (hll_is_symb(fn)) {
      kind = get_form_kind(hll_unwrap_zsymb(fn));
    }

    compile_form(compiler, ast, kind);
    compiler_pop_location(compiler);
  } break;
  case HLL_OBJ_SYMB:
    compile_symbol(compiler, ast);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_FIND);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CDR);
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

// Compiles expression as getting its value.
// Does not evaluate it.
// After it one value is located on top of the stack.
static void compile_expression(hll_compiler *compiler, hll_value ast) {
  switch (hll_get_value_kind(ast)) {
  case HLL_OBJ_NIL:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    break;
  case HLL_OBJ_TRUE:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_TRUE);
    break;
  case HLL_OBJ_NUM:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_CONST);
    hll_bytecode_emit_u16(compiler->bytecode,
                          add_num_const(compiler, hll_unwrap_num(ast)));
    break;
  case HLL_OBJ_CONS: {
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_NIL);

    hll_value obj = ast;
    compiler_push_location(compiler, obj);
    while (!hll_is_nil(obj)) {
      assert(hll_is_cons(obj));
      compile_expression(compiler, hll_unwrap_car(obj));
      hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_APPEND);

      hll_value cdr = hll_unwrap_cdr(obj);
      if (!hll_is_nil(cdr) && !hll_is_cons(cdr)) {
        compile_expression(compiler, cdr);
        hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_SETCDR);
        break;
      }

      obj = cdr;
    }
    compiler_pop_location(compiler);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_POP);
  } break;
  case HLL_OBJ_SYMB:
    compile_symbol(compiler, ast);
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

hll_value hll_compile_ast(hll_compiler *compiler, hll_value ast) {
  hll_value result = hll_new_func(compiler->vm, hll_nil(), compiler->bytecode);
  hll_sb_push(compiler->vm->temp_roots, result);
  compile_progn(compiler, ast);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BYTECODE_END);
  (void)hll_sb_pop(compiler->vm->temp_roots);
  return result;
}

#if 0

enum hll_tail_recursive_form_kind {
  HLL_TAIL_RECURIVE_FORM_OTHER,
  HLL_TAIL_RECURSIVE_FORM_IF,
  HLL_TAIL_RECURSIVE_FORM_PROGN,
  HLL_TAIL_RECURSIVE_FORM_AND,
  HLL_TAIL_RECURSIVE_FORM_OR,
  HLL_TAIL_RECURSIVE_FORM_LET,
};

static enum hll_tail_recursive_form_kind
get_tail_recursive_form_kind(hll_value form) {
  assert(hll_is_symb(form));
  const char *symb = hll_unwrap_zsymb(form);

  enum hll_tail_recursive_form_kind kind = HLL_TAIL_RECURIVE_FORM_OTHER;
  if (strcmp(symb, "if") == 0) {
    kind = HLL_TAIL_RECURSIVE_FORM_IF;
  } else if (strcmp(symb, "progn") == 0) {
    kind = HLL_TAIL_RECURSIVE_FORM_PROGN;
  } else if (strcmp(symb, "and") == 0) {
    kind = HLL_TAIL_RECURSIVE_FORM_AND;
  } else if (strcmp(symb, "or") == 0) {
    kind = HLL_TAIL_RECURSIVE_FORM_OR;
  } else if (strcmp(symb, "let") == 0) {
    kind = HLL_TAIL_RECURSIVE_FORM_LET;
  }

  return kind;
}

static bool is_if_tail_recursive(hll_value form, hll_value name_symb) {}

static bool is_and_tail_recursive(hll_value form, hll_value name_symb) {}

static bool is_or_tail_recursive(hll_value form, hll_value name_symb) {}

static bool is_let_tail_recurive(hll_value form, hll_value name_symb) {}

bool is_progn_tail_recursive(hll_value progn, hll_value name_symb) {
  assert(hll_is_symb(name_symb));
  // skip to the last statement
  for (; hll_is_cons(progn) && !hll_is_nil(hll_unwrap_cdr(progn));
       progn = hll_unwrap_cdr(progn))
    ;

  // Now progn has either last element or some bogus value.
  if (!hll_is_cons(progn) || !hll_is_nil(hll_unwrap_cdr(progn))) {
    return false;
  }

  hll_value last = hll_unwrap_car(progn);
  if (!hll_is_cons(last)) {
    return false;
  }

  hll_value form = hll_unwrap_car(last);
  if (!hll_is_symb(form)) {
    return false;
  }

  enum hll_tail_recursive_form_kind kind = get_tail_recursive_form_kind(form);
  switch (kind) {
  case HLL_TAIL_RECURIVE_FORM_OTHER: {
    const char *name = hll_unwrap_zsymb(name_symb);
    if (strcmp(name, hll_unwrap_zsymb(form)) == 0) {
      return true;
    }
  } break;
  case HLL_TAIL_RECURSIVE_FORM_IF:
    return is_if_tail_recursive(form, name_symb);
  case HLL_TAIL_RECURSIVE_FORM_PROGN:
    return is_progn_tail_recursive(form, name_symb);
  case HLL_TAIL_RECURSIVE_FORM_AND:
    return is_and_tail_recursive(form, name_symb);
  case HLL_TAIL_RECURSIVE_FORM_OR:
    return is_or_tail_recursive(form, name_symb);
  case HLL_TAIL_RECURSIVE_FORM_LET:
    return is_let_tail_recurive(form, name_symb);
  }

  return false;
}
#endif
