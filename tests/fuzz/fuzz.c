#include "hll_hololisp.h"

#include <stdlib.h>
#include <string.h>

static void error_fn(struct hll_vm *vm, const char *text) {
  (void)vm;
  (void)text;
}

static void write_fn(struct hll_vm *vm, const char *text) {
  (void)vm;
  (void)text;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  void *zero_t = calloc(size + 1, 1);
  memcpy(zero_t, data, size);
  hll_config cfg = {0};
  hll_initialize_default_config(&cfg);
  cfg.error_fn = error_fn;
  cfg.write_fn = write_fn;
  struct hll_vm *vm = hll_make_vm(&cfg);
  hll_interpret_result res = hll_interpret(vm, zero_t, "fuzz", 0);
  hll_delete_vm(vm);
  free(zero_t);
  int result = res == HLL_RESULT_OK ? 0 : -1;
  return result;
}
