#include "hll_debug.h"

#include <assert.h>
#include <stdio.h>

#include "hll_mem.h"
#include "hll_vm.h"

void hll_report_errorv(struct hll_vm *vm, uint32_t translation_unit,
                       uint32_t offset, uint32_t len, const char *fmt,
                       va_list args) {
  (void)translation_unit;
  (void)offset;
  (void)len;
  if (vm == NULL) {
    return;
  }

  if (vm->config.error_fn == NULL) {
    return;
  }

  char buffer[4096];
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  vm->config.error_fn(vm, buffer);
  vm->config.error_fn(vm, "\n");
}

void hll_report_error(struct hll_vm *vm, uint32_t translation_unit,
                      uint32_t offset, uint32_t len, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  hll_report_errorv(vm, translation_unit, offset, len, fmt, args);
  va_end(args);
}

uint32_t hll_ds_init_tu(hll_debug_storage *ds, const char *source) {
  (void)ds;
  (void)source;
  /* assert(0); */
  return 0;
}

hll_debug_storage *hll_make_debug_storage(void) {
  hll_debug_storage *storage = hll_alloc(sizeof(*storage));

  return storage;
}

void hll_delete_debug_storage(hll_debug_storage *ds) {
  hll_sb_free(ds->dtus);
  hll_free(ds, sizeof(*ds));
}
