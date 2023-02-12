//
// Created by lasagnaphil on 2023-02-12.
//

#include "vm/compiler.h"

ParseRule Compiler::g_rules[] = {
        [TOKEN_LEFT_PAREN]    = {&Compiler::grouping,   NULL,                   PREC_NONE},
        [TOKEN_RIGHT_PAREN]   = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_LEFT_BRACE]    = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_RIGHT_BRACE]   = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_COMMA]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_DOT]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_MINUS]         = {&Compiler::unary,      &Compiler::binary,      PREC_TERM},
        [TOKEN_PLUS]          = {NULL,                  &Compiler::binary,      PREC_TERM},
        [TOKEN_SEMICOLON]     = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_SLASH]         = {NULL,                  &Compiler::binary,      PREC_FACTOR},
        [TOKEN_STAR]          = {NULL,                  &Compiler::binary,      PREC_FACTOR},
        [TOKEN_BANG]          = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_BANG_EQUAL]    = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_EQUAL]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_EQUAL_EQUAL]   = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_GREATER]       = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_GREATER_EQUAL] = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_LESS]          = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_LESS_EQUAL]    = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_IDENTIFIER]    = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_STRING]        = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_NUMBER]        = {&Compiler::number,     NULL,                   PREC_NONE},
        [TOKEN_AND]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_CLASS]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_ELSE]          = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_FALSE]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_FOR]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_FUN]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_IF]            = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_NIL]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_OR]            = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_PRINT]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_RETURN]        = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_SUPER]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_THIS]          = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_TRUE]          = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_VAR]           = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_WHILE]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_ERROR]         = {NULL,                  NULL,                   PREC_NONE},
        [TOKEN_EOF]           = {NULL,                  NULL,                   PREC_NONE},
};