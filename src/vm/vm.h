#include "vm/compiler.h"
#include "vm/value.h"
#include "vm/string.h"
#include "vm/string_interner.h"

struct CallFrame {
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
};

class VM {
public:
    VM();
    ~VM();

    void repl();

    void run_file(const char* path);

    InterpretResult interpret(const char* source);

    ObjFunction* compile(const char* source);

    void define_native(const char* name, NativeFun function);

private:
    void init_builtin_functions();

    InterpretResult run();

    void reset_stack() {
        m_stack_top = m_stack.data();
        m_frame_count = 0;
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

    bool call_value(Value callee, int32_t arg_count);
    bool call(ObjClosure* closure, int32_t arg_count);
    bool invoke(ObjString* name, int32_t arg_count);
    bool invoke_from_class(ObjClass* klass, ObjString* name, int32_t arg_count);

    ObjUpvalue* capture_upvalue(Value* local);
    void close_upvalues(Value* last);

    bool get(Value obj, Value key, Value* value);
    bool set(Value obj, Value key, Value value);

    void define_method(ObjString* name);
    bool bind_method(ObjClass* klass, ObjString* name);

    template <typename ...Args>
    inline void runtime_error(const char* fmt, Args&&... args) {
        fmt::print(fmt, std::forward<Args>(args)...);
        fputs("\n", stderr);

        for (int32_t i = m_frame_count - 1; i >= 0; i--) {
            CallFrame* frame = &m_frames[i];
            ObjFunction* function = frame->closure->function;
            size_t instruction = frame->ip - function->chunk.m_code.data() - 1;
            fmt::print(stderr, "[line {}] in ",
                       function->chunk.m_lines[instruction]);
            if (function->name == nullptr) {
                fmt::print(stderr, "script\n");
            }
            else {
                fmt::print(stderr, "{}()\n", function->name->chars);
            }
        }

        auto& frame = m_frames[m_frame_count - 1];
        size_t instr = frame.ip - frame.closure->function->chunk.m_code.data() - 1;
        int line = frame.closure->function->chunk.m_lines[instr];
        fmt::print(stderr, "[line {}] in script\n", line);
        reset_stack();
    }

    static constexpr int32_t MaxFrameSize = 64;
    static constexpr int32_t MaxStackSize = MaxFrameSize * UINT8_COUNT;

    Array<CallFrame, MaxFrameSize> m_frames;
    int32_t m_frame_count = 0;

    Array<Value, MaxStackSize> m_stack;
    Value* m_stack_top = nullptr;

    StringInterner m_string_interner;
    ObjTable m_globals;

    ObjUpvalue* m_open_upvalues = nullptr;

    ObjString* m_init_string;
};
