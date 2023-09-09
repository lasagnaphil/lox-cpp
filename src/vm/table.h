#pragma once

#include "vm/value.h"

struct Entry {
    Value key;
    Value value;
};

struct ObjTable {
    Obj obj;
    int32_t count;
    int32_t capacity;
    Entry* entries;

    ObjTable() : obj(OBJ_TABLE) {}
};

void init_table(ObjTable* table);

void free_table(ObjTable* table);

bool table_get(ObjTable* table, Value key, Value* value);

bool table_set(ObjTable* table, Value key, Value value);

bool table_delete(ObjTable* table, Value key);

void table_add_all(ObjTable* from, ObjTable* to);

ObjString* table_find_string(ObjTable* table, const char* chars, int32_t length, uint32_t hash);
