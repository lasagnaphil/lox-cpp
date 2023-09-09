#include "vm/string.h"

#include <cstring>
#include <cstddef>
#include <cstdlib>
#include <new>

uint32_t hash_string(const char *key, int32_t length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

ObjString *allocate_obj_string(int32_t length) {
    void* raw_data = malloc(offsetof(ObjString, chars[length + 1]));
    new (raw_data) ObjString();
    auto str = static_cast<ObjString*>(raw_data);
    str->length = length;
    return str;
}

ObjString *create_obj_string(const char *chars, int32_t length) {
    auto str = allocate_obj_string(length);
    memcpy(str->chars, chars, length);
    str->hash = hash_string(chars, length);
    str->chars[length] = 0;
    return str;
}

void free_obj_string(ObjString* obj_string) {
    obj_string->~ObjString();
    free(obj_string);
    obj_string = nullptr;
}

ObjString* create_obj_string_with_known_hash(const char* chars, int32_t length, uint32_t hash) {
    auto str = allocate_obj_string(length);
    memcpy(str->chars, chars, length);
    str->hash = hash;
    str->chars[length] = 0;
    return str;
}

ObjString* concat_string(ObjString* a, ObjString* b) {
    int32_t length = a->length + b->length;
    auto result = allocate_obj_string(length);
    memcpy(result->chars, a->chars, a->length);
    memcpy(result->chars + a->length, b->chars, b->length);
    result->hash = hash_string(result->chars, length);
    result->chars[length] = 0;
    return result;
}
