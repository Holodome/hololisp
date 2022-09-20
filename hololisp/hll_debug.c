#include "hll_debug.h"

#include <stdio.h>
#include <assert.h>

#include "hll_vm.h"

void hll_report_errorv(struct hll_vm *vm, size_t offset, uint32_t len,
                       const char *fmt, va_list args) {
  (void)offset;
  (void)len;
  if (vm == NULL) {
    return;
  }

  ++vm->error_count;
  if (vm->config.error_fn == NULL) {
    return;
  }

  char buffer[4096];
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  vm->config.error_fn(vm, buffer);
  vm->config.error_fn(vm, "\n");
}
void hll_report_error(struct hll_vm *vm, size_t offset, uint32_t len,
                      const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  hll_report_errorv(vm, offset, len, fmt, args);
  va_end(args);
}
void hll_report_error_value(struct hll_vm *vm, hll_value value, const char *msg,
                            ...) {
  va_list args;
  va_start(args, msg);
  hll_report_error_valuev(vm, value, msg, args);
  va_end(args);
}
void hll_report_error_valuev(struct hll_vm *vm, hll_value value,
                             const char *msg, va_list args) {
  assert(0);
  (void)vm;
  (void)value;
  (void)msg;
  (void)args;
}
