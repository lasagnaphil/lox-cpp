#include "vm/function.h"

ObjFunction *create_obj_function() {
    void* raw_data = malloc(sizeof(ObjFunction));
    new (raw_data) ObjFunction();
    ObjFunction* fn = static_cast<ObjFunction*>(raw_data);
    fn->arity = 0;
    fn->upvalue_count = 0;
    fn->name = nullptr;
    return fn;
}

void free_obj_function(ObjFunction *fn) {
    free(fn->name);
    fn->~ObjFunction();
    free(fn);
}

ObjClosure *create_obj_closure(ObjFunction* function) {
    ObjUpvalue** upvalues = static_cast<ObjUpvalue**>(malloc(sizeof(ObjUpvalue*) * function->upvalue_count));
    for (int32_t i = 0; i < function->upvalue_count; i++) {
        upvalues[i] = nullptr;
    }
    void* raw_data = malloc(sizeof(ObjClosure));
    ObjClosure* closure = static_cast<ObjClosure*>(raw_data);
    new (closure) ObjClosure();
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;
    return closure;
}

void free_obj_closure(ObjClosure *closure) {
    free(closure->upvalues);
    closure->~ObjClosure();
    free(closure);
}

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

ObjUpvalue* create_obj_upvalue(Value* slot) {
    void* raw_data = malloc(sizeof(ObjUpvalue));
    ObjUpvalue* upvalue = static_cast<ObjUpvalue*>(raw_data);
    new (upvalue) ObjUpvalue();
    upvalue->location = slot;
    return upvalue;
}

void free_obj_upvalue(ObjUpvalue* upvalue) {
    upvalue->~ObjUpvalue();
    free(upvalue);
}
