//
// hololisp public API definition.
//
#ifndef HLL_HOLOLISP_H
#define HLL_HOLOLISP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
// NOTE: Normally emscripten applications are not defining public interface like
// this because they contain 'main' function that directly references everything
// needed. In our case we are compiling library 'as is', so we need to mark each
// function.
#define HLL_PUB extern EMSCRIPTEN_KEEPALIVE
#else
#define HLL_PUB extern
#endif

struct hll_vm;

typedef uint64_t hll_value;

// Describes function that is used to perform error reporting (it's user
// part). This means that internally, if any part of language encounters
// error it will be handled accordingly. This function is responsible for
// providing output to user.
// We use function pointer here to allow simple silencing of errors
// (setting function to NULL), as well as to provide configurability options.
typedef void hll_error_fn(struct hll_vm *vm, const char *text);

typedef void hll_write_fn(struct hll_vm *vm, const char *text);

struct hll_config {
  // The callback used when user prints message (printf for example).
  // If this is NULL, nothing shall be printed.
  hll_write_fn *write_fn;

  // THe callback used when error is reported.
  // If this is NULL, nothing happens.
  hll_error_fn *error_fn;

  // Initial size of head before first garbage collection.
  // Default value is 10MB
  size_t heap_size;

  // After garbage collection, heap is resized to automatically adjust to
  // amount of memory used. If heap size gets too small, garbage collections
  // may occur more often, Which is not desirable. This value sets minimal
  // possible size for heap.
  // Default value is 1MB
  size_t min_heap_size;

  // After garbage collection the next heap size that should trigger garbage
  // collections is set. This value describes size of heap that should
  // trigger next garbage collection in percent of current size.
  // Default value is 50
  size_t heap_grow_percent;

  // Any data user wants to be accessed through callback functions.
  void *user_data;
};

enum hll_interpret_result {
  HLL_RESULT_OK = 0x0,
  HLL_RESULT_COMPILE_ERROR = 0x1,
  HLL_RESULT_RUNTIME_ERROR = 0x2,
};

// If config is NULL, uses default config.
// Config is copied to the VM.
HLL_PUB
struct hll_vm *hll_make_vm(const struct hll_config *config);

// Deletes VM and frees all its data.
HLL_PUB
void hll_delete_vm(struct hll_vm *vm);

// Runs given source as hololisp code. Name is meta information.
HLL_PUB
enum hll_interpret_result hll_interpret(struct hll_vm *vm, const char *name,
                                        const char *source, bool print_result);

#endif
