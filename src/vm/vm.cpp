#include "vm/vm.h"

#include "vm/string.h"
#include "vm/array.h"
#include "vm/table.h"

VM::VM() : m_compiler(&m_scanner, &m_string_interner) {
    m_stack_top = m_stack.data();
    m_string_interner.init();
    m_globals.init();
}

VM::~VM() {
    m_string_interner.free();
    m_globals.clear();
}

inline bool read_file_to_buf(const char* path, Vector<char>& buf) {
    FILE* file;
    if (fopen_s(&file, path, "rb") != 0) {
        return false;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    buf.resize(file_size + 1);
    size_t bytes_read = fread(buf.data(), sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        return false;
    }

    fclose(file);
    return true;
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
        fmt::print(stderr, "Cloud not open file \"%s\".\n", path);
        exit(74);
    }
    InterpretResult result = interpret(buf.data());
    buf.clear();
    if (result == InterpretResult::CompileError) exit(65);
    if (result == InterpretResult::RuntimeError) exit(70);
}

InterpretResult VM::interpret(const char *source) {
    Chunk chunk;
    if (!compile(source, &chunk)) {
        return InterpretResult::CompileError;
    }

    m_chunk = &chunk;
    m_ip = m_chunk->m_code.data();

    InterpretResult result = run();

    m_chunk = nullptr;
    m_ip = nullptr;

    return result;
}

bool VM::compile(const char *source, Chunk *chunk) {
    m_scanner.init(source);
    m_compiler.reset_errors();
    m_compiler.set_compiling_chunk(chunk);
    m_compiler.compile();
    return !m_compiler.had_error();
}

InterpretResult VM::run() {
#define READ_BYTE() (*m_ip++)
#define READ_SHORT() (m_ip += 2, (uint16_t)((m_ip[-2] << 8) | m_ip[-1]))
#define READ_CONSTANT() (m_chunk->m_constants[READ_BYTE()])
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
        m_chunk->disassemble_instruction((int32_t)(m_ip - m_chunk->m_code.data()));
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
                auto& val = m_stack[slot];
                if (val.is_obj()) val.obj_decref();
                val = peek(0);
                if (val.is_obj()) val.obj_incref();
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                auto val = m_stack[slot];
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
            } break;
            case OP_PRINT: {
                Value value = pop();
                std::string str = value.to_std_string();
                fputs(str.c_str(), stdout);
                if (value.is_obj()) value.obj_decref();
                fmt::print("\n");
                break;
            }
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                m_ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t offset = READ_SHORT();
                if (peek(0).is_falsey()) m_ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                m_ip -= offset;
                break;
            }
            case OP_RETURN: {
                // print_value(pop());
                // fmt::print("\n");
                return InterpretResult::Ok;
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
