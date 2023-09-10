#pragma once

#include "vm/value.h"

struct ObjArray {
    Obj obj;
    int32_t count;
    int32_t capacity;
    Value* values;

    ObjArray() : obj(OBJ_ARRAY) {}

    void init();
    void clear();

    bool get(int32_t index, Value* value) const;
    bool set(int32_t index, Value value);

    void resize(int32_t count);

    void push(Value value);
    bool pop(Value* value);
};

ObjArray* create_obj_array();

void free_obj_array(ObjArray* array);