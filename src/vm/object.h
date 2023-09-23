#pragma once

#include "vm/value.h"
#include "vm/chunk.h"
#include "vm/table.h"

struct ObjUpvalue {
    Obj obj = OBJ_UPVALUE;
    Value* location;
    Value closed;
    ObjUpvalue* next;
};

struct ObjFunction {
    Obj obj = OBJ_FUNCTION;
    int32_t arity;
    int32_t upvalue_count;
    Chunk chunk;
    ObjString* name;
};

struct ObjClosure {
    Obj obj = OBJ_CLOSURE;
    ObjFunction* function;
    ObjUpvalue** upvalues;
    int32_t upvalue_count;
};

typedef Value (*NativeFun)(int arg_count, Value* args);

struct ObjNativeFun {
    Obj obj = OBJ_NATIVEFUN;
    NativeFun function;
};

struct ObjClass {
    Obj obj = OBJ_CLASS;
    ObjString* name;
};

struct ObjInstance {
    Obj obj = OBJ_INSTANCE;
    ObjClass* klass;
    ObjTable fields;
};

ObjUpvalue* create_obj_upvalue(Value* slot);
void free_obj_upvalue(ObjUpvalue* upvalue);

ObjFunction* create_obj_function();
void free_obj_function(ObjFunction* fn);

ObjClosure* create_obj_closure(ObjFunction* function);
void free_obj_closure(ObjClosure* closure);

ObjNativeFun* create_obj_native_fun(NativeFun native_fn);
void free_obj_native_fun(ObjNativeFun* native_fn);

ObjClass* create_obj_class(ObjString* name);
void free_obj_class(ObjClass* klass);

ObjInstance* create_obj_instance(ObjClass* klass);
void free_obj_instance(ObjInstance* inst);