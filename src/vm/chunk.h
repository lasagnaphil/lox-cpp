#pragma once

#include "core/vector.h"
#include "core/log.h"

#include "vm/opcode.h"
#include "vm/value.h"
#include "vm/string.h"

#include <cassert>

inline void print_object(Value value) {
    switch (value.as.obj->type) {
        case OBJ_STRING:
            fmt::print("{}", value.as_string()->chars);
            break;
    }
}

inline void print_value(Value value) {
    switch (value.type) {
        case VAL_BOOL: fmt::print(value.as_bool()? "true": "false"); break;
        case VAL_NIL: fmt::print("nil"); break;
        case VAL_NUMBER: fmt::print("{:g}", value.as_number()); break;
        case VAL_OBJ: print_object(value); break;
    }
}

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

    int32_t print_jump_instruction(OpCode opcode, int32_t sign, int32_t offset) const;
};
