#include "vm/native_fun.h"

ObjNativeFun* create_obj_native_fun(NativeFun native_fn) {
    void* raw_data = malloc(sizeof(ObjNativeFun));
    ObjNativeFun* fn = static_cast<ObjNativeFun*>(raw_data);
    new (fn) ObjNativeFun();
    fn->function = native_fn;
    return fn;
}

void free_obj_native_fun(ObjNativeFun* native_fn) {
    native_fn->~ObjNativeFun();
    free(native_fn);
}

