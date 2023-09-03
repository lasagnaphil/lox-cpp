#include "vm/value.h"

#include "vm/string.h"

#include <cstring>

bool Value::equals(const Value &a, const Value &b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:   return a.as_bool() == b.as_bool();
        case VAL_NIL:    return true;
        case VAL_NUMBER: return a.as_number() == b.as_number();
        case VAL_OBJ:    return a.as_obj() == b.as_obj();
        default:         return false;
    }
}

bool Value::not_equals(const Value &a, const Value &b) {
    if (a.type != b.type) return true;
    switch (a.type) {
        case VAL_BOOL:   return a.as_bool() != b.as_bool();
        case VAL_NIL:    return false;
        case VAL_NUMBER: return a.as_number() != b.as_number();
        case VAL_OBJ:    return a.as_obj() != b.as_obj();
        default:         return false;
    }
}
