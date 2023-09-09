#include "vm/value.h"

#include "vm/string.h"

#include <cstring>

uint32_t Value::hash() const {
    switch (type) {
        case VAL_NIL: return 0;
        case VAL_BOOL: return as.boolean? 1231 : 1237;
        case VAL_NUMBER: {
            auto bits = reinterpret_cast<const char*>(&as.number);
            return hash_string(bits, 4);
        }
        case VAL_OBJ: {
            if (as.obj->type == OBJ_STRING) {
                return reinterpret_cast<ObjString *>(as.obj)->hash;
            }
            else {
                auto bits = reinterpret_cast<const char *>(&as.obj);
                return hash_string(bits, 4);
            }
        }
    }
    return 0;
}

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
