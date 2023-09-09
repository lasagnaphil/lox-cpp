#include "vm/vm.h"

VM::VM() : m_compiler(&m_scanner, &m_string_interner) {
    m_stack_top = m_stack.data();
    m_string_interner.init();
    init_table(&m_globals);
}

VM::~VM() {
    m_string_interner.free();
    free_table(&m_globals);
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
            print_value(*slot);
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
                m_stack[slot] = peek(0);
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                push(m_stack[slot]);
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value;
                // TODO: create specialized table_string_get() for optimization
                if (!table_get(&m_globals, Value(name), &value)) {
                    runtime_error("Undefined variable '{}'.", name->chars);
                    return InterpretResult::RuntimeError;
                }
                push(value);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ObjString *name = READ_STRING();
                Value a = pop();
                // TODO: create specialized table_string_set() for optimization
                table_set(&m_globals, Value(name), a);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                // TODO: create specialized table_string_set() for optimization
                Value name_value = Value(name);
                if (table_set(&m_globals, name_value, peek(0))) {
                    table_delete(&m_globals, name_value);
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
                print_value(pop());
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
        }
    }

#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef BINARY_OP
}
