///
/// hololisp public API definition.
///
#ifndef HLL_HOLOLISP_H
#define HLL_HOLOLISP_H

#include <stddef.h>
#include <stdint.h>

struct hll_vm;

#ifndef HLL_NUMBER_TYPE
typedef double hll_num;
#else
typedef double HLL_NUMBER_TYPE
#endif

typedef void hll_error_fn(struct hll_vm *vm, uint32_t line, uint32_t column,
                          char const *message);

typedef void hll_write_fn(struct hll_vm *vm, char const *text);

typedef struct hll_config {
    /// The callback used when user prints message (printf for example).
    /// If this is NULL, nothing shall be printed.
    hll_write_fn *write_fn;

    /// THe callback used when error is reported.
    /// If this is NULL, nothing happens.
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

    /// Any data user wants to be accessed through callback functions.
    void *user_data;
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
hll_interpret_result hll_interpret(struct hll_vm *vm, char const *source);

#endif
