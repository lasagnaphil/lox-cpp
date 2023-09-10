#include "vm/function.h"

ObjFunction *create_obj_function() {
    void* raw_data = malloc(sizeof(ObjFunction));
    new (raw_data) ObjFunction();
    ObjFunction* fn = static_cast<ObjFunction*>(raw_data);
    fn->init();
    return fn;
}

void free_obj_function(ObjFunction *fn) {
    fn->clear();
    fn->~ObjFunction();
    free(fn);
}

void ObjFunction::init() {
    arity = 0;
    name = nullptr;
}

void ObjFunction::clear() {
    free(name);
}
