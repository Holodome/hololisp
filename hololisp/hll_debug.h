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

struct hll_bytecode;
struct hll_call_frame;

typedef struct hll_loc {
  uint32_t translation_unit;
  uint32_t offset;
} hll_loc;

// Debug Translation Unit.
typedef struct {
  const char *source;
  const char *name;
} hll_dtu;

typedef uint32_t hll_debug_flags;
enum {
  HLL_DEBUG_DIAGNOSTICS_COLORED = 0x1,
};

typedef struct hll_debug_storage {
  struct hll_vm *vm;

  hll_dtu *dtus;
  hll_debug_flags flags;

  uint32_t error_count;
} hll_debug_storage;

hll_debug_storage *hll_make_debug(struct hll_vm *vm, hll_debug_flags flags);
void hll_delete_debug(hll_debug_storage *ds);
void hll_reset_debug(hll_debug_storage *ds);

uint32_t hll_ds_init_tu(hll_debug_storage *ds, const char *source,
                        const char *name);

// Used to report error in current state contained by vm.
// vm must have current_filename field present if message needs to include
// source location.
// offset specifies byte offset of reported location in file.
// len specifies length of reported part (e.g. token).
void hll_report_errorv(hll_debug_storage *debug, hll_loc loc, const char *fmt,
                       va_list args);
void hll_report_error(hll_debug_storage *debug, hll_loc loc, const char *fmt,
                      ...) __attribute__((format(printf, 3, 4)));

void hll_report_runtime_errorv(hll_debug_storage *debug, const char *fmt,
                               va_list args);

void hll_debug_print_summary(hll_debug_storage *debug);

#endif
