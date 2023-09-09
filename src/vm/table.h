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

    void init();
    void clear();

    bool get(Value key, Value* value) const;
    bool set(Value key, Value value);

    ObjString* get_string(const char* chars, int32_t length, uint32_t hash);

    bool remove(Value key);

    static void add_all(ObjTable* from, ObjTable* to);
};

ObjTable* create_obj_table();

void free_obj_table(ObjTable* table);
