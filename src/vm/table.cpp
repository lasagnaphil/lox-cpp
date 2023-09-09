#include "vm/table.h"

#include "vm/string.h"

#include <new>
#include <cstring>

int32_t grow_capacity(int32_t capacity) {
    return capacity < 8? 8 : capacity * 2;
}

ObjTable *create_obj_table() {
    void* raw_data = malloc(sizeof(ObjTable));
    new (raw_data) ObjTable();
    ObjTable* table = static_cast<ObjTable*>(raw_data);
    table->init();
    return table;
}

void free_obj_table(ObjTable* table) {
    table->clear();
    table->~ObjTable();
    free(table);
}

void ObjTable::init() {
    count = 0;
    capacity = 0;
    entries = nullptr;
}

void ObjTable::clear() {
    for (int32_t i = 0; i < capacity; i++) {
        Entry* entry = &entries[i];
        if (!entry->key.is_nil()) {
            if (entry->key.is_obj()) entry->key.obj_decref();
            if (entry->value.is_obj()) entry->value.obj_decref();
        }
    }
    free(entries);
    init();
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

bool ObjTable::get(Value key, Value *value) const {
    if (count == 0) return false;
    Entry* entry = find_entry(entries, capacity, key);
    if (entry->key.is_nil()) return false;

    *value = entry->value;
    if (value->is_obj()) {
        value->obj_incref();
    }
    return true;
}

static void adjust_capacity(ObjTable* table, int32_t capacity) {
    Entry* entries = static_cast<Entry*>(malloc(sizeof(Entry) * capacity));
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

    free(table->entries);
    table->entries = entries;
    table->capacity = capacity;
}

bool ObjTable::set(Value key, Value value) {
    if (count + 1 > capacity * 3 / 4) {
        int32_t new_capacity = grow_capacity(capacity);
        adjust_capacity(this, new_capacity);
    }
    Entry* entry = find_entry(entries, capacity, key);
    bool is_new_key = entry->key.is_nil();
    if (is_new_key) {
        entry->key = key;
        count++;
    }
    else {
        if (entry->value.is_obj()) entry->value.obj_decref();
    }
    entry->value = value;
    return is_new_key;
}

ObjString *ObjTable::get_string(const char *chars, int32_t length, uint32_t hash) {
    if (count == 0) return nullptr;

    uint32_t index = hash % capacity;
    for (;;) {
        Entry* entry = &entries[index];
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
        index = (index + 1) % capacity;
    }
}

bool ObjTable::remove(Value key) {
    if (count == 0) return false;

    Entry* entry = find_entry(entries, capacity, key);
    if (entry->key.is_nil()) return false;

    if (entry->key.is_obj()) entry->key.obj_decref();
    if (entry->value.is_obj()) entry->value.obj_decref();

    entry->key = Value();
    entry->value = Value(true); // tombstone
    return true;
}

void ObjTable::add_all(ObjTable *from, ObjTable *to) {
    for (int32_t i = 0; i < from->capacity; i++) {
        Entry* entry = &from->entries[i];
        if (!entry->key.is_nil()) {
            to->set(entry->key, entry->value);
        }
    }
}
