#pragma once

#include "vm/string.h"
#include "vm/table.h"

class StringInterner {
public:
    void init();
    void free();

    ObjString* create_string(const char* chars, int32_t length);

    ObjString* create_string(const char* chars, int32_t length, uint32_t hash);

    void free_string(ObjString* str);

private:
    ObjTable m_strings;

};