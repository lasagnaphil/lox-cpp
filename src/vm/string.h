#pragma once

#include "vm/value.h"

struct ObjString {
    Obj obj;
    int32_t length;
    uint32_t hash;
    char chars[];

    ObjString() : obj(OBJ_STRING) {}
};

int32_t hash_string(const char *key, int32_t length);

ObjString* allocate_obj_string(int32_t length);

ObjString* create_obj_string(const char* chars, int32_t length);

ObjString* create_obj_string_with_known_hash(const char* chars, int32_t length, int32_t hash);

ObjString* concat_string(ObjString* a, ObjString* b);
