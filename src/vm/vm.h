#include "vm/compiler.h"
#include "vm/value.h"
#include "vm/string.h"
#include "vm/string_interner.h"


class VM {
public:
    VM();
    ~VM();

    void repl();

    void run_file(const char* path);

    InterpretResult interpret(const char* source);

    bool compile(const char* source, Chunk* chunk);

private:
    InterpretResult run();

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

    Value peek(int distance) {
        return m_stack_top[-1 - distance];
    }

    template <typename ...Args>
    inline void runtime_error(const char* fmt, Args&&... args) {
        fmt::print(fmt, std::forward<Args...>(args)...);
        fputs("\n", stderr);

        size_t instr = m_ip - m_chunk->m_code.data() - 1;
        int line = m_chunk->m_lines[instr];
        fmt::print(stderr, "[line {}] in script\n", line);
        reset_stack();
    }

    static constexpr int32_t MaxStackSize = 256;

    Scanner m_scanner;
    Compiler m_compiler;
    Chunk* m_chunk = nullptr;
    const uint8_t* m_ip = nullptr;
    Array<Value, MaxStackSize> m_stack;
    Value* m_stack_top = nullptr;
    StringInterner m_string_interner;
    ObjTable m_globals;
};
