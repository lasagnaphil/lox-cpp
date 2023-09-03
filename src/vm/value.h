#pragma once

#include <cstdint>
#include <cstdlib>
#include <type_traits>

enum ValueType {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    VAL_OBJ
};

enum ObjType {
    OBJ_STRING,
    OBJ_TABLE
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
    ObjType type : 4;
    uint32_t uid : 28; // TODO: make this 64-bit?
    uint32_t refcount; // TODO: make this atomic

    Obj(ObjType type_) : type(type_), uid(gen_random_uid()), refcount(1) {}
};

struct ObjString;
struct ObjMap;

struct Value {
    ValueType type : 4;
    uint32_t uid : 28; // TODO: make this 64-bit?
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
    explicit Value(ObjString* str) : type(VAL_OBJ) { as.obj = reinterpret_cast<Obj*>(str); }
    explicit Value(ObjMap* str) : type(VAL_OBJ) { as.obj = reinterpret_cast<Obj*>(str); }

    bool is_bool() const { return type == VAL_BOOL; }
    bool is_nil() const { return type == VAL_NIL; }
    bool is_number() const { return type == VAL_NUMBER; }
    bool is_obj() const { return type == VAL_OBJ; }
    bool is_obj_type(ObjType type) const { return is_obj() && as_obj()->type == type; }
    bool is_string() const { return is_obj_type(OBJ_STRING); }

    bool as_bool() const { return as.boolean; }
    double as_number() const { return as.number; }
    Obj* as_obj() const { return as.obj; }
    ObjString* as_string() const { return reinterpret_cast<ObjString*>(as.obj); }

    bool is_falsey() const {
        return is_nil() || (is_bool() && !as_bool());
    }

    void obj_incref() {
        as.obj->refcount++;
    }

    void obj_decref() {
        as.obj->refcount--;
        if (as.obj->refcount == 0) {
            free(as.obj);
            // After this, the Value is in an invalid state, don't use it!
        }
    }

    static bool equals(const Value& a, const Value& b);

    static bool not_equals(const Value& a, const Value& b);
};
