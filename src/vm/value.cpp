#include "vm/value.h"

#include "vm/string.h"
#include "vm/array.h"
#include "vm/table.h"
#include "vm/function.h"
#include "vm/native_fun.h"

#include <fmt/core.h>

void Value::obj_free() {
    switch (as.obj->type) {
        case OBJ_STRING: {
            free_obj_string(reinterpret_cast<ObjString*>(as.obj));
            break;
        }
        case OBJ_ARRAY: {
            free_obj_array(reinterpret_cast<ObjArray*>(as.obj));
            break;
        }
        case OBJ_TABLE: {
            free_obj_table(reinterpret_cast<ObjTable*>(as.obj));
            break;
        }
        case OBJ_FUNCTION: {
            free_obj_function(reinterpret_cast<ObjFunction*>(as.obj));
            break;
        }
        case OBJ_NATIVEFUN: {
            free_obj_native_fun(reinterpret_cast<ObjNativeFun*>(as.obj));
            break;
        }
    }
    as.obj = nullptr;
}

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

std::string object_to_string(Value value);

std::string value_to_string(Value value) {
    switch (value.type) {
        case VAL_BOOL: return value.as_bool()? "true": "false";
        case VAL_NIL: return "nil";
        case VAL_NUMBER: return fmt::format("{:g}", value.as_number());
        case VAL_OBJ: return object_to_string(value);
    }
}

std::string object_to_string(Value value) {
    switch (value.as.obj->type) {
        case OBJ_STRING: return value.as_string()->chars;
        case OBJ_ARRAY: {
            std::string str = "[ ";
            ObjArray* array = value.as_array();
            bool entry_start = true;
            for (int32_t i = 0; i < array->count; i++) {
                Value value = array->values[i];
                if(!value.is_nil()) {
                    if (entry_start) entry_start = false;
                    else str += ", ";
                    str += value_to_string(value);
                }
            }
            str += " ]";
            return str;
        }
        case OBJ_TABLE: {
            std::string str = "{ ";
            ObjTable* table = value.as_table();
            bool entry_start = true;
            for (int32_t i = 0; i < table->capacity; i++) {
                Entry* entry = &table->entries[i];
                if (!entry->key.is_nil()) {
                    if (entry_start) entry_start = false;
                    else str += ", ";
                    str += value_to_string(entry->key);
                    str += " = ";
                    str += value_to_string(entry->value);
                }
            }
            str += " }";
            return str;
        }
        case OBJ_FUNCTION: {
            ObjFunction* fn = value.as_function();
            if (fn->name == nullptr) {
                return "<script>";
            }
            else {
                return fmt::format("<fn {}>", fn->name->chars);
            }
        }
        case OBJ_NATIVEFUN: {
            return "<native fn>";
        }
    }
}

std::string Value::to_std_string() const {
    return value_to_string(*this);
}
