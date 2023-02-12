#include "vm/compiler.h"

class VM {
public:
    VM() {
        m_stack_top = m_stack.data();
    }

    void repl() {
        Array<char, 1024> line;
        for (;;) {
            fmt::print("> ");
            if (!fgets(line.data(), sizeof(line), stdin)) {
                fmt::print("\n");
                break;
            }
            interpret(line.data());
        }
    }

    void run_file(const char* filename) {

    }

    InterpretResult interpret(const char* source) {
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

    bool compile(const char* source, Chunk* chunk) {
        m_scanner.init(source);
        m_compiler.set_scanner(&m_scanner);
        m_compiler.set_compiling_chunk(chunk);
        m_compiler.advance();
        m_compiler.expression();
        m_compiler.consume(TOKEN_EOF, "Expect end of expression.");
        m_compiler.end();
        return !m_compiler.had_error();
    }

private:
    InterpretResult run() {
#define READ_BYTE() (*m_ip++)
#define READ_CONSTANT() (m_chunk->m_constants[READ_BYTE()])
#define BINARY_OP(op) \
    do { \
        double b = pop(); \
        double a = pop(); \
        push(a op b); \
    } while (false)

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
                case OP_ADD: BINARY_OP(+); break;
                case OP_SUBTRACT: BINARY_OP(-); break;
                case OP_MULTIPLY: BINARY_OP(*); break;
                case OP_DIVIDE: BINARY_OP(/); break;
                case OP_NEGATE: {
                    push(-pop());
                } break;
                case OP_RETURN: {
                    print_value(pop());
                    fmt::print("\n");
                    return InterpretResult::Ok;
                }
            }
        }

#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_OP
    }

    void reset_stack() {
        m_stack_top = m_stack.data();
    }

    void push(Value value) {
        *m_stack_top = value;
        m_stack_top++;
    }

    Value pop() {
        m_stack_top--;
        return *m_stack_top;
    }

    static constexpr int32_t MaxStackSize = 256;

    Scanner m_scanner;
    Compiler m_compiler;
    Chunk* m_chunk = nullptr;
    const uint8_t* m_ip = nullptr;
    Array<Value, MaxStackSize> m_stack;
    Value* m_stack_top = nullptr;
};
