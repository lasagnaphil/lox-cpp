#pragma once

#include "vm/value.h"

struct ObjClosure {
    Obj obj;
    ObjFunction* function;

    ObjClosure() : obj(OBJ_CLOSURE) {}
};

ObjClosure* create_obj_closure();

void free_obj_closure(ObjClosure* closure);