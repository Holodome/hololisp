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

#ifdef HLL_DEBUG
#if defined(__GNUC__) || defined(__clang__)
#define HLL_UNREACHABLE __builtin_trap()
#else
#error Not implemented
#endif
#else // HLL_DEBUG
#if defined(__GNUC__) || defined(__clang__)
#define HLL_UNREACHABLE __builtin_unreachable()
#else
#error Not implemented
#endif
#endif // HLL_DEBUG

#define HLL_CALL_STACK_SIZE 1024
#define HLL_STACK_SIZE (1024 * 1024)

// Instance of hololisp virtual machine. Virtual machine stores state
// of execution hololisp code in a single session, e.g. REPL session.
struct hll_vm;

// Type of 'value' in language. By 'value' we understand minimal unit of
// language that interpreter can operate on. 'values' can be, for example,
// numbers, conses, symbols.
typedef uint64_t hll_value;

// This function is responsible for providing error output to user.
typedef void hll_error_fn(struct hll_vm *vm, const char *text);

// Function that is used in 'print' statements.
typedef void hll_write_fn(struct hll_vm *vm, const char *text);

// Stores configuration of vm runtime.
// Config cannot be changed after virtual machine creation.
typedef struct hll_config {
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
} hll_config;

void hll_initialize_default_config(hll_config *config);

// Result of calling hll_interpret.
// Contains minimal information about source of error.
typedef enum {
  HLL_RESULT_OK = 0x0,
  HLL_RESULT_ERROR = 0x1,
} hll_interpret_result;

enum {
  // Print result of execution. This may be needed depending on context:
  // for example, when execution REPL result print is needed, while when
  // executing files this is unwanted.
  HLL_INTERPRET_PRINT_RESULT = 0x1,
  // Make output of error messages be colored using ANSI escape codes.
  HLL_INTERPRET_COLORED = 0x2
};

// If config is NULL, uses default config.
// Config is copied to the VM.
HLL_PUB struct hll_vm *hll_make_vm(const struct hll_config *config);

// Deletes VM and frees all its data.
HLL_PUB void hll_delete_vm(struct hll_vm *vm) __attribute__((nonnull));

// Runs given source as hololisp code.
HLL_PUB hll_interpret_result hll_interpret(struct hll_vm *vm,
                                           const char *source, const char *name,
                                           uint32_t flags)
    __attribute__((nonnull));

#endif
