#include "hll_gc.h"

#include <assert.h>

#include "hll_bytecode.h"
#include "hll_mem.h"
#include "hll_util.h"
#include "hll_value.h"
#include "hll_vm.h"

static void hll_gray_value(hll_gc *gc, hll_value value) {
  if (!hll_is_obj(value)) {
    return;
  }

  hll_obj *obj = hll_unwrap_obj(value);
  if (obj->is_dark) {
    return;
  }

  obj->is_dark = true;
  hll_sb_push(gc->gray_objs, value);
}

static void hll_blacken_value(hll_gc *gc, hll_value value) {
  if (!hll_is_obj(value)) {
    return;
  }

  hll_obj *obj = hll_unwrap_obj(value);
  gc->bytes_allocated += sizeof(hll_obj);

  switch (obj->kind) {
  case HLL_VALUE_CONS:
    hll_gray_value(gc, hll_unwrap_car(value));
    hll_gray_value(gc, hll_unwrap_cdr(value));
    break;
  case HLL_VALUE_SYMB:
    gc->bytes_allocated +=
        hll_unwrap_symb(value)->length + 1 + sizeof(hll_obj_symb);
    break;
  case HLL_VALUE_BIND:
    gc->bytes_allocated += sizeof(hll_obj_bind);
    break;
  case HLL_VALUE_ENV:
    gc->bytes_allocated += sizeof(hll_obj_env);
    hll_gray_value(gc, hll_unwrap_env(value)->vars);
    hll_gray_value(gc, hll_unwrap_env(value)->up);
    break;
  case HLL_VALUE_FUNC: {
    gc->bytes_allocated += sizeof(hll_obj_func);
    hll_gray_value(gc, hll_unwrap_func(value)->param_names);
    hll_gray_value(gc, hll_unwrap_func(value)->env);
    hll_bytecode *bytecode = hll_unwrap_func(value)->bytecode;
    for (size_t i = 0; i < hll_sb_len(bytecode->constant_pool); ++i) {
      hll_gray_value(gc, bytecode->constant_pool[i]);
    }
  } break;
  case HLL_VALUE_MACRO: {
    gc->bytes_allocated += sizeof(hll_obj_func);
    hll_gray_value(gc, hll_unwrap_macro(value)->param_names);
    hll_gray_value(gc, hll_unwrap_macro(value)->env);
    hll_bytecode *bytecode = hll_unwrap_macro(value)->bytecode;
    for (size_t i = 0; i < hll_sb_len(bytecode->constant_pool); ++i) {
      hll_gray_value(gc, bytecode->constant_pool[i]);
    }
  } break;
  default:
    HLL_UNREACHABLE;
    break;
  }
}

static void hll_collect_garbage(hll_gc *gc) {
  struct hll_vm *vm = gc->vm;

  // Reset allocated bytes count
  gc->bytes_allocated = 0;
  hll_sb_purge(gc->gray_objs);
  hll_gray_value(gc, vm->global_env);
  for (size_t i = 0; i < hll_sb_len(gc->temp_roots); ++i) {
    hll_gray_value(gc, gc->temp_roots[i]);
  }
  for (size_t i = 0; i < hll_sb_len(vm->stack); ++i) {
    hll_gray_value(gc, vm->stack[i]);
  }
  for (size_t i = 0; i < hll_sb_len(vm->call_stack); ++i) {
    hll_call_frame *f = vm->call_stack + i;
    hll_gray_value(gc, f->env);
    hll_gray_value(gc, f->func);
  }
  hll_gray_value(gc, vm->env);

  for (size_t i = 0; i < hll_sb_len(gc->gray_objs); ++i) {
    hll_blacken_value(gc, gc->gray_objs[i]);
  }

  // Free all objects not marked
  hll_obj **obj_ptr = &gc->all_objs;
  while (*obj_ptr != NULL) {
    if (!(*obj_ptr)->is_dark) {
      hll_obj *to_free = *obj_ptr;
      *obj_ptr = to_free->next_gc;
      hll_free_obj(vm, to_free);
    } else {
      (*obj_ptr)->is_dark = false;
      obj_ptr = &(*obj_ptr)->next_gc;
    }
  }

  gc->next_gc = gc->bytes_allocated +
                ((gc->bytes_allocated * vm->config.heap_grow_percent) / 100);
  if (gc->next_gc < vm->config.min_heap_size) {
    gc->next_gc = vm->config.min_heap_size;
  }
}

void *hll_gc_realloc(hll_gc *gc, void *ptr, size_t old_size, size_t new_size) {
  gc->bytes_allocated -= old_size;
  gc->bytes_allocated += new_size;

  if (new_size > 0 && !gc->forbid
#if !HLL_STRESS_GC
      && gc->bytes_allocated > gc->next_gc
#endif
  ) {
    hll_collect_garbage(gc);
  }

  return hll_realloc(ptr, old_size, new_size);
}

hll_gc *hll_make_gc(struct hll_vm *vm) {
  hll_gc *gc = hll_alloc(sizeof(*gc));
  gc->vm = vm;
  gc->next_gc = vm->config.heap_size;

  return gc;
}

void hll_delete_gc(hll_gc *gc) {
  hll_obj *obj = gc->all_objs;
  while (obj != NULL) {
    hll_obj *next = obj->next_gc;
    assert(next != obj);
    hll_free_obj(gc->vm, obj);
    obj = next;
  }
  hll_sb_free(gc->gray_objs);
  hll_sb_free(gc->temp_roots);
  hll_free(gc, sizeof(*gc));
}

void hll_push_forbid_gc(hll_gc *gc) { ++gc->forbid; }
void hll_pop_forbid_gc(hll_gc *gc) {
  assert(gc->forbid);
  --gc->forbid;
}
