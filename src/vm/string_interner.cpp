#include "vm/string_interner.h"

void StringInterner::init() {
    m_strings.init();
}

void StringInterner::free() {
    m_strings.clear();
}

ObjString* StringInterner::create_string(const char *chars, int32_t length) {
    int32_t hash = hash_string(chars, length);
    return create_string(chars, length, hash);
}

ObjString* StringInterner::create_string(const char *chars, int32_t length, uint32_t hash) {
    ObjString* interned = m_strings.get_string(chars, length, hash);
    if (interned != nullptr) {
        return interned;
    }
    else {
        ObjString* new_string = create_obj_string_with_known_hash(chars, length, hash);
        auto value = Value(new_string);
        value.obj_incref();
        m_strings.set(value, Value());
        return new_string;
    }
}
