#pragma once

#include "vm/value.h"

typedef Value (*NativeFun)(int arg_count, Value* args);

struct ObjNativeFun {
    Obj obj;
    NativeFun function;

    ObjNativeFun() : obj(OBJ_NATIVEFUN) {}
};

ObjNativeFun* create_obj_native_fun(NativeFun native_fn);
void free_obj_native_fun(ObjNativeFun* native_fn);
