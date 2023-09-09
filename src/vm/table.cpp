#include "vm/table.h"

#include "vm/string.h"

#include <new>
#include <cstring>

int32_t grow_capacity(int32_t capacity) {
    return capacity < 8? 8 : capacity * 2;
}

void init_table(ObjTable* table) {
    table->count = 0;
    table->capacity = 0;
    table->entries = nullptr;
}

void free_table(ObjTable* table) {
    delete [] table->entries;
    init_table(table);
}

static Entry* find_entry(Entry* entries, int32_t capacity, Value key) {
    uint32_t index = key.hash() % capacity;
    Entry* tombstone = nullptr;
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key.is_nil()) {
            if (entry->value.is_nil()) {
                return tombstone != nullptr ? tombstone : entry;
            }
            else {
                if (tombstone == nullptr) tombstone = entry;
            }
        }
        else if (Value::equals(entry->key, key)) {
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(ObjTable* table, int32_t capacity) {
    Entry* entries = new Entry[capacity];
    for (int32_t i = 0; i < capacity; i++) {
        entries[i].key = Value();
        entries[i].value = Value();
    }

    table->count = 0;
    for (int32_t i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key.is_nil()) continue;

        Entry* dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    delete[] table->entries;
    table->entries = entries;
    table->capacity = capacity;
}

bool table_get(ObjTable* table, Value key, Value* value) {
    if (table->count == 0) return false;
    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key.is_nil()) return false;

    *value = entry->value;
    if (value->is_obj()) {
        value->obj_incref();
    }
    return true;
}

bool table_set(ObjTable* table, Value key, Value value) {
    if (table->count + 1 > table->capacity * 3 / 4) {
        int32_t capacity = grow_capacity(table->capacity);
        adjust_capacity(table, capacity);
    }
    Entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key.is_nil();
    if (is_new_key && entry->value.is_nil()) table->count++;

    entry->key = key;
    if (entry->value.is_obj()) {
        entry->value.obj_decref();
    }
    entry->value = value;
    return is_new_key;
}

bool table_delete(ObjTable* table, Value key) {
    if (table->count == 0) return false;

    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key.is_nil()) return false;

    entry->key = Value();
    entry->value = Value(true); // tombstone
    return true;
}

void table_add_all(ObjTable *from, ObjTable *to) {
    for (int32_t i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (!entry->key.is_nil()) {
            table_set(to, entry->key, entry->value);
        }
    }
}

ObjString *table_find_string(ObjTable *table, const char *chars, int32_t length, uint32_t hash) {
    if (table->count == 0) return nullptr;

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry* entry = &table->entries[index];
        if (entry->key.is_nil()) {
            if (entry->value.is_nil()) return nullptr;
        }
        else if (entry->key.is_string()) {
            auto key = entry->key.as_string();
            if (key->length == length &&
                key->hash == hash &&
                memcmp(key->chars, chars, length) == 0) {
                return key;
            }
        }
        else
        index = (index + 1) % table->capacity;
    }
}
