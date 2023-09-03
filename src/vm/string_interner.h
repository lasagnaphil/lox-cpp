#pragma once

#include "vm/string.h"
#include "vm/table.h"

ObjString* table_find_string(ObjTable* table, const char* chars, int32_t length, int32_t hash);

class StringInterner {
public:
    void init();
    void free();

    ObjString* create_string(const char* chars, int32_t length);

    ObjString* create_string(const char* chars, int32_t length, int32_t hash);

    void free_string(ObjString* str);

private:
    ObjTable m_strings;

};