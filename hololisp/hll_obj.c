#include "hll_obj.h"

#include <stdlib.h>
#include <string.h>

#include "hll_vm.h"

hll_obj *
hll_new_nil(struct hll_vm *vm) {
    (void)vm;
    hll_obj *obj = calloc(1, sizeof(hll_obj));
    obj->kind = HLL_OBJ_NIL;
    return obj;
}

hll_obj *
hll_new_true(struct hll_vm *vm) {
    (void)vm;
    hll_obj *obj = calloc(1, sizeof(hll_obj));
    obj->kind = HLL_OBJ_TRUE;
    return obj;
}

hll_obj *
hll_new_num(struct hll_vm *vm, hll_num num) {
    (void)vm;
    hll_obj *obj = calloc(1, sizeof(hll_obj));
    obj->kind = HLL_OBJ_NUM;
    obj->as.num = num;
    return obj;
}

hll_obj *
hll_new_symbol(struct hll_vm *vm, char const *symbol, size_t length) {
    (void)vm;
    hll_obj_symb *body = calloc(1, sizeof(hll_obj_symb) + length + 1);
    memcpy(body->symb, symbol, length);
    body->symb[length] = '\0';

    hll_obj *obj = calloc(1, sizeof(hll_obj));
    obj->kind = HLL_OBJ_SYMB;
    obj->as.body = body;

    return obj;
}

hll_obj *
hll_new_cons(struct hll_vm *vm, hll_obj *car, hll_obj *cdr) {
    (void)vm;
    hll_obj_cons *body = calloc(1, sizeof(hll_obj_cons));
    body->car = car;
    body->cdr = cdr;

    hll_obj *obj = calloc(1, sizeof(hll_obj));
    obj->kind = HLL_OBJ_CONS;
    obj->as.body = body;

    return obj;
}
