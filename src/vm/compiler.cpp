//
// Created by lasagnaphil on 2023-02-12.
//

#include "vm/compiler.h"

ParseRule Compiler::g_rules[] = {
        [TOKEN_LEFT_PAREN]    = {&Compiler::grouping,   &Compiler::call,        PREC_CALL},
        [TOKEN_RIGHT_PAREN]   = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {&Compiler::table,      NULL,                   PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_LEFT_BRACKET]  = {&Compiler::array,      &Compiler::subscript,   PREC_CALL},
        [TOKEN_RIGHT_BRACKET] = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_COMMA]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_DOT]           = {NULL,                  &Compiler::dot,         PREC_CALL},
        [TOKEN_MINUS]         = {&Compiler::unary,      &Compiler::binary,      PREC_TERM},
        [TOKEN_PLUS]          = {NULL,                  &Compiler::binary,      PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_SLASH]         = {NULL,                  &Compiler::binary,      PREC_FACTOR},
        [TOKEN_STAR]          = {NULL,                  &Compiler::binary,      PREC_FACTOR},
        [TOKEN_QUESTION_MARK] = {NULL,                  &Compiler::ternary,     PREC_TERNARY},
        [TOKEN_COLON]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_BANG]          = {&Compiler::unary,      NULL,                   PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL,                  &Compiler::binary,      PREC_EQUALITY},
        [TOKEN_EQUAL]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL,                  &Compiler::binary,      PREC_EQUALITY},
        [TOKEN_GREATER]       = {NULL,                  &Compiler::binary,      PREC_COMPARISON},
        [TOKEN_GREATER_EQUAL] = {NULL,                  &Compiler::binary,      PREC_COMPARISON},
        [TOKEN_LESS]          = {NULL,                  &Compiler::binary,      PREC_COMPARISON},
        [TOKEN_LESS_EQUAL]    = {NULL,                  &Compiler::binary,      PREC_COMPARISON},
        [TOKEN_IDENTIFIER]    = {&Compiler::variable,   NULL,                   PREC_NONE},
        [TOKEN_STRING]        = {&Compiler::string,     NULL,                   PREC_NONE},
        [TOKEN_NUMBER]        = {&Compiler::number,     NULL,                   PREC_NONE},
        [TOKEN_AND]           = {NULL,                  &Compiler::and_,        PREC_AND},
        [TOKEN_CLASS]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_ELSE]          = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_FALSE]         = {&Compiler::literal,    NULL,                   PREC_NONE},
        [TOKEN_FOR]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_FUN]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_IF]            = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_NIL]           = {&Compiler::literal,    NULL,                   PREC_NONE},
        [TOKEN_OR]            = {NULL,                  &Compiler::or_,         PREC_OR},
        [TOKEN_RETURN]        = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_SUPER]         = {&Compiler::super_,     NULL,                   PREC_NONE},
        [TOKEN_THIS]          = {&Compiler::this_,      NULL,                   PREC_NONE},
        [TOKEN_TRUE]          = {&Compiler::literal,    NULL,                   PREC_NONE},
        [TOKEN_VAR]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_WHILE]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_ERROR]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_EOF]           = {NULL,                  NULL,                   PREC_NONE},
};