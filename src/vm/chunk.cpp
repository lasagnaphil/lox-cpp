#include "vm/chunk.h"
#include "vm/table.h"

#include <string>

Chunk::~Chunk() {
    for (Value constant : m_constants) {
        if (constant.is_obj()) {
            constant.obj_decref();
        }
    }
}

void Chunk::write(OpCode opcode, int32_t line, std::initializer_list<uint8_t> bytes) {
    write(opcode, line);
    for (uint8_t byte : bytes) {
        write(byte, line);
    }
}

void Chunk::write(uint8_t byte, int32_t line) {
    m_code.push_back(byte);
    m_lines.push_back(line);
}

int32_t Chunk::add_constant(Value value) {
    m_constants.push_back(value);
    return m_constants.ssize() - 1;
}

void Chunk::print_disassembly(const char *name) const {
    fmt::print("== {} ==\n", name);
    for (int32_t offset = 0; offset < m_code.ssize(); ) {
        offset = disassemble_instruction(offset);
    }
}

int32_t Chunk::disassemble_instruction(int32_t offset) const {
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
        case OP_GET_GLOBAL:
        case OP_DEFINE_GLOBAL:
        case OP_SET_GLOBAL:
            return print_constant_instruction((OpCode)instr, offset);
        case OP_NIL:
        case OP_TRUE:
        case OP_FALSE:
        case OP_POP:
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
        case OP_PRINT:
        case OP_RETURN:
        case OP_TABLE_NEW:
        case OP_TABLE_GET:
        case OP_TABLE_SET:
            return print_simple_instruction((OpCode)instr, offset);
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
            return print_byte_instruction((OpCode)instr, offset);
        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
            return print_jump_instruction((OpCode)instr, 1, offset);
        case OP_LOOP:
            return print_jump_instruction((OpCode)instr, -1, offset);
        default:
            return print_simple_instruction(OP_INVALID, offset);
    }
}

int32_t Chunk::print_simple_instruction(OpCode opcode, int32_t offset) const {
    assert(opcode < OP_COUNT);
    fmt::print("{}\n", g_opcode_str[opcode]);
    return offset + 1;
}

int32_t Chunk::print_constant_instruction(OpCode opcode, int32_t offset) const {
    assert(opcode < OP_COUNT);
    uint8_t constant_loc = m_code[offset + 1];
    fmt::print("{:16s} {:4d} '", g_opcode_str[opcode], constant_loc);
    fmt::print(m_constants[constant_loc].to_std_string());
    fmt::print("'\n");
    return offset + 2;
}

int32_t Chunk::print_byte_instruction(OpCode opcode, int32_t offset) const {
    assert(opcode < OP_COUNT);
    uint8_t slot = m_code[offset + 1];
    fmt::print("{:16s} {:4d}\n", g_opcode_str[opcode], slot);
    return offset + 2;
}

int32_t Chunk::print_jump_instruction(OpCode opcode, int32_t sign, int32_t offset) const {
    assert(opcode < OP_COUNT);
    uint16_t jump = (uint16_t)(m_code[offset + 1] << 8);
    jump |= m_code[offset + 2];
    fmt::print("{:16s} {:4d} -> {:d}\n", g_opcode_str[opcode], offset, offset + 3 + sign * jump);
    return offset + 3;
}

