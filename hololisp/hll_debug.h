#ifndef HLL_DEBUG_H
#define HLL_DEBUG_H

#include <stdarg.h>

#include "hll_hololisp.h"

#define HLL_DEBUG_INFO_HASH_MAP_SIZE 8192

// Debug information is generated for following objects:
// * objects created by parser
//
// Examples when we expect meaningful error message:
// * compile error - source location
// * invalid function invocation - where the function lambda was created
// * runtime error - compiled bytecode mapped to source location
struct hll_obj_debug_info {
  struct hll_obj_debug_info *next;
  uint64_t hash;
  // offset in src
  uint32_t offset;
  // length is src
  uint32_t length;
  // index of src. We store all sources (either that is a string containing
  // lisp source code, or macro invocation).
  uint32_t src_idx;
};

struct hll_debug_storage {
  struct hll_obj_debug_info *obj_debug_infos;


};

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
