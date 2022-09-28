#include "hll_debug.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>

#include "hll_mem.h"
#include "hll_vm.h"

void hll_report_errorv(hll_debug_storage *debug, hll_loc loc, const char *fmt,
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

  const char *dtu_source = dtu->source;

  size_t line_number = 0;
  size_t column_number = 0;
  const char *cursor = dtu_source;
  const char *last_line_start = dtu_source;
  while (cursor < dtu_source + loc.offset) {
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

  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[1m");
  }
  char buffer[4096];
  snprintf(buffer, sizeof(buffer), "%s:%zu:%zu: ", dtu->name, line_number,
           column_number);
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

  snprintf(buffer, sizeof(buffer), "%.*s\n", (int)(line_end - last_line_start),
           last_line_start);

  error_fn(debug->vm, buffer);

  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[32;1m");
  }

  if (cursor - last_line_start != 0) {
    snprintf(buffer, sizeof(buffer), "%*c^\n", (int)(cursor - last_line_start),
             ' ');
    error_fn(debug->vm, buffer);
  } else {
    snprintf(buffer, sizeof(buffer), "^\n");
    error_fn(debug->vm, buffer);
  }

  if (debug->flags & HLL_DEBUG_DIAGNOSTICS_COLORED) {
    error_fn(debug->vm, "\033[0m");
  }
}

void hll_report_error(hll_debug_storage *vm, hll_loc loc, const char *fmt,
                      ...) {
  va_list args;
  va_start(args, fmt);
  hll_report_errorv(vm, loc, fmt, args);
  va_end(args);
}

uint32_t hll_ds_init_tu(hll_debug_storage *ds, const char *source,
                        const char *name) {
  hll_dtu tu = {.source = source, .name = name};
  hll_sb_push(ds->dtus, tu);
  return hll_sb_len(ds->dtus);
}

hll_debug_storage *hll_make_debug(hll_vm *vm, hll_debug_flags flags) {
  hll_debug_storage *storage = hll_alloc(sizeof(*storage));
  storage->flags = flags;
  storage->vm = vm;

  return storage;
}

void hll_delete_debug(hll_debug_storage *ds) {
  hll_sb_free(ds->dtus);
  hll_free(ds, sizeof(*ds));
}

void hll_reset_debug(hll_debug_storage *ds) { ds->error_count = 0; }

void hll_debug_print_summary(hll_debug_storage *debug) {
  char buffer[1024];
  snprintf(buffer, sizeof(buffer), "%" PRIu32 " errors generated.\n",
           debug->error_count);

  hll_error_fn *error_fn = debug->vm->config.error_fn;
  if (error_fn != NULL) {
    error_fn(debug->vm, buffer);
  }
}
