#include "vm/string_interner.h"

void StringInterner::init() {
    init_table(&m_strings);
}

void StringInterner::free() {
    free_table(&m_strings);
}

ObjString *StringInterner::create_string(const char *chars, int32_t length) {
    int32_t hash = hash_string(chars, length);
    return create_string(chars, length, hash);
}

ObjString *StringInterner::create_string(const char *chars, int32_t length, uint32_t hash) {
    ObjString* interned = table_find_string(&m_strings, chars, length, hash);
    if (interned != nullptr) {
        return interned;
    }
    else {
        ObjString* new_string = create_obj_string_with_known_hash(chars, length, hash);
        table_set(&m_strings, Value(new_string), Value());
        return new_string;
    }
}
