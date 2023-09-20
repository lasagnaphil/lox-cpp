#pragma once

#include "vm/value.h"
#include "vm/chunk.h"

struct ObjUpvalue {
    Obj obj = OBJ_UPVALUE;
    Value* location;
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

ObjFunction* create_obj_function();
void free_obj_function(ObjFunction* fn);

ObjClosure* create_obj_closure(ObjFunction* function);
void free_obj_closure(ObjClosure* closure);

ObjNativeFun* create_obj_native_fun(NativeFun native_fn);
void free_obj_native_fun(ObjNativeFun* native_fn);

ObjUpvalue* create_obj_upvalue(Value* slot);
void free_obj_upvalue(ObjUpvalue* upvalue);
