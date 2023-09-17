#include "vm/closure.h"

ObjClosure *create_obj_closure() {
    void* raw_data = malloc(sizeof(ObjClosure));
    ObjClosure* closure = static_cast<ObjClosure*>(raw_data);
    new (closure) ObjClosure();
    return closure;
}

void free_obj_closure(ObjClosure *closure) {
    closure->~ObjClosure();
    free(closure);
}
