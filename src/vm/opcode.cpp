#include "opcode.h"

const char* g_opcode_str[OP_COUNT] = {
#define X(val) #val,
    OPCODE_LIST(X)
#undef X
};
