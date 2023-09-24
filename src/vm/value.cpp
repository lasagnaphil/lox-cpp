#include "vm/value.h"

#include "vm/string.h"
#include "vm/array.h"
#include "vm/table.h"
#include "vm/object.h"

#include <fmt/core.h>

void Value::obj_free() {
    Obj* obj = as_obj();
    switch (obj->type) {
        case OBJ_STRING: {
            free_obj_string(reinterpret_cast<ObjString*>(obj));
            break;
        }
        case OBJ_UPVALUE: {
            free_obj_upvalue(reinterpret_cast<ObjUpvalue*>(obj));
            break;
        }
        case OBJ_ARRAY: {
            free_obj_array(reinterpret_cast<ObjArray*>(obj));
            break;
        }
        case OBJ_TABLE: {
            free_obj_table(reinterpret_cast<ObjTable*>(obj));
            break;
        }
        case OBJ_FUNCTION: {
            free_obj_function(reinterpret_cast<ObjFunction*>(obj));
            break;
        }
        case OBJ_CLOSURE: {
            free_obj_closure(reinterpret_cast<ObjClosure*>(obj));
            break;
        }
        case OBJ_NATIVEFUN: {
            free_obj_native_fun(reinterpret_cast<ObjNativeFun*>(obj));
            break;
        }
        case OBJ_CLASS: {
            free_obj_class(reinterpret_cast<ObjClass*>(obj));
            break;
        }
        case OBJ_INSTANCE: {
            free_obj_instance(reinterpret_cast<ObjInstance*>(obj));
            break;
        }
        case OBJ_BOUND_METHOD: {
            free_obj_bound_method(reinterpret_cast<ObjBoundMethod*>(obj));
            break;
        }
    }
    // Zero-out the pointer for some safety
    value = (QNAN | SIGN_BIT);
}

uint32_t Value::hash() const {
#ifdef LOX_NAN_BOXING
    if (is_nil()) return 0;
    else if (is_bool()) return as_bool()? 1231 : 1237;
    else if (is_number()) {
        double number = as_number();
        auto bits = reinterpret_cast<const char*>(&number);
        return hash_string(bits, 4);
    }
    else if (is_obj()) {
        Obj* obj = as_obj();
        if (obj->type == OBJ_STRING) {
            return reinterpret_cast<ObjString*>(obj)->hash;
        }
        else {
            auto bits = reinterpret_cast<const char *>(&obj);
            return hash_string(bits, 4);
        }
    }
#else
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
#endif
    return 0;
}

bool Value::equals(const Value &a, const Value &b) {
#ifdef LOX_NAN_BOXING
    return a.value == b.value;
#else
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:   return a.as_bool() == b.as_bool();
        case VAL_NIL:    return true;
        case VAL_NUMBER: return a.as_number() == b.as_number();
        case VAL_OBJ:    return a.as_obj() == b.as_obj();
        default:         return false;
    }
#endif
}

std::string object_to_string(Value value, bool print_refcount);

std::string value_to_string(Value value, bool print_refcount) {
    if (value.is_bool()) return value.as_bool()? "true" : "false";
    else if (value.is_nil()) return "nil";
    else if (value.is_number()) return fmt::format("{:g}", value.as_number());
    else if (value.is_obj()) {
        if (print_refcount)
            return fmt::format("{} ({})", object_to_string(value, true), value.as_obj()->refcount);
        else
            return object_to_string(value, false);

    }
    else return "";
}

std::string function_to_string(ObjFunction* fn) {
    if (fn->name == nullptr) {
        return "<script>";
    }
    else {
        return fmt::format("<fn {}>", fn->name->chars);
    }
}

std::string object_to_string(Value value, bool print_refcount) {
    Obj* obj = value.as_obj();
    switch (obj->type) {
        case OBJ_STRING: return value.as_string()->chars;
        case OBJ_UPVALUE: {
            return "upvalue";
        }
        case OBJ_ARRAY: {
            std::string str = "[ ";
            ObjArray* array = value.as_array();
            bool entry_start = true;
            for (int32_t i = 0; i < array->count; i++) {
                Value value = array->values[i];
                if (entry_start) entry_start = false;
                else str += ", ";
                str += value_to_string(value, print_refcount);
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
                    str += value_to_string(entry->key, print_refcount);
                    str += " = ";
                    str += value_to_string(entry->value, print_refcount);
                }
            }
            str += " }";
            return str;
        }
        case OBJ_FUNCTION: {
            return function_to_string(value.as_function());
        }
        case OBJ_CLOSURE: {
            return function_to_string(value.as_closure()->function);
        }
        case OBJ_NATIVEFUN: {
            return "<native fn>";
        }
        case OBJ_CLASS: {
            ObjClass* klass = value.as_class();
            return fmt::format("<class {}>", klass->name->chars);
        }
        case OBJ_INSTANCE: {
            ObjInstance* inst = value.as_instance();
            return fmt::format("<inst {}>", inst->klass->name->chars);
        }
        case OBJ_BOUND_METHOD: {
            return function_to_string(value.as_bound_method()->method->function);
        }
        default: return "";
    }
}

std::string Value::to_std_string(bool print_refcount) const {
    return value_to_string(*this, print_refcount);
}
