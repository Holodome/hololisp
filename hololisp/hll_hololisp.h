///
/// hololisp public API definition.
///
#ifndef __HLL_HOLOLISP_H__
#define __HLL_HOLOLISP_H__

#include <stddef.h>
#include <stdint.h>

struct hll_vm;

typedef enum {
    /// A syntax resolution error detected at compile time.
    HLL_ERR_COMPILE,
    /// Runtime error.
    HLL_ERR_RUNTIME,
} hll_error_kind;

typedef void hll_error_fn(struct hll_vm *vm, hll_error_kind kind,
                          char const *module, uint32_t line, uint32_t column,
                          char const *message);

typedef void hll_write_fn(struct hll_vm *vm, char const *text);

typedef struct hll_config {
    /// The callback used when user prints message (printf for example).
    /// If this is NULL, nothing shall be printed.
    hll_write_fn *write_fn;

    /// The callback used when user reports to errors.
    /// If this is NULL, no errors shall be reported
    hll_error_fn *error_fn;

    /// Initial size of head before first garbage collection.
    /// Default value is 10MB
    size_t heap_size;

    /// After garbage collection, heap is resized to automatically adjust to
    /// amount of memory used. If heap size gets too small, garbage collections
    /// may occur more often, Which is not desirable. This value sets minimal
    /// possible size for heap.
    /// Default value is 1MB
    size_t min_heap_size;

    /// After garbage collection the next heap size that should trigger garbage
    /// collections is set. This value describes size of heap that should
    /// trigger next garbage collection in percent of current size.
    /// Default value is 50
    size_t heap_grow_percent;
} hll_config;

typedef enum {
    HLL_RESULT_OK = 0x0,
    HLL_RESULT_COMPILE_ERROR = 0x1,
    HLL_RESULT_RUNTIME_ERROR = 0x2,
} hll_interpret_result;

/// If config is NULL, uses default config.
/// Config is copied to the VM.
struct hll_vm *hll_make_vm(hll_config const *config);

/// Deletes VM and frees all its data.
void hll_delete_vm(struct hll_vm *vm);

/// Runs given source as hololisp code. Name is meta information.
hll_interpret_result hll_interpret(struct hll_vm *vm, char const *name,
                                   char const *source);

#endif