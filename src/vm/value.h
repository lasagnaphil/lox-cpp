#pragma once

enum ValueType {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER
};

struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
    } as;

    Value() : type(VAL_NIL) {}
    Value(bool boolean) : type(VAL_BOOL) { as.boolean = boolean; }
    Value(double number) : type(VAL_NUMBER) { as.number = number; }

    bool is_bool() const { return type == VAL_BOOL; }
    bool is_nil() const { return type == VAL_NIL; }
    bool is_number() const { return type == VAL_NUMBER; }

    bool as_bool() const { return as.boolean; }
    double as_number() const { return as.number; }

    bool is_falsey() const {
        return is_nil() || (is_bool() && !as_bool());
    }

    static bool equals(Value a, Value b) {
        if (a.type != b.type) return false;
        switch (a.type) {
            case VAL_BOOL:   return a.as_bool() == b.as_bool();
            case VAL_NIL:    return true;
            case VAL_NUMBER: return a.as_number() == b.as_number();
            default:         return false;
        }
    }

    static bool not_equals(Value a, Value b) {
        if (a.type == b.type) return true;
        switch (a.type) {
            case VAL_BOOL:   return a.as_bool() != b.as_bool();
            case VAL_NIL:    return false;
            case VAL_NUMBER: return a.as_number() != b.as_number();
            default:         return false;
        }
    }
};

