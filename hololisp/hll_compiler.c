#include "hll_compiler.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hll_bytecode.h"
#include "hll_debug.h"
#include "hll_gc.h"
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
  HLL_CAR_CDR(dd, DD)

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
  // language-defined function
  HLL_FORM_REGULAR,
  HLL_FORM_QUOTE,
  HLL_FORM_IF,
  HLL_FORM_SET,
  HLL_FORM_LET,
  HLL_FORM_LIST,
  HLL_FORM_CONS,
  HLL_FORM_SETCAR,
  HLL_FORM_SETCDR,
  HLL_FORM_PROGN,
  HLL_FORM_LAMBDA,
  HLL_FORM_DEFINE,
  HLL_FORM_DEFMACRO,
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
  assert(table);
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

static void store_location(hll_location_table *table, uint64_t hash,
                           uint32_t offset, uint32_t length) {
  assert(table);
  hll_location_entry new_entry = {
      .hash = hash, .offset = offset, .length = length};
  hll_sb_push(table->entries, new_entry);
  size_t *entry_idx = get_location_entry_idx(table, hash);
  assert(!*entry_idx);
  hll_location_entry *e = &hll_sb_last(table->entries);
  e->next = *entry_idx;
  *entry_idx = e - table->entries;
}

static uint64_t hash_value(hll_value value) {
  assert(hll_is_obj(value));
  struct hll_obj *obj = hll_unwrap_obj(value);
  return (uint64_t)(uintptr_t)obj;
}

static hll_location_entry *get_location_entry(hll_location_table *table,
                                              uint64_t hash) {
  size_t *entry_idx = get_location_entry_idx(table, hash);
  if (entry_idx == NULL) {
    return NULL;
  }
  return table->entries + *entry_idx;
}

bool hll_compile(struct hll_vm *vm, const char *source, const char *name,
                 hll_value *compiled) {
  bool result = true;

  hll_translation_unit tu = hll_make_tu(vm, source, name, HLL_TU_FLAG_DEBUG);

  hll_lexer lexer;
  hll_lexer_init(&lexer, source, &tu);

  hll_reader reader;
  hll_reader_init(&reader, &lexer, &tu);

  hll_push_forbid_gc(vm->gc);
  hll_value ast = hll_read_ast(&reader);

  if (lexer.error_count == 0 && reader.error_count == 0) {
    hll_compiler compiler;
    hll_compiler_init(&compiler, &tu, hll_nil());
    *compiled = hll_compile_ast(&compiler, ast);
    hll_sb_free(compiler.loc_stack);

    if (compiler.error_count != 0) {
      result = false;
    }
  } else {
    result = false;
  }

  hll_pop_forbid_gc(vm->gc);
  hll_delete_tu(&tu);

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
    HLL_UNREACHABLE;
  }

  return state;
}

__attribute__((format(printf, 2, 3))) static void
lexer_error(hll_lexer *lexer, const char *fmt, ...) {
  ++lexer->error_count;
  if (lexer->tu == NULL || !(lexer->tu->flags & HLL_TU_FLAG_DEBUG)) {
    return;
  }

  va_list args;
  va_start(args, fmt);
  hll_report_errorv(lexer->tu->vm->debug,
                    (hll_loc){lexer->tu->translation_unit, lexer->next.offset},
                    fmt, args);
  va_end(args);
}

void hll_lexer_init(hll_lexer *lexer, const char *input,
                    hll_translation_unit *tu) {
  memset(lexer, 0, sizeof(hll_lexer));
  lexer->tu = tu;
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

hll_translation_unit hll_make_tu(struct hll_vm *vm, const char *source,
                                 const char *name, hll_tu_flags flags) {
  hll_translation_unit tu = {0};
  tu.vm = vm;
  tu.flags = flags;
  tu.name = name;
  tu.source = source;

  if (flags & HLL_TU_FLAG_DEBUG) {
    tu.locs = hll_alloc(sizeof(*tu.locs));
    tu.translation_unit = hll_ds_init_tu(vm->debug, source, name);
  }

  return tu;
}

void hll_delete_tu(hll_translation_unit *tu) {
  if (tu->locs != NULL) {
    hll_sb_free(tu->locs->entries);
    hll_free(tu->locs, sizeof(*tu->locs));
  }
}

void hll_reader_init(hll_reader *reader, hll_lexer *lexer,
                     hll_translation_unit *tu) {
  memset(reader, 0, sizeof(hll_reader));
  reader->lexer = lexer;
  reader->tu = tu;
}

__attribute__((format(printf, 3, 4))) static void
reader_error(hll_reader *reader, uint32_t offset, const char *fmt, ...) {
  if (!reader->tu) {
    return;
  }

  hll_loc loc = {.translation_unit = reader->tu->translation_unit, offset};
  va_list args;
  va_start(args, fmt);
  hll_report_errorv(reader->tu->vm->debug, loc, fmt, args);
  va_end(args);
  ++reader->error_count;
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
      reader_error(reader, token->offset, "Unexpected token");
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

static void record_location(hll_reader *reader, hll_value value) {
  if (!(reader->tu->flags & HLL_TU_FLAG_DEBUG)) {
    return;
  }

  uint64_t hash = hash_value(value);
  uint32_t offset = reader->token->offset;
  uint32_t length = reader->token->length;
  store_location(reader->tu->locs, hash, offset, length);
}

static hll_value read_list(hll_reader *reader) {
  peek_token(reader);
  // This is used to report errors because I am lazy to do it properly
  uint32_t head_offset = reader->token->offset;
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
      hll_new_cons(reader->tu->vm, read_expr(reader), hll_nil());
  record_location(reader, list_head);

  // Now enter the loop of parsing other list elements.
  for (;;) {
    peek_token(reader);
    if (reader->token->kind == HLL_TOK_EOF) {
      reader_error(reader, head_offset,
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
        reader_error(reader, head_offset,
                     "Missing closing paren after dot when reading list");
        return list_head;
      }
      eat_token(reader);
      return list_head;
    }

    hll_value ast = read_expr(reader);
    hll_value cons = hll_new_cons(reader->tu->vm, ast, hll_nil());
    hll_setcdr(list_tail, cons);
    list_tail = cons;
    record_location(reader, list_tail);
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

    ast = hll_new_symbol(reader->tu->vm,
                         reader->lexer->input + reader->token->offset,
                         reader->token->length);
    break;
  case HLL_TOK_LPAREN:
    ast = read_list(reader);
    break;
  case HLL_TOK_QUOTE: {
    eat_token(reader);
    ast = hll_new_cons(
        reader->tu->vm, hll_new_symbolz(reader->tu->vm, "quote"),
        hll_new_cons(reader->tu->vm, read_expr(reader), hll_nil()));
  } break;
  case HLL_TOK_COMMENT:
  case HLL_TOK_UNEXPECTED:
    // These code paths should be covered in next_token()
    HLL_UNREACHABLE;
    break;
  default:
    reader_error(reader, reader->token->offset,
                 "Unexpected token when parsing s-expression");
    eat_token(reader);
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
    hll_value cons = hll_new_cons(reader->tu->vm, ast, hll_nil());

    if (hll_is_nil(list_head)) {
      list_head = list_tail = cons;
      record_location(reader, list_head);
    } else {
      hll_unwrap_cons(list_tail)->cdr = cons;
      list_tail = cons;
    }
  }

  return list_head;
}

void hll_compiler_init(hll_compiler *compiler, hll_translation_unit *tu,
                       hll_value name) {
  memset(compiler, 0, sizeof(hll_compiler));
  compiler->tu = tu;
  compiler->bytecode = hll_new_bytecode(name);
  compiler->bytecode->translation_unit = tu->translation_unit;
}

__attribute__((format(printf, 3, 4))) static void
compiler_error(hll_compiler *compiler, hll_value ast, const char *fmt, ...) {
  ++compiler->error_count;
  if (!(compiler->tu->flags & HLL_TU_FLAG_DEBUG)) {
    return;
  }

  uint64_t hash = hash_value(ast);
  hll_location_entry *loc = get_location_entry(compiler->tu->locs, hash);
  assert(loc);

  va_list args;
  va_start(args, fmt);
  hll_report_errorv(compiler->tu->vm->debug,
                    (hll_loc){compiler->tu->translation_unit, loc->offset}, fmt,
                    args);
  va_end(args);
}

static bool compiler_push_location(hll_compiler *compiler, hll_value value) {
  if (compiler->tu == NULL || !(compiler->tu->flags & HLL_TU_FLAG_DEBUG)) {
    return false;
  }

  if (!hll_is_obj(value)) {
    return false;
  }

  uint64_t hash = hash_value(value);
  hll_location_entry *loc = get_location_entry(compiler->tu->locs, hash);
  if (loc == NULL) {
    return false;
  }
  hll_compiler_loc_stack_entry e = {.cu = compiler->tu->translation_unit,
                                    .offset = loc->offset,
                                    .length = loc->length};
  hll_sb_push(compiler->loc_stack, e);

  size_t current_op_idx = hll_bytecode_op_idx(compiler->bytecode);
  size_t section_size = current_op_idx - compiler->loc_op_idx;
  if (section_size) {
    hll_bytecode_add_loc(compiler->tu->vm->debug,
                         compiler->tu->translation_unit, section_size, e.cu,
                         e.offset);
    compiler->loc_op_idx = current_op_idx;
  }

  return true;
}

static void compiler_pop_location(hll_compiler *compiler, bool skip) {
  if (!skip) {
    return;
  }

  if (compiler->tu == NULL || !(compiler->tu->flags & HLL_TU_FLAG_DEBUG)) {
    return;
  }

  assert(hll_sb_len(compiler->loc_stack));
  (void)hll_sb_pop(compiler->loc_stack);
  size_t current_op_idx = hll_bytecode_op_idx(compiler->bytecode);
  if (hll_sb_len(compiler->loc_stack)) {
    hll_compiler_loc_stack_entry *e = &hll_sb_last(compiler->loc_stack);
    size_t section_size = current_op_idx - compiler->loc_op_idx;
    if (section_size) {
      hll_bytecode_add_loc(compiler->tu->vm->debug,
                           compiler->tu->translation_unit, section_size, e->cu,
                           e->offset);
    }
  }
  compiler->loc_op_idx = current_op_idx;
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
  } else if (strcmp(symb, "defmacro") == 0) {
    kind = HLL_FORM_DEFMACRO;
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
    if (hll_is_symb(test) && strcmp(hll_unwrap_zsymb(test), symb_) == 0) {
      uint16_t narrowed = i;
      assert(i == narrowed);
      return narrowed;
    }
  }

  hll_value symb = hll_new_symbol(compiler->tu->vm, symb_, length);
  hll_sb_push(compiler->bytecode->constant_pool, symb);
  size_t result = hll_sb_len(compiler->bytecode->constant_pool) - 1;
  uint16_t narrowed = result;
  assert(result == narrowed);
  return narrowed;
}

static void compile_symbol(hll_compiler *compiler, hll_value ast) {
  assert(hll_get_value_kind(ast) == HLL_VALUE_SYMB);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CONST);
  hll_bytecode_emit_u16(compiler->bytecode,
                        add_symb_const(compiler, hll_unwrap_zsymb(ast),
                                       hll_unwrap_symb(ast)->length));
}

static void compile_expression(hll_compiler *compiler, hll_value ast);
static void compile_eval_expression(hll_compiler *compiler, hll_value ast);

static void compile_function_call_internal(hll_compiler *compiler,
                                           hll_value list) {
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  for (hll_value arg = list; hll_is_cons(arg); arg = hll_unwrap_cdr(arg)) {
    hll_value obj = hll_unwrap_car(arg);
    compile_eval_expression(compiler, obj);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_APPEND);
  }
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_POP);

  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CALL);
}

static bool expand_macro(hll_compiler *compiler, hll_value list,
                         hll_value *expanded) {
  hll_value macro = hll_unwrap_car(list);
  hll_value args = hll_unwrap_cdr(list);
  if (hll_get_value_kind(macro) != HLL_VALUE_SYMB) {
    return false;
  }

  hll_value macro_body;
  if (!hll_find_var(compiler->tu->vm->macro_env, macro, &macro_body)) {
    return false;
  } else {
    macro_body = hll_unwrap_cdr(macro_body);
  }

  hll_expand_macro_result res =
      hll_expand_macro(compiler->tu->vm, macro_body, args, expanded);
  if (res == HLL_EXPAND_MACRO_ERR_ARGS) {
    compiler_error(compiler, list,
                   "macro invocation argument count does not match");
    return false;
  }
  return true;
}

static void compile_function_call(hll_compiler *compiler, hll_value list) {
  hll_value expanded;
  if (expand_macro(compiler, list, &expanded)) {
    compile_eval_expression(compiler, expanded);
    return;
  }
  hll_value fn = hll_unwrap_car(list);
  hll_value args = hll_unwrap_cdr(list);
  compile_eval_expression(compiler, fn);
  compile_function_call_internal(compiler, args);
}

static void compile_quote(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) == 1) {
    compiler_error(compiler, args, "'quote' form must have an argument");
    return;
  }
  args = hll_unwrap_cdr(args);
  if (!hll_is_nil(hll_unwrap_cdr(args))) {
    compiler_error(compiler, args,
                   "'quote' form must have exactly one argument");
  }
  compile_expression(compiler, hll_unwrap_car(args));
}

static void compile_progn_internal(hll_compiler *compiler, hll_value prog) {
  if (hll_is_nil(prog)) {
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
    return;
  }

  for (; hll_is_cons(prog); prog = hll_unwrap_cdr(prog)) {
    compile_eval_expression(compiler, hll_unwrap_car(prog));
    if (!hll_is_nil(hll_unwrap_cdr(prog))) {
      hll_bytecode_emit_op(compiler->bytecode, HLL_BC_POP);
    }
  }
}

static void compile_progn(hll_compiler *compiler, hll_value prog) {
  compile_progn_internal(compiler, hll_unwrap_cdr(prog));
}

static void compile_if(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) < 3) {
    compiler_error(compiler, args, "'if' form expects at least 2 arguments");
    return;
  }
  args = hll_unwrap_cdr(args);

  hll_value cond = hll_unwrap_car(args);
  compile_eval_expression(compiler, cond);
  hll_value pos_arm = hll_unwrap_cdr(args);
  assert(hll_is_cons(pos_arm));
  hll_value neg_arm = hll_unwrap_cdr(pos_arm);
  pos_arm = hll_unwrap_car(pos_arm);

  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_JN);
  size_t jump_false = hll_bytecode_emit_u16(compiler->bytecode, 0);
  compile_eval_expression(compiler, pos_arm);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_JN);
  size_t jump_out = hll_bytecode_emit_u16(compiler->bytecode, 0);
  write_u16_be(compiler->bytecode->ops + jump_false,
               hll_bytecode_op_idx(compiler->bytecode) - jump_false - 2);
  compile_progn_internal(compiler, neg_arm);
  write_u16_be(compiler->bytecode->ops + jump_out,
               hll_bytecode_op_idx(compiler->bytecode) - jump_out - 2);
}

static void compile_let(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) < 2) {
    compiler_error(compiler, args,
                   "'let' special form requires variable declarations");
    return;
  }
  args = hll_unwrap_cdr(args);

  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_PUSHENV);
  for (hll_value let = hll_unwrap_car(args); hll_is_cons(let);
       let = hll_unwrap_cdr(let)) {
    hll_value pair = hll_unwrap_car(let);
    if (!hll_is_cons(pair)) {
      compiler_error(compiler, args,
                     "'let' special form requires lists as variable bindings");
      return;
    }
    hll_value name = hll_unwrap_car(pair);
    hll_value value = hll_unwrap_cdr(pair);
    if (hll_is_cons(value)) {
      assert(hll_get_value_kind(hll_unwrap_cdr(value)) == HLL_VALUE_NIL);
      value = hll_unwrap_car(value);
    }

    assert(hll_get_value_kind(name) == HLL_VALUE_SYMB);
    compile_expression(compiler, name);
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_LET);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_POP);
  }

  compile_progn_internal(compiler, hll_unwrap_cdr(args));
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_POPENV);
}

#define HLL_CAR_CDR(_lower, _)                                                 \
  static void compile_c##_lower##r(hll_compiler *compiler, hll_value args) {   \
    if (hll_list_length(args) != 2) {                                          \
      compiler_error(compiler, args,                                           \
                     "'c" #_lower "r' expects exactly 1 argument");            \
    } else {                                                                   \
      args = hll_unwrap_cdr(args);                                             \
      compile_eval_expression(compiler, hll_unwrap_car(args));                 \
      const char *ops = #_lower;                                               \
      const char *op = ops + sizeof(#_lower) - 2;                              \
      for (;;) {                                                               \
        if (*op == 'a') {                                                      \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CAR);                \
        } else if (*op == 'd') {                                               \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CDR);                \
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
  if (hll_get_value_kind(location) == HLL_VALUE_SYMB) {
    kind = HLL_LOC_FORM_SYMB;
  } else if (hll_is_cons(location)) {
    hll_value first = hll_unwrap_car(location);
    if (hll_get_value_kind(first) == HLL_VALUE_SYMB) {
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
                                 hll_value value, hll_value reporter) {
  hll_location_form kind = get_location_form(location);
  switch (kind) {
  case HLL_LOC_NONE:
    compiler_error(compiler, reporter, "location is not valid");
    break;
  case HLL_LOC_FORM_SYMB:
    compile_symbol(compiler, location);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_FIND);
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCDR);
    break;
  case HLL_LOC_FORM_NTHCDR:
    if (hll_list_length(location) != 3) {
      compiler_error(compiler, reporter,
                     "'nthcdr' expects exactly 2 arguments");
      break;
    }
    // get the nth function
    compile_symbol(compiler, hll_new_symbolz(compiler->tu->vm, "nthcdr"));
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_FIND);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CDR);
    // call nth
    compile_function_call_internal(compiler, hll_unwrap_cdr(location));
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCDR);
    break;
  case HLL_LOC_FORM_NTH: {
    if (hll_list_length(location) != 3) {
      compiler_error(compiler, reporter, "'nth' expects exactly 2 arguments");
      break;
    }
    // get the nth function
    compile_symbol(compiler, hll_new_symbolz(compiler->tu->vm, "nthcdr"));
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_FIND);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CDR);
    // call nth
    compile_function_call_internal(compiler, hll_unwrap_cdr(location));
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCAR);
  } break;
#define HLL_CAR_CDR(_lower, _upper)                                            \
  case HLL_LOC_FORM_C##_upper##R: {                                            \
    if (hll_list_length(location) != 2) {                                      \
      compiler_error(compiler, reporter,                                       \
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
          hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCAR);             \
        } else if (*op == 'd') {                                               \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCDR);             \
        } else {                                                               \
          HLL_UNREACHABLE;                                                     \
        }                                                                      \
        break;                                                                 \
      } else {                                                                 \
        if (*op == 'a') {                                                      \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CAR);                \
        } else if (*op == 'd') {                                               \
          hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CDR);                \
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
  if (hll_list_length(args) < 2) {
    compiler_error(compiler, args, "'set!' expects at least 1 argument");
    return;
  }

  hll_value location = hll_unwrap_car(hll_unwrap_cdr(args));
  hll_value value = hll_unwrap_cdr(hll_unwrap_cdr(args));
  if (hll_is_cons(value)) {
    assert(hll_is_nil(hll_unwrap_cdr(value)));
    value = hll_unwrap_car(value);
  }

  compile_set_location(compiler, location, value, args);
}

static void compile_setcar(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) != 3) {
    compiler_error(compiler, args, "'setcar!' expects exactly 2 arguments");
    return;
  }

  hll_value location = hll_unwrap_car(hll_unwrap_cdr(args));
  hll_value value = hll_unwrap_cdr(hll_unwrap_cdr(args));
  if (hll_is_cons(value)) {
    assert(hll_is_nil(hll_unwrap_cdr(value)));
    value = hll_unwrap_car(value);
  }

  compile_eval_expression(compiler, location);
  compile_eval_expression(compiler, value);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCAR);
}

static void compile_setcdr(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) != 3) {
    compiler_error(compiler, args, "'setcar!' expects exactly 2 arguments");
    return;
  }

  hll_value location = hll_unwrap_car(hll_unwrap_cdr(args));
  hll_value value = hll_unwrap_cdr(hll_unwrap_cdr(args));
  if (hll_is_cons(value)) {
    assert(hll_get_value_kind(hll_unwrap_cdr(value)) == HLL_VALUE_NIL);
    value = hll_unwrap_car(value);
  }

  compile_eval_expression(compiler, location);
  compile_eval_expression(compiler, value);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCDR);
}

static void compile_list(hll_compiler *compiler, hll_value args) {
  args = hll_unwrap_cdr(args);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  for (hll_value arg = args; !hll_is_nil(arg); arg = hll_unwrap_cdr(arg)) {
    assert(hll_is_cons(arg));
    hll_value obj = hll_unwrap_car(arg);
    compile_eval_expression(compiler, obj);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_APPEND);
  }
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_POP);
}

static void compile_cons(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) != 3) {
    compiler_error(compiler, args, "'cons' expects exactly 2 arguments");
    return;
  }
  args = hll_unwrap_cdr(args);

  hll_value car = hll_unwrap_car(args);
  hll_value cdr = hll_unwrap_car(hll_unwrap_cdr(args));
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  compile_eval_expression(compiler, car);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_APPEND);
  compile_eval_expression(compiler, cdr);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCDR);
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_POP);
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

  hll_value cons = hll_new_cons(compiler->tu->vm, (hll_value)symb, hll_nil());
  if (hll_is_nil(*param_list)) {
    *param_list = *param_list_tail = cons;
  } else {
    hll_unwrap_cons(*param_list_tail)->cdr = cons;
    *param_list_tail = cons;
  }
}

static bool compile_function_internal(hll_compiler *compiler, hll_value params,
                                      hll_value report, hll_value body,
                                      hll_value name, hll_value *compiled_) {
  hll_compiler new_compiler = {0};
  hll_compiler_init(&new_compiler, compiler->tu, name);
  hll_value compiled = hll_compile_ast(&new_compiler, body);
  hll_sb_free(new_compiler.loc_stack);
  if (new_compiler.error_count != 0) {
    compiler->error_count += new_compiler.error_count;
    return false;
  }

  hll_value param_list = hll_nil();
  hll_value param_list_tail = hll_nil();
  if (hll_is_symb(params)) {
    add_symbol_to_function_param_list(&new_compiler, hll_nil(), &param_list,
                                      &param_list_tail);
    add_symbol_to_function_param_list(&new_compiler, params, &param_list,
                                      &param_list_tail);
  } else {
    if (!hll_is_list(params)) {
      compiler_error(compiler, report, "param list must be a list");
      return false;
    }

    hll_value obj = params;
    for (; hll_is_cons(obj); obj = hll_unwrap_cdr(obj)) {
      hll_value car = hll_unwrap_car(obj);
      if (!hll_is_symb(car)) {
        compiler_error(compiler, report, "function param name is not a symbol");
        return false;
      }

      add_symbol_to_function_param_list(&new_compiler, car, &param_list,
                                        &param_list_tail);
    }

    if (!hll_is_nil(obj)) {
      if (!hll_is_symb(obj)) {
        compiler_error(compiler, report, "function param name is not a symbol");
        return false;
      }
      assert(hll_is_cons(param_list_tail));
      hll_unwrap_cons(param_list_tail)->cdr = (hll_value)obj;
    }
  }

  hll_unwrap_func(compiled)->param_names = param_list;
  *compiled_ = compiled;
  return true;
}

static bool compile_function(hll_compiler *compiler, hll_value params,
                             hll_value reporter, hll_value body, hll_value name,
                             uint16_t *idx) {
  hll_value func;
  if (!compile_function_internal(compiler, params, reporter, body, name,
                                 &func)) {
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
  if (hll_list_length(args) < 3) {
    compiler_error(compiler, args, "'lambda' expects at least 2 arguments");
    return;
  }

  hll_value params = hll_unwrap_car(hll_unwrap_cdr(args));
  args = hll_unwrap_cdr(hll_unwrap_cdr(args));
  hll_value body = args;

  uint16_t function_idx;
  if (compile_function(compiler, params, args, body, hll_nil(),
                       &function_idx)) {
    return;
  }

  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_MAKEFUN);
  hll_bytecode_emit_u16(compiler->bytecode, function_idx);
}

static void process_defmacro(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) < 3) {
    compiler_error(compiler, args, "'defmacro' expects at least 2 arguments");
    return;
  }

  hll_value control = hll_unwrap_car(hll_unwrap_cdr(args));
  if (!hll_is_cons(control)) {
    compiler_error(compiler, args,
                   "'defmacro' first argument must be list of macro name and "
                   "its parameters");
    return;
  }

  hll_value name = hll_unwrap_car(control);
  if (!hll_is_symb(name)) {
    compiler_error(compiler, args, "'defmacro' name should be a symbol");
    return;
  }

  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  hll_value params = hll_unwrap_cdr(control);
  hll_value body = hll_unwrap_cdr(hll_unwrap_cdr(args));

  hll_value macro_expansion;
  if (compile_function_internal(compiler, params, args, body, name,
                                &macro_expansion)) {
    if (hll_find_var(compiler->tu->vm->macro_env, name, NULL)) {
      compiler_error(compiler, args, "Macro with same name already exists (%s)",
                     hll_unwrap_zsymb(name));
      return;
    }
    hll_add_variable(compiler->tu->vm, compiler->tu->vm->macro_env, name,
                     macro_expansion);
  }
}

static void compile_define(hll_compiler *compiler, hll_value args) {
  if (hll_list_length(args) == 1) {
    compiler_error(compiler, args, "'define' form expects at least 1 argument");
    return;
  }

  hll_value decide = hll_unwrap_car(hll_unwrap_cdr(args));
  hll_value rest = hll_unwrap_cdr(hll_unwrap_cdr(args));

  if (hll_is_cons(decide)) {
    hll_value name = hll_unwrap_car(decide);
    if (!hll_is_symb(name)) {
      compiler_error(compiler, args, "'define' name should be a symbol");
      return;
    }

    hll_value params = hll_unwrap_cdr(decide);
    hll_value body = rest;

    compile_expression(compiler, name);

    uint16_t function_idx;
    if (compile_function(compiler, params, args, body, name, &function_idx)) {
      return;
    }

    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_MAKEFUN);
    hll_bytecode_emit_u16(compiler->bytecode, function_idx);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_LET);
  } else if (hll_get_value_kind(decide) == HLL_VALUE_SYMB) {
    if (hll_list_length(args) > 3) {
      compiler_error(compiler, args,
                     "'define' variable expects exactly 2 or 3 arguments");
      return;
    }
    hll_value value = rest;
    if (hll_is_cons(value)) {
      assert(hll_get_value_kind(hll_unwrap_cdr(value)) == HLL_VALUE_NIL);
      value = hll_unwrap_car(value);
    }

    compile_expression(compiler, decide);
    compile_eval_expression(compiler, value);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_LET);
  } else {
    compiler_error(compiler, args,
                   "'define' first argument must either be a function name and "
                   "arguments or a variable name");
    return;
  }
}

static void compile_form(hll_compiler *compiler, hll_value args,
                         hll_form_kind kind) {
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
  case HLL_FORM_DEFMACRO:
    process_defmacro(compiler, args);
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

static void compile_eval_expression(hll_compiler *compiler, hll_value ast) {
  switch (hll_get_value_kind(ast)) {
  case HLL_VALUE_NIL:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
    break;
  case HLL_VALUE_TRUE:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_TRUE);
    break;
  case HLL_VALUE_NUM:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CONST);
    hll_bytecode_emit_u16(compiler->bytecode,
                          add_num_const(compiler, hll_unwrap_num(ast)));
    break;
  case HLL_VALUE_CONS: {
    bool pop = compiler_push_location(compiler, ast);
    hll_value fn = hll_unwrap_car(ast);
    hll_form_kind kind = HLL_FORM_REGULAR;
    if (hll_is_symb(fn)) {
      kind = get_form_kind(hll_unwrap_zsymb(fn));
    }

    compile_form(compiler, ast, kind);
    compiler_pop_location(compiler, pop);
  } break;
  case HLL_VALUE_SYMB:
    compile_symbol(compiler, ast);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_FIND);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CDR);
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
  case HLL_VALUE_NIL:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
    break;
  case HLL_VALUE_TRUE:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_TRUE);
    break;
  case HLL_VALUE_NUM:
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_CONST);
    hll_bytecode_emit_u16(compiler->bytecode,
                          add_num_const(compiler, hll_unwrap_num(ast)));
    break;
  case HLL_VALUE_CONS: {
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);

    hll_value obj = ast;
    bool pop = compiler_push_location(compiler, obj);
    while (!hll_is_nil(obj)) {
      assert(hll_is_cons(obj));
      compile_expression(compiler, hll_unwrap_car(obj));
      hll_bytecode_emit_op(compiler->bytecode, HLL_BC_APPEND);

      hll_value cdr = hll_unwrap_cdr(obj);
      if (!hll_is_nil(cdr) && !hll_is_cons(cdr)) {
        compile_expression(compiler, cdr);
        hll_bytecode_emit_op(compiler->bytecode, HLL_BC_SETCDR);
        break;
      }

      obj = cdr;
    }
    compiler_pop_location(compiler, pop);
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_POP);
  } break;
  case HLL_VALUE_SYMB:
    compile_symbol(compiler, ast);
    break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

hll_value hll_compile_ast(hll_compiler *compiler, hll_value ast) {
  hll_value result =
      hll_new_func(compiler->tu->vm, hll_nil(), compiler->bytecode);
  if (hll_is_nil(ast)) {
    hll_bytecode_emit_op(compiler->bytecode, HLL_BC_NIL);
  } else {
    bool pop_ = compiler_push_location(compiler, ast);
    assert(pop_);
    for (; hll_is_cons(ast); ast = hll_unwrap_cdr(ast)) {
      hll_value expr = hll_unwrap_car(ast);
      bool pop = compiler_push_location(compiler, expr);
      compile_eval_expression(compiler, expr);
      if (!hll_is_nil(hll_unwrap_cdr(ast))) {
        hll_bytecode_emit_op(compiler->bytecode, HLL_BC_POP);
      }
      compiler_pop_location(compiler, pop);
    }
    compiler_pop_location(compiler, pop_);
  }
  hll_bytecode_emit_op(compiler->bytecode, HLL_BC_END);
  hll_optimize_bytecode(compiler->bytecode);
  assert(hll_sb_len(compiler->loc_stack) == 0);
  return result;
}
