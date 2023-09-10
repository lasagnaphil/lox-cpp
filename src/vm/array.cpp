#include "vm/array.h"

ObjArray *create_obj_array() {
    void* raw_data = malloc(sizeof(ObjArray));
    new (raw_data) ObjArray();
    ObjArray* array = static_cast<ObjArray*>(raw_data);
    array->init();
    return array;
}

void free_obj_array(ObjArray *array) {
    array->clear();
    array->~ObjArray();
    free(array);
}

static int32_t grow_capacity(int32_t capacity) {
    return capacity < 4? 4 : capacity * 2;
}

void ObjArray::init() {
    count = 0;
    capacity = 0;
    values = nullptr;
}

void ObjArray::clear() {
    for (int32_t i = 0; i < capacity; i++) {
        Value* value = &values[i];
        if (value->is_obj())  value->obj_decref();
    }
    free(values);
    init();
}

bool ObjArray::get(int32_t index, Value *value) const {
    if (index < 0) index += count;
    if (index >= count) return false;
    *value = values[index];
    return true;
}

bool ObjArray::set(int32_t index, Value value) {
    if (index < 0) index += count;
    if (index >= count) return false;
    values[index] = value;
    return true;
}

static void adjust_capacity(ObjArray* array, int32_t capacity) {
    Value* values = static_cast<Value*>(malloc(sizeof(Value) * capacity));
    for (int32_t i = 0; i < capacity; i++) {
        values[i] = Value();
    }

    for (int32_t i = 0; i < array->count; i++) {
        values[i] = array->values[i];
    }

    free(array->values);
    array->values = values;
    array->capacity = capacity;
}

void ObjArray::resize(int32_t count_) {
    if (count >= capacity) {
        adjust_capacity(this, count_);
    }
    count = count_;
}

void ObjArray::push(Value value) {
    if (count >= capacity) {
        int32_t new_capacity = grow_capacity(capacity);
        adjust_capacity(this, new_capacity);
    }
    values[count++] = value;
}

bool ObjArray::pop(Value* value) {
    if (count == 0) return false;
    *value = values[--count];
    return true;
}
