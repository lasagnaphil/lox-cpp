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

static Entry* find_entry(Entry* entries, int32_t capacity, ObjString* key) {
    uint32_t index = key->hash % capacity;
    Entry* tombstone = nullptr;
    for (;;) {
        Entry* entry = &entries[index];
        if (entry->key == nullptr) {
            if (entry->value.is_nil()) {
                return tombstone != nullptr ? tombstone : entry;
            }
            else {
                if (tombstone == nullptr) tombstone = entry;
            }
        }
        else if (entry->key == key) {
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(ObjTable* table, int32_t capacity) {
    Entry* entries = new Entry[capacity];
    for (int32_t i = 0; i < capacity; i++) {
        entries[i].key = nullptr;
        entries[i].value = Value();
    }

    table->count = 0;
    for (int32_t i = 0; i < table->capacity; i++) {
        Entry* entry = &table->entries[i];
        if (entry->key == nullptr) continue;

        Entry* dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    delete[] table->entries;
    table->entries = entries;
    table->capacity = capacity;
}

bool table_get(ObjTable* table, ObjString* key, Value* value) {
    if (table->count == 0) return false;
    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == nullptr) return false;

    *value = entry->value;
    return true;
}

bool table_set(ObjTable* table, ObjString* key, Value value) {
    if (table->count + 1 > table->capacity * 3 / 4) {
        int32_t capacity = grow_capacity(table->capacity);
        adjust_capacity(table, capacity);
    }
    Entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key == nullptr;
    if (is_new_key && entry->value.is_nil()) table->count++;

    entry->key = key;
    entry->value = value;
    return is_new_key;
}

bool table_delete(ObjTable* table, ObjString* key) {
    if (table->count == 0) return false;

    Entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == nullptr) return false;

    entry->key = nullptr;
    entry->value = Value(true); // tombstone
    return true;
}

void table_add_all(ObjTable *from, ObjTable *to) {
    for (int32_t i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (entry->key != nullptr) {
            table_set(to, entry->key, entry->value);
        }
    }
}

ObjString *table_find_string(ObjTable *table, const char *chars, int32_t length, int32_t hash) {
    if (table->count == 0) return nullptr;

    uint32_t index = hash % table->capacity;
    for (;;) {
        Entry* entry = &table->entries[index];
        if (entry->key == nullptr) {
            if (entry->value.is_nil()) return nullptr;
        }
        else if (entry->key->length == length &&
                 entry->key->hash == hash &&
                 memcmp(entry->key->chars, chars, length) == 0) {
            return entry->key;
        }
        index = (index + 1) % table->capacity;
    }
}
