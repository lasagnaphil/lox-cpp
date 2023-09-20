#pragma once

#include <cstdint>

#define OPCODE_LIST(X)    \
    X(OP_CONSTANT)        \
    X(OP_NIL)             \
    X(OP_TRUE)            \
    X(OP_FALSE)           \
    X(OP_POP)             \
    X(OP_GET_LOCAL)       \
    X(OP_SET_LOCAL)       \
    X(OP_GET_UPVALUE)     \
    X(OP_SET_UPVALUE)     \
    X(OP_GET_GLOBAL)      \
    X(OP_DEFINE_GLOBAL)   \
    X(OP_SET_GLOBAL)      \
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
    X(OP_JUMP)            \
    X(OP_JUMP_IF_FALSE)   \
    X(OP_LOOP)            \
    X(OP_CALL)            \
    X(OP_CLOSURE)         \
    X(OP_RETURN)          \
    X(OP_ARRAY_NEW)       \
    X(OP_TABLE_NEW)       \
    X(OP_GET)             \
    X(OP_SET)             \
    X(OP_GET_NOPOP)       \
    X(OP_SET_NOPOP)       \
    X(OP_INVALID)

enum OpCode : uint8_t {
#define X(val) val,
    OPCODE_LIST(X)
#undef X
    OP_COUNT
};

extern const char* g_opcode_str[OP_COUNT];
