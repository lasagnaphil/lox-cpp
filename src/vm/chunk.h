#pragma once

#include "core/vector.h"
#include "core/log.h"
#include "vm/value.h"
#include <cassert>

#define OPCODE_LIST(X)    \
    X(OP_CONSTANT)        \
    X(OP_NIL)             \
    X(OP_TRUE)            \
    X(OP_FALSE)           \
    X(OP_EQUAL)           \
    X(OP_NOT_EQUAL)       \
    X(OP_GREATER)         \
    X(OP_GREATER_EQUAL)   \
    X(OP_LESS)            \
    X(OP_LESS_EQUAL)      \
    X(OP_ADD)             \
    X(OP_SUBTRACT)        \
    X(OP_MULTIPLY)        \
    X(OP_DIVIDE)          \
    X(OP_NOT)             \
    X(OP_NEGATE)          \
    X(OP_RETURN)          \
    X(OP_INVALID)

enum OpCode : uint8_t {
#define X(val) val,
    OPCODE_LIST(X)
#undef X
    OP_COUNT
};

extern const char* g_opcode_str[OP_COUNT];

inline void print_value(Value value) {
    switch (value.type) {
        case VAL_BOOL: fmt::print(value.as_bool()? "true": "false"); break;
        case VAL_NIL: fmt::print("nil"); break;
        case VAL_NUMBER: fmt::print("{:g}", value.as_number()); break;
    }
}

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
            fmt::print("   | ");
        }
        else {
            fmt::print("{:4d} ", m_lines[offset]);
        }
        uint8_t instr = m_code[offset];
        switch (instr) {
            case OP_CONSTANT:
                return print_constant_instruction((OpCode)instr, offset);
            case OP_NIL:
            case OP_TRUE:
            case OP_FALSE:
            case OP_EQUAL:
            case OP_NOT_EQUAL:
            case OP_GREATER:
            case OP_GREATER_EQUAL:
            case OP_LESS:
            case OP_LESS_EQUAL:
            case OP_ADD:
            case OP_SUBTRACT:
            case OP_MULTIPLY:
            case OP_DIVIDE:
            case OP_NEGATE:
            case OP_RETURN:
                return print_simple_instruction((OpCode)instr, offset);
            default:
                return print_simple_instruction(OP_INVALID, offset);
        }
    }

private:
    int32_t print_simple_instruction(OpCode opcode, int32_t offset) const {
        assert(opcode < OP_COUNT);
        fmt::print("{}\n", g_opcode_str[opcode]);
        return offset + 1;
    }

    int32_t print_constant_instruction(OpCode opcode, int32_t offset) const {
        assert(opcode < OP_COUNT);
        uint8_t constant_loc = m_code[offset + 1];
        fmt::print("{:16s} {:4d} '", g_opcode_str[opcode], constant_loc);
        print_value(m_constants[constant_loc]);
        fmt::print("'\n");
        return offset + 2;
    }
};
