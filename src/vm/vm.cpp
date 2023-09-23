#include "vm/vm.h"

#include "core/file.h"
#include "vm/string.h"
#include "vm/array.h"
#include "vm/table.h"

#include "fmt/args.h"

VM::VM() {
    m_stack_top = m_stack.data();
    m_string_interner.init();
    m_globals.init();

    init_builtin_functions();
}

VM::~VM() {
    m_string_interner.free();
    m_globals.clear();
}

void VM::repl() {
    Array<char, 1024> line;
    for (;;) {
        fmt::print("> ");
        if (fgets(line.data(), sizeof(line), stdin) == nullptr) {
            fmt::print("\n");
            break;
        }
        line[strcspn(line.data(), "\r\n")] = 0; // Removing trailing newline
        InterpretResult res = interpret(line.data());
    }
}

void VM::run_file(const char *path) {
    Vector<char> buf;
    if (!read_file_to_buf(path, buf)) {
        fmt::print(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    InterpretResult result = interpret(buf.data());
    buf.clear();
    if (result == InterpretResult::CompileError) exit(65);
    if (result == InterpretResult::RuntimeError) exit(70);
}

InterpretResult VM::interpret(const char *source) {
    ObjFunction* fn = compile(source);
    if (fn == nullptr) {
        return InterpretResult::CompileError;
    }

    ObjClosure* closure = create_obj_closure(fn);
    push(Value(closure));
    call(closure, 0);

    return run();
    // return InterpretResult::Ok;
}

ObjFunction* VM::compile(const char *source) {
    Parser parser;
    parser.init(source);
    Compiler compiler(&parser, &m_string_interner);
    compiler.init_script();
    compiler.reset_errors();
    return compiler.compile();
}

void VM::define_native(const char *name, NativeFun function) {
    auto name_str = m_string_interner.create_string(name, (int32_t)strlen(name));
    Value key = Value(name_str);
    Value value = Value(create_obj_native_fun(function));
    m_globals.set(key, value);
}

void VM::init_builtin_functions() {
    define_native("clock", [](int32_t arg_count, Value* args) {
        return Value((double)clock() / CLOCKS_PER_SEC);
    });
    define_native("print", [](int32_t arg_count, Value* args) {
        if (arg_count == 0) return Value();
        if (args[0].is_string()) {
            auto fmt_str = args[0].as_string();
            if (arg_count == 1) {
                puts(fmt_str->chars);
                putc('\n', stdout);
            } else {
                auto store = fmt::dynamic_format_arg_store<fmt::format_context>();
                for (int32_t i = 1; i < arg_count; i++) {
                    store.push_back(args[i].to_std_string());
                }
                fmt::vprint(fmt_str->chars, store);
                putc('\n', stdout);
            }
        }
        else {
            auto str = args[0].to_std_string();
            puts(str.c_str());
        }
        return Value();
    });
}

InterpretResult VM::run() {
    CallFrame* frame = &m_frames[m_frame_count - 1];
#define READ_BYTE() (*frame->ip++)
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
#define READ_CONSTANT() (frame->closure->function->chunk.m_constants[READ_BYTE()])
#define READ_STRING() READ_CONSTANT().as_string()
#define BINARY_OP(op) \
    do {              \
        if (!peek(0).is_number() || !peek(1).is_number()) { \
            runtime_error("Operands must be numbers."); \
            return InterpretResult::RuntimeError; \
        } \
        double b = pop().as_number(); \
        double a = pop().as_number(); \
        push(Value(a op b)); \
    } while (false);

#ifdef DEBUG_TRACE_EXECUTION
    fmt::print("---- Debug Trace ----\n");
#endif

    for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
        fmt::print("          ");
        for (Value* slot = m_stack.data(); slot < m_stack_top; slot++) {
            fmt::print("[ ");
            std::string str = slot->to_std_string();
            fputs(str.c_str(), stdout);
            fmt::print(" ]");
        }
        fmt::print("\n");
        frame->closure->function->chunk.disassemble_instruction(
            (int32_t)(frame->ip - frame->closure->function->chunk.m_code.data()));
#endif
        uint8_t inst = READ_BYTE();
        switch (inst) {
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(Value()); break;
            case OP_TRUE: push(Value(true)); break;
            case OP_FALSE: push(Value(false)); break;
            case OP_POP: pop(); break;
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                auto& val = frame->slots[slot];
                if (val.is_obj()) val.obj_decref();
                val = peek(0);
                if (val.is_obj()) val.obj_incref();
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                auto val = frame->slots[slot];
                push(val);
                if (val.is_obj()) val.obj_incref();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                // TODO: create specialized table_string_get() for optimization
                if (!m_globals.get(Value(name), &value)) {
                    runtime_error("Undefined variable '{}'.", name->chars);
                    return InterpretResult::RuntimeError;
                }
                push(value);
                if (value.is_obj()) value.obj_incref();
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                Value a = pop();
                // TODO: create specialized table_string_set() for optimization
                m_globals.set(Value(name), a);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                // TODO: create specialized table_string_set() for optimization
                Value name_value = Value(name);
                if (m_globals.set(name_value, peek(0))) {
                    m_globals.remove(name_value);
                    runtime_error("Undefined variable '{}'.", name->chars);
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame->closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame->closure->upvalues[slot]->location = peek(0);
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(Value(Value::equals(a, b)));
                if (a.is_obj()) a.obj_decref();
                if (b.is_obj()) b.obj_decref();
                break;
            }
            case OP_NOT_EQUAL: {
                Value b = pop();
                Value a = pop();
                push(Value(Value::not_equals(a, b)));
                if (a.is_obj()) a.obj_decref();
                if (b.is_obj()) b.obj_decref();
                break;
            }
            case OP_GREATER: BINARY_OP(>); break;
            case OP_GREATER_EQUAL: BINARY_OP(>=); break;
            case OP_LESS: BINARY_OP(<) break;
            case OP_LESS_EQUAL: BINARY_OP(<=) break;
            case OP_ADD: {
                if (peek(0).is_string() && peek(1).is_string()) {
                    Value b = pop();
                    Value a = pop();
                    ObjString* str = concat_string(a.as_string(), b.as_string());
                    ObjString* actual_str = m_string_interner.create_string(str->chars, str->length, str->hash);
                    if (actual_str != str) free(str);
                    Value result = Value(actual_str);
                    result.obj_incref();
                    push(result);
                    a.obj_decref();
                    b.obj_decref();
                }
                else if (peek(0).is_number() && peek(1).is_number()) {
                    double b = pop().as_number();
                    double a = pop().as_number();
                    push(Value(a + b));
                }
                else {
                    runtime_error("Operands must be two numbers or two strings.");
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OP_SUBTRACT: BINARY_OP(-); break;
            case OP_MULTIPLY: BINARY_OP(*); break;
            case OP_DIVIDE: BINARY_OP(/); break;
            case OP_NOT: {
                push(Value(pop().is_falsey()));
                break;
            }
            case OP_NEGATE: {
                if (!peek(0).is_number()) {
                    runtime_error("Operand must be a number.");
                    return InterpretResult::RuntimeError;
                }
                push(Value(-pop().as_number()));
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (peek(0).is_falsey()) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            case OP_CALL: {
                int32_t arg_count = READ_BYTE();
                Value fn_value = peek(arg_count);
                if (!call_value(fn_value, arg_count)) {
                    return InterpretResult::RuntimeError;
                }
                frame = &m_frames[m_frame_count - 1];
                break;
            }
            case OP_CLOSURE: {
                ObjFunction* function = READ_CONSTANT().as_function();
                ObjClosure* closure = create_obj_closure(function);
                push(Value(closure));
                for (int32_t i = 0; i < closure->upvalue_count; i++) {
                    uint8_t is_local = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (is_local) {
                        closure->upvalues[i] = capture_upvalue(frame->slots + index);
                    }
                    else {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }
                }
                break;
            }
            case OP_RETURN: {
                Value result = pop();
                m_frame_count--;
                if (m_frame_count == 0) {
                    pop();
                    return InterpretResult::Ok;
                }

                m_stack_top = frame->slots;
                push(result);
                frame = &m_frames[m_frame_count - 1];
                break;
            }
            case OP_TABLE_NEW: {
                ObjTable* table = create_obj_table();
                Value value = Value(table);
                value.obj_incref();
                push(value);
                break;
            }
            case OP_ARRAY_NEW: {
                uint16_t size = READ_SHORT();
                ObjArray* array = create_obj_array();
                array->resize(size);
                Value value = Value(array);
                value.obj_incref();
                push(value);
                break;
            }
            case OP_GET: {
                Value key = pop();
                Value obj = pop();
                Value value;
                if (!get(obj, key, &value)) {
                    obj.obj_decref();
                    return InterpretResult::RuntimeError;
                }
                obj.obj_decref();
                break;
            }
            case OP_SET: {
                Value value = pop();
                Value key = pop();
                Value obj = pop();
                if (!set(obj, key, value)) {
                    obj.obj_decref();
                    return InterpretResult::RuntimeError;
                }
                push(value);
                if (value.is_obj()) value.obj_incref();
                break;
            }
            case OP_GET_NOPOP: {
                Value key = pop();
                Value obj = peek(0);
                Value value;
                if (!get(obj, key, &value)) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }
            case OP_SET_NOPOP: {
                Value value = pop();
                Value key = pop();
                Value obj = peek(0);
                if (!set(obj, key, value)) {
                    return InterpretResult::RuntimeError;
                }
                break;
            }
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}

bool VM::call_value(Value callee, int32_t arg_count) {
    if (callee.is_obj()) {
        switch (callee.obj_type()) {
            case OBJ_CLOSURE:
                return call(callee.as_closure(), arg_count);
            case OBJ_NATIVEFUN: {
                auto native_fn = callee.as_nativefun()->function;
                Value result = native_fn(arg_count, m_stack_top - arg_count);
                m_stack_top -= arg_count + 1;
                push(result);
                return true;
            }
            default:
                break;
        }
    }
    runtime_error("Can only call functions and classes.");
    return false;
}

bool VM::call(ObjClosure* closure, int32_t arg_count) {
    if (arg_count != closure->function->arity) {
        runtime_error("Expected {} arguments but got {}.", closure->function->arity, arg_count);
    }
    if (m_frame_count == MaxFrameSize) {
        runtime_error("Stack overflow.");
        return false;
    }
    CallFrame* frame = &m_frames[m_frame_count++];
    frame->closure = closure;
    frame->ip = closure->function->chunk.m_code.data();
    frame->slots = m_stack_top - arg_count - 1;
    return true;
}

ObjUpvalue *VM::capture_upvalue(Value *local) {
    ObjUpvalue* created_upvalue = create_obj_upvalue(local);
    return created_upvalue;
}

bool VM::get(Value obj, Value key, Value* value) {
    if (!obj.is_obj()) {
        runtime_error("Cannot get field on a non-object type.");
        return false;
    }
    if (obj.as_obj()->type == OBJ_ARRAY) {
        if (!key.is_number()) {
            runtime_error("Array index must be a number.");
            if (key.is_obj()) key.obj_decref();
            return false;
        }
        int32_t index = (int32_t)key.as_number();
        ObjArray* array = obj.as_array();
        bool found = array->get(index, value);
        if (!found) {
            runtime_error("Cannot subscript array of count {} with index {}.", array->count, index);
            return false;
        }
        push(*value);
        if (value->is_obj()) value->obj_incref();
    }
    else if (obj.as_obj()->type == OBJ_TABLE) {
        bool found = obj.as_table()->get(key, value);
        if (!found) {
            runtime_error("Cannot find key {} in table.", key.to_std_string());
            return false;
        }
        push(*value);
        if (value->is_obj()) value->obj_incref();
        if (key.is_obj()) key.obj_decref();
    }
    return true;
}

bool VM::set(Value obj, Value key, Value value) {
    if (!obj.is_obj()) {
        runtime_error("Cannot set field on a non-object type.");
        return false;
    }
    if (obj.as_obj()->type == OBJ_ARRAY) {
        if (!key.is_number()) {
            runtime_error("Array index must be a number.");
            if (key.is_obj()) key.obj_decref();
            return false;
        }
        int32_t index = (int32_t)key.as_number();
        ObjArray* array = obj.as_array();
        bool success = array->set(index, value);
        if (!success) {
            runtime_error("Cannot subscript array of count {} with index {}.", array->count, index);
            return false;
        }
    }
    else if (obj.as_obj()->type == OBJ_TABLE) {
        obj.as_table()->set(key, value);
    }
    return true;
}

