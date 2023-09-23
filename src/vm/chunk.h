#pragma once

#include "core/vector.h"
#include "core/log.h"

#include "vm/opcode.h"
#include "vm/value.h"
#include "vm/string.h"

#include <cassert>

class Chunk {
public:
    Vector<uint8_t> m_code;
    Vector<int32_t> m_lines;
    Vector<Value> m_constants;

    Chunk() = default;
    ~Chunk();

    int32_t code_count() const { return m_code.ssize(); }

    void write(OpCode opcode, int32_t line, std::initializer_list<uint8_t> bytes);
    void write(uint8_t byte, int32_t line);

    int32_t add_constant(Value value);

    void print_disassembly(const char* name) const;

    int32_t disassemble_instruction(int32_t offset) const;

private:
    int32_t print_simple_instruction(OpCode opcode, int32_t offset) const;

    int32_t print_constant_instruction(OpCode opcode, int32_t offset) const;

    int32_t print_byte_instruction(OpCode opcode, int32_t offset) const;

    int32_t print_invoke_instruction(OpCode opcode, int32_t offset) const;

    int32_t print_jump_instruction(OpCode opcode, int32_t sign, int32_t offset) const;

    int32_t print_object_new_instruction(OpCode opcode, int32_t offset) const;
};
