//
// hll_debug.h
//
// Debug information is generated for following objects:
// * objects created by parser
//
// Examples when we expect meaningful error message:
// * compile error - source location
// * invalid function invocation - where the function lambda was created
// * runtime error - compiled bytecode mapped to source location
//
#ifndef HLL_DEBUG_H
#define HLL_DEBUG_H

#include <stdarg.h>

#include "hll_hololisp.h"

// Debug Translation Unit.
typedef struct {
  const char *source;
} hll_dtu;

typedef struct hll_debug_storage {
  hll_dtu *dtus;
} hll_debug_storage;

hll_debug_storage hll_make_debug_storage(void);
void hll_delete_debug_storage(hll_debug_storage *ds);

uint32_t hll_ds_init_tu(hll_debug_storage *ds, const char *source);

// Used to report error in current state contained by vm.
// vm must have current_filename field present if message needs to include
// source location.
// offset specifies byte offset of reported location in file.
// len specifies length of reported part (e.g. token).
void hll_report_errorv(struct hll_vm *vm, size_t offset, uint32_t len,
                       const char *fmt, va_list args);
void hll_report_error(struct hll_vm *vm, size_t offset, uint32_t len,
                      const char *fmt, ...)
    __attribute__((format(printf, 4, 5)));
void hll_report_error_value(struct hll_vm *vm, hll_value value, const char *msg,
                            ...) __attribute__((format(printf, 3, 4)));
void hll_report_error_valuev(struct hll_vm *vm, hll_value value,
                             const char *msg, va_list args);

#endif
