#pragma once

#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <string>

#define LOX_NAN_BOXING

enum ValueType {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
};

enum ObjType {
    OBJ_STRING,
    OBJ_UPVALUE,
    OBJ_ARRAY,
    OBJ_TABLE,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_NATIVEFUN,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_BOUND_METHOD
};

// https://en.wikipedia.org/wiki/Xorshift
inline uint32_t gen_random_uid() {
    // TODO: Make this atomic
    // TODO: Randomize state
    static uint32_t state = 0xb0bacafe;
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state = x;
}

struct Obj {
    ObjType type : 5;
    uint32_t uid : 27; // TODO: make this 64-bit?
    uint32_t refcount; // TODO: make this atomic

    Obj(ObjType type_) : type(type_), uid(gen_random_uid()), refcount(1) {}
};

struct ObjString;
struct ObjArray;
struct ObjTable;
struct ObjFunction;
struct ObjClosure;
struct ObjNativeFun;
struct ObjClass;
struct ObjInstance;
struct ObjBoundMethod;

struct Value {

#ifdef LOX_NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN ((uint64_t)0x7ffc000000000000)
#define TAG_NIL 1
#define TAG_FALSE 2
#define TAG_TRUE 3

#define NIL_VAL         ((uint64_t)(QNAN | TAG_NIL))
#define FALSE_VAL       ((uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL        ((uint64_t)(QNAN | TAG_TRUE))

    uint64_t value;

    Value() : value((uint64_t)(QNAN | TAG_NIL)) {}

    // Need to do this template shenanigans to prevent any T* -> bool implicit conversions (C++ wtf)
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, bool>>>
    explicit Value(T boolean) : value(boolean? TRUE_VAL : FALSE_VAL) {}

    explicit Value(double number) {
        memcpy(&value, &number, sizeof(Value));
    }
    explicit Value(Obj* obj) {
        value = SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj);
    }

    bool is_bool() const { return (value | 1) == TRUE_VAL; }
    bool is_nil() const { return value == NIL_VAL; }
    bool is_number() const { return (value & QNAN) != QNAN; }
    bool is_obj() const { return (value & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT); }
    bool is_obj_type(ObjType type) const { return is_obj() && as_obj()->type == type; }

    bool as_bool() const { return value == TRUE_VAL; }
    double as_number() const {
        double num;
        memcpy(&num, &value, sizeof(Value));
        return num;
    }
    Obj* as_obj() const { return (Obj*)(uintptr_t)(value & ~(SIGN_BIT | QNAN)); }

#else
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj* obj;
    } as;

    Value() : type(VAL_NIL) {}

    // Need to do this template shenanigans to prevent any T* -> bool implicit conversions (C++ wtf)
    template <typename T, typename = std::enable_if_t<std::is_same_v<T, bool>>>
    explicit Value(T boolean) : type(VAL_BOOL) { as.boolean = boolean; }

    explicit Value(double number) : type(VAL_NUMBER) { as.number = number; }
    explicit Value(Obj* obj) : type(VAL_OBJ) { as.obj = obj; }

    bool is_bool() const { return type == VAL_BOOL; }
    bool is_nil() const { return type == VAL_NIL; }
    bool is_number() const { return type == VAL_NUMBER; }
    bool is_obj() const { return type == VAL_OBJ; }
    bool is_obj_type(ObjType type) const { return is_obj() && as_obj()->type == type; }

    bool as_bool() const { return as.boolean; }
    double as_number() const { return as.number; }
    Obj* as_obj() const { return as.obj; }

#endif

    explicit Value(ObjString* obj) : Value(reinterpret_cast<Obj*>(obj)) {}
    explicit Value(ObjArray* obj) : Value(reinterpret_cast<Obj*>(obj)) {}
    explicit Value(ObjTable* obj) : Value(reinterpret_cast<Obj*>(obj)) {}
    explicit Value(ObjFunction* obj) : Value(reinterpret_cast<Obj*>(obj)) {}
    explicit Value(ObjClosure* obj) : Value(reinterpret_cast<Obj*>(obj)) {}
    explicit Value(ObjNativeFun* obj) : Value(reinterpret_cast<Obj*>(obj)) {}
    explicit Value(ObjClass* obj) : Value(reinterpret_cast<Obj*>(obj)) {}
    explicit Value(ObjInstance* obj) : Value(reinterpret_cast<Obj*>(obj)) {}
    explicit Value(ObjBoundMethod* obj) : Value(reinterpret_cast<Obj*>(obj)) {}

    bool is_string() const { return is_obj_type(OBJ_STRING); }
    bool is_array() const { return is_obj_type(OBJ_ARRAY); }
    bool is_table() const { return is_obj_type(OBJ_TABLE); }
    bool is_function() const { return is_obj_type(OBJ_FUNCTION); }
    bool is_closure() const { return is_obj_type(OBJ_CLOSURE); }
    bool is_nativefun() const { return is_obj_type(OBJ_NATIVEFUN); }
    bool is_class() const { return is_obj_type(OBJ_CLASS); }
    bool is_instance() const { return is_obj_type(OBJ_INSTANCE); }
    bool is_bound_method() const { return is_obj_type(OBJ_BOUND_METHOD); }

    ObjString* as_string() const { return reinterpret_cast<ObjString*>(as_obj()); }
    ObjArray* as_array() const { return reinterpret_cast<ObjArray*>(as_obj()); }
    ObjTable* as_table() const { return reinterpret_cast<ObjTable*>(as_obj()); }
    ObjFunction* as_function() const { return reinterpret_cast<ObjFunction*>(as_obj()); }
    ObjClosure* as_closure() const { return reinterpret_cast<ObjClosure*>(as_obj()); }
    ObjNativeFun* as_nativefun() const { return reinterpret_cast<ObjNativeFun*>(as_obj()); }
    ObjClass* as_class() const { return reinterpret_cast<ObjClass*>(as_obj()); }
    ObjInstance* as_instance() const { return reinterpret_cast<ObjInstance*>(as_obj()); }
    ObjBoundMethod* as_bound_method() const { return reinterpret_cast<ObjBoundMethod*>(as_obj()); }

    ObjType obj_type() const { return as_obj()->type; }

    bool is_falsey() const {
        return is_nil() || (is_bool() && !as_bool());
    }

    void obj_incref() {
        as_obj()->refcount++;
    }

    void obj_decref() {
        Obj* obj = as_obj();
        obj->refcount--;
        if (obj->refcount == 0) {
            obj_free();
            // After this, the Value is in an invalid state, don't use it!
        }
    }

    void obj_free();

    uint32_t hash() const;

    static bool equals(const Value& a, const Value& b);

    std::string to_std_string(bool print_refcount = false) const;

};
