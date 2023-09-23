#include "vm/object.h"

template <typename T>
T* new_object() {
    void* raw_data = malloc(sizeof(T));
    T* obj = static_cast<T*>(raw_data);
    new (obj) T();
    return obj;
}

template <typename T>
void delete_object(T* obj) {
    obj->~T();
    free(obj);
}

ObjUpvalue* create_obj_upvalue(Value* slot) {
    ObjUpvalue* upvalue = new_object<ObjUpvalue>();
    upvalue->location = slot;
    upvalue->closed = Value();
    upvalue->next = nullptr;
    return upvalue;
}

void free_obj_upvalue(ObjUpvalue* upvalue) {
    delete_object<ObjUpvalue>(upvalue);
}

ObjFunction *create_obj_function() {
    ObjFunction* fn = new_object<ObjFunction>();
    fn->arity = 0;
    fn->upvalue_count = 0;
    fn->name = nullptr;
    return fn;
}

void free_obj_function(ObjFunction *fn) {
    free(fn->name);
    delete_object(fn);
}

ObjClosure *create_obj_closure(ObjFunction* function) {
    ObjUpvalue** upvalues = static_cast<ObjUpvalue**>(malloc(sizeof(ObjUpvalue*) * function->upvalue_count));
    for (int32_t i = 0; i < function->upvalue_count; i++) {
        upvalues[i] = nullptr;
    }
    ObjClosure* closure = new_object<ObjClosure>();
    closure->function = function;
    closure->upvalues = upvalues;
    closure->upvalue_count = function->upvalue_count;
    return closure;
}

void free_obj_closure(ObjClosure *closure) {
    free(closure->upvalues);
    delete_object(closure);
}

ObjNativeFun* create_obj_native_fun(NativeFun native_fn) {
    ObjNativeFun* fn = new_object<ObjNativeFun>();
    fn->function = native_fn;
    return fn;
}

void free_obj_native_fun(ObjNativeFun* native_fn) {
    delete_object(native_fn);
}

ObjClass *create_obj_class(ObjString *name) {
    ObjClass* klass = new_object<ObjClass>();
    klass->name = name;
    return klass;
}

void free_obj_class(ObjClass *klass) {
    delete_object(klass);
}

ObjInstance *create_obj_instance(ObjClass *klass) {
    ObjInstance* inst = new_object<ObjInstance>();
    inst->klass = klass;
    inst->fields.init();
    return inst;
}

void free_obj_instance(ObjInstance *inst) {
    inst->fields.clear();
    delete_object(inst);
}
