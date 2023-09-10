#pragma once

#include "vm/value.h"
#include "vm/chunk.h"

struct ObjFunction {
    Obj obj;
    int32_t arity;
    Chunk chunk;
    ObjString* name;

    ObjFunction() : obj(OBJ_FUNCTION) {}

    void init();
    void clear();
};

ObjFunction* create_obj_function();

void free_obj_function(ObjFunction* fn);