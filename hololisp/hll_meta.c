#include "hll_meta.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "hll_bytecode.h"
#include "hll_mem.h"
#include "hll_vm.h"

typedef struct {
  const char *line_start;
  const char *line_end;
  size_t line;
  size_t column;
} hll_src_loc_info;

static hll_src_loc_info get_src_loc_info(const char *src, size_t offset) {
  size_t line_number = 0;
  size_t column_number = 0;
  const char *cursor = src;
  const char *last_line_start = src;
  while (cursor < src + offset) {
    int symb = *cursor++;
    assert(symb != '\0');

    if (symb == '\n') {
      ++line_number;
      column_number = 0;
      last_line_start = cursor;
    } else {
      ++column_number;
    }
  }

  ++line_number;
  ++column_number;
  const char *line_end = last_line_start;
  while (*line_end && *line_end != '\n') {
    ++line_end;
  }

  return (hll_src_loc_info){.line_start = last_line_start,
                            .line_end = line_end,
                            .line = line_number,
                            .column = column_number};
}

void hll_report_errorv(hll_meta_storage *debug, hll_loc loc, const char *fmt,
                       va_list args) {
  ++debug->error_count;

  hll_error_fn *error_fn = debug->vm->config.error_fn;
  if (error_fn == NULL) {
    return;
  }

  if (loc.translation_unit == 0) {
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    error_fn(debug->vm, buffer);
    error_fn(debug->vm, "\n");
    return;
  }

  assert(loc.translation_unit <= hll_sb_len(debug->dtus));
  hll_dtu *dtu = debug->dtus + loc.translation_unit - 1;
  hll_src_loc_info info = get_src_loc_info(dtu->source, loc.offset);

  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[1m");
  }
  char buffer[4096];
  snprintf(buffer, sizeof(buffer), "%s:%zu:%zu: ", dtu->name, info.line,
           info.column);
  error_fn(debug->vm, buffer);

  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[31;1m");
  }
  error_fn(debug->vm, "error: ");
  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[0m");
    error_fn(debug->vm, "\033[1m");
  }

  vsnprintf(buffer, sizeof(buffer), fmt, args);
  error_fn(debug->vm, buffer);
  error_fn(debug->vm, "\n");

  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[0m");
  }

  snprintf(buffer, sizeof(buffer), "%.*s\n",
           (int)(info.line_end - info.line_start), info.line_start);

  error_fn(debug->vm, buffer);

  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[32;1m");
  }

  if (info.column != 1) {
    snprintf(buffer, sizeof(buffer), "%*c^\n", (int)info.column - 1, ' ');
    error_fn(debug->vm, buffer);
  } else {
    snprintf(buffer, sizeof(buffer), "^\n");
    error_fn(debug->vm, buffer);
  }

  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[0m");
  }
}

void hll_report_error(hll_meta_storage *vm, hll_loc loc, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  hll_report_errorv(vm, loc, fmt, args);
  va_end(args);
}

uint32_t hll_ds_init_tu(hll_meta_storage *ds, const char *source,
                        const char *name) {
  hll_dtu tu = {.source = source, .name = name};
  hll_sb_push(ds->dtus, tu);
  return hll_sb_len(ds->dtus);
}

hll_meta_storage *hll_make_debug(hll_vm *vm, uint32_t flags) {
  hll_meta_storage *storage = hll_alloc(sizeof(*storage));
  storage->flags = flags;
  storage->vm = vm;
  return storage;
}

void hll_delete_debug(hll_meta_storage *ds) {
  for (size_t i = 0; i < hll_sb_len(ds->dtus); ++i) {
    hll_dtu *dtu = ds->dtus + i;
    hll_sb_free(dtu->locs);
    hll_sb_free(dtu->loc_rle);
  }
  hll_sb_free(ds->dtus);
  hll_free(ds, sizeof(*ds));
}

void hll_reset_debug(hll_meta_storage *ds) { ds->error_count = 0; }

void hll_debug_print_summary(hll_meta_storage *debug) {
  char buffer[1024];
  if (debug->error_count != 1) {
    snprintf(buffer, sizeof(buffer), "%" PRIu32 " errors generated.\n",
             debug->error_count);
  } else {
    snprintf(buffer, sizeof(buffer), "%" PRIu32 " error generated.\n",
             debug->error_count);
  }

  hll_error_fn *error_fn = debug->vm->config.error_fn;
  if (error_fn != NULL) {
    error_fn(debug->vm, buffer);
  }
}

void hll_report_runtime_errorv(hll_meta_storage *debug, const char *fmt,
                               va_list args) {
  hll_call_frame *f = debug->vm->call_stack - 1;
  size_t op_idx = f->ip - f->bytecode->ops;
  assert(op_idx);
  --op_idx;
  uint32_t offset =
      hll_bytecode_get_loc(debug, f->bytecode->translation_unit, op_idx);
  uint32_t translation_unit = f->bytecode->translation_unit;
  hll_report_errorv(
      debug, (hll_loc){.offset = offset, .translation_unit = translation_unit},
      fmt, args);
}

void hll_bytecode_add_loc(hll_meta_storage *debug, uint32_t translation_unit,
                          size_t op_length, uint32_t compilation_unit,
                          uint32_t offset) {
  hll_dtu *dtu = debug->dtus + translation_unit - 1;
  assert(dtu <= &hll_sb_last(debug->dtus));
  hll_loc bc_loc = {
      .translation_unit = compilation_unit,
      .offset = offset,
  };
  hll_sb_push(dtu->locs, bc_loc);

  assert(op_length);
  hll_bytecode_rle rle = {.length = op_length,
                          .loc_idx = hll_sb_len(dtu->locs) - 1};
  hll_sb_push(dtu->loc_rle, rle);
}

uint32_t hll_bytecode_get_loc(hll_meta_storage *debug,
                              uint32_t translation_unit, size_t op_idx) {
  hll_dtu *dtu = debug->dtus + translation_unit - 1;
  assert(dtu <= &hll_sb_last(debug->dtus));
  size_t cursor = 0;
  hll_bytecode_rle *rle = dtu->loc_rle;
  if (rle == NULL) {
    return 0;
  }
  for (;;) {
    if (cursor <= op_idx && op_idx < cursor + rle->length) {
      break;
    }
    cursor += rle->length;
    ++rle;

    assert(rle <= &hll_sb_last(dtu->loc_rle));
  }

  return dtu->locs[rle->loc_idx].offset;
}
