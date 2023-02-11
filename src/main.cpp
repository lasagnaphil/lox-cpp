#include "core/vector.h"
#include "core/array.h"
#include "core/log.h"

#include <fmt/core.h>

#define DEBUG_TRACE_EXECUTION

#define OPCODE_LIST(X) \
    X(OP_CONSTANT)     \
    X(OP_RETURN)       \
    X(OP_INVALID)


enum OpCode : uint8_t {
#define X(val) val,
    OPCODE_LIST(X)
#undef X
};

const char* g_opcode_str[] = {
#define X(val) #val,
        OPCODE_LIST(X)
#undef X
};

using Value = double;

constexpr int32_t g_num_opcodes = sizeof(g_opcode_str) / sizeof(const char*);

class Chunk {
public:
    Vector<uint8_t> m_code;
    Vector<int32_t> m_lines;
    Vector<Value> m_constants;

    void write(OpCode opcode, int32_t line, std::initializer_list<uint8_t> bytes) {
        write(opcode, line);
        for (uint8_t byte : bytes) {
            write(byte, line);
        }
    }
    void write(uint8_t byte, int32_t line) {
        m_code.push_back(byte);
        m_lines.push_back(line);
    }

    int32_t add_constant(Value value) {
        m_constants.push_back(value);
        return m_constants.ssize() - 1;
    }

    void print_disassembly(const char* name) const {
        fmt::print("== {} ==\n", name);
        for (int32_t offset = 0; offset < m_code.ssize(); ) {
            offset = disassemble_instruction(offset);
        }
    }

    int32_t disassemble_instruction(int32_t offset) const {
        fmt::print("{:04d} ", offset);
        if (offset > 0 && m_lines[offset] == m_lines[offset - 1]) {
            fmt::print("  | ");
        }
        else {
            fmt::print("{:4d} ", m_lines[offset]);
        }
        uint8_t instr = m_code[offset];
        switch (instr) {
            case OP_CONSTANT: {
                offset = print_constant_instruction((OpCode)instr, offset);
            } break;
            case OP_RETURN: {
                offset = print_simple_instruction((OpCode)instr, offset);
            } break;
            default: {
                offset = print_simple_instruction((OpCode)instr, offset);
            }
        }
        return offset;
    }

private:
    int32_t print_simple_instruction(OpCode opcode, int32_t offset) const {
        log_assert(opcode < g_num_opcodes);
        fmt::print("{}\n", g_opcode_str[opcode]);
        return offset + 1;
    }

    int32_t print_constant_instruction(OpCode opcode, int32_t offset) const {
        log_assert(opcode < g_num_opcodes);
        uint8_t constant_loc = m_code[offset + 1];
        fmt::print("{:16s} {:4d} '{:g}'\n", g_opcode_str[opcode], constant_loc, m_constants[constant_loc]);
        return offset + 2;
    }
};

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

class VM {
public:
    InterpretResult interpret(const Chunk* chunk) {
        m_chunk = chunk;
        m_ip = chunk->m_code.data();
        return run();
    }

private:
    InterpretResult run() {
#define READ_BYTE() (*m_ip++)
#define READ_CONSTANT() (m_chunk->m_constants[READ_BYTE()])

        for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
            m_chunk->disassemble_instruction((int32_t)(m_ip - m_chunk->m_code.data()));
#endif
            uint8_t inst = READ_BYTE();
            switch (inst) {
                case OP_CONSTANT: {
                    Value constant = READ_CONSTANT();
                    fmt::print("{:g}\n", constant);
                }
                case OP_RETURN: {
                    return InterpretResult::Ok;
                }
            }
        }

#undef READ_BYTE
    }

    static constexpr int32_t MaxStackSize = 256;

    const Chunk* m_chunk;
    const uint8_t* m_ip;
    Array<Value, MaxStackSize> m_stack;
};

int main() {
    Chunk chunk;
    int32_t const_loc = chunk.add_constant(3.14);
    chunk.write(OP_CONSTANT, 0, {(uint8_t)const_loc});
    chunk.write(OP_RETURN, 1);
    chunk.print_disassembly("Test");

    VM vm;
    InterpretResult res = vm.interpret(&chunk);

    return 0;
}
