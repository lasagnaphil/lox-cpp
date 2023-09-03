#pragma once

#include "vm/chunk.h"
#include "vm/scanner.h"
#include "vm/string_interner.h"

#include "core/array.h"

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

enum Precedence : uint8_t {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
};

class Compiler;
using ParseFn = void (Compiler::*)(bool);

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

#define MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

class Compiler {
public:
    Compiler(Scanner* scanner, StringInterner* string_interner)
    : m_scanner(scanner), m_string_interner(string_interner) {}

    Chunk* current_chunk() const { return m_compiling_chunk; }

    void set_compiling_chunk(Chunk* chunk) {
        m_compiling_chunk = chunk;
    }

    void compile() {
        advance();
        while (!match(TOKEN_EOF)) {
            declaration();
        }
        end();
    }

    bool had_error() const { return m_had_error; }
    void advance() {
        m_previous = m_current;
        for (;;) {
            m_current = m_scanner->scan_token();
            if (m_current.type != TOKEN_ERROR) break;

            error_at_current(m_current.start);
        }
    }

    void consume(TokenType type, const char* message) {
        if (m_current.type == type) {
            advance();
            return;
        }
        error_at_current(message);
    }

    bool check(TokenType type) const {
        return m_current.type == type;
    }

    bool match(TokenType type) {
        if (!check(type)) return false;
        advance();
        return true;
    }

    void emit_byte(uint8_t byte) {
        current_chunk()->write(byte, m_previous.line);
    }

    void emit_bytes(uint8_t byte1, uint8_t byte2) {
        emit_byte(byte1);
        emit_byte(byte2);
    }

    void emit_return() {
        emit_byte(OP_RETURN);
    }

    uint8_t make_constant(Value value) {
        int32_t constant = current_chunk()->add_constant(value);
        if (constant > UINT8_MAX) {
            error("Too many constants in one chunk.");
            return 0;
        }
        return (uint8_t) constant;
    }

    void emit_constant(Value value) {
        emit_bytes(OP_CONSTANT, make_constant(value));
    }

    void end() {
        emit_return();
#ifdef DEBUG_PRINT_CODE
        if (!m_had_error) {
            current_chunk()->print_disassembly("code");
        }
#endif
    }

    void expression() {
        parse_precedence(PREC_ASSIGNMENT);
    }

    void var_declaration() {
        uint8_t global = parse_variable("Expect variable name.");
        if (match(TOKEN_EQUAL)) {
            expression();
        }
        else {
            emit_byte(OP_NIL);
        }
        consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
        define_variable(global);
    }

    void print_statement() {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after value.");
        emit_byte(OP_PRINT);
    }

    void synchronize() {
        m_panic_mode = false;

        while (m_current.type != TOKEN_EOF) {
            if (m_previous.type == TOKEN_SEMICOLON) return;
            switch (m_current.type) {
                case TOKEN_CLASS:
                case TOKEN_FUN:
                case TOKEN_VAR:
                case TOKEN_IF:
                case TOKEN_WHILE:
                case TOKEN_PRINT:
                case TOKEN_RETURN:
                    return;

                default:
                    ;
            }
        }
    }

    void expression_statement() {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
        emit_byte(OP_POP);
    }

    void declaration() {
        if (match(TOKEN_VAR)) {
            var_declaration();
        }
        else {
            statement();
        }

        if (m_panic_mode) synchronize();
    }

    void statement() {
        if (match(TOKEN_PRINT)) {
            print_statement();
        }
        else {
            expression_statement();
        }
    }

    void grouping(bool can_assign) {
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    }

    void number(bool can_assign) {
        double value = strtod(m_previous.start, nullptr);
        emit_constant(Value(value));
    }

    void string(bool can_assign) {
        ObjString* str = m_string_interner->create_string(m_previous.start + 1, m_previous.length - 2);
        auto value = Value(str);
        value.obj_incref();
        emit_constant(value);
    }

    void named_variable(Token name, bool can_assign) {
        uint8_t arg = identifier_constant(name);

        if (can_assign && match(TOKEN_EQUAL)) {
            expression();
            emit_bytes(OP_SET_GLOBAL, arg);
        }
        else {
            emit_bytes(OP_GET_GLOBAL, arg);
        }
    }

    void variable(bool can_assign) {
        named_variable(m_previous, can_assign);
    }

    void unary(bool can_assign) {
        TokenType op_type = m_previous.type;

        parse_precedence(PREC_UNARY);

        switch (op_type) {
            case TOKEN_BANG: emit_byte(OP_NOT); break;
            case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
            default: return;
        }
    }

    void binary(bool can_assign) {
        TokenType op_type = m_previous.type;
        ParseRule rule = get_rule(op_type);
        parse_precedence((Precedence)(rule.precedence + 1));

        switch (op_type) {
            case TOKEN_BANG_EQUAL:    emit_byte(OP_NOT_EQUAL); break;
            case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
            case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
            case TOKEN_GREATER_EQUAL: emit_byte(OP_GREATER_EQUAL); break;
            case TOKEN_LESS:          emit_byte(OP_LESS); break;
            case TOKEN_LESS_EQUAL:    emit_byte(OP_LESS_EQUAL); break;
            case TOKEN_PLUS:          emit_byte(OP_ADD); break;
            case TOKEN_MINUS:         emit_byte(OP_SUBTRACT); break;
            case TOKEN_STAR:          emit_byte(OP_MULTIPLY); break;
            case TOKEN_SLASH:         emit_byte(OP_DIVIDE); break;
            default: return; // Unreachable.
        }
    }

    void literal(bool assign) {
        switch (m_previous.type) {
            case TOKEN_FALSE: emit_byte(OP_FALSE); break;
            case TOKEN_NIL: emit_byte(OP_NIL); break;
            case TOKEN_TRUE: emit_byte(OP_TRUE); break;
            default: return; // Unreachable.
        }
    }

    void parse_precedence(Precedence precedence) {
        advance();
        ParseFn prefix_rule = get_rule(m_previous.type).prefix;
        if (prefix_rule == nullptr) {
            error("Expect expression.");
            return;
        }

        bool can_assign = precedence <= PREC_ASSIGNMENT;
        MEMBER_FN(*this, prefix_rule)(can_assign);

        while (precedence <= get_rule(m_current.type).precedence) {
            advance();
            ParseFn infix_rule = get_rule(m_previous.type).infix;
            MEMBER_FN(*this, infix_rule)(can_assign);
        }
    }

    uint8_t identifier_constant(const Token& name) {
        ObjString* str = m_string_interner->create_string(name.start, name.length);
        Value value = Value(str);
        value.obj_incref();
        return make_constant(value);
    }

    uint8_t parse_variable(const char* error_message) {
        consume(TOKEN_IDENTIFIER, error_message);
        return identifier_constant(m_previous);
    }

    void define_variable(uint8_t global) {
        emit_bytes(OP_DEFINE_GLOBAL, global);
    }

    ParseRule get_rule(TokenType type) const {
        return g_rules[type];
    }

    void error_at_current(const char* message) {
        error_at(m_current, message);
    }
    void error(const char* message) {
        error_at(m_previous, message);
    }
    void error_at(const Token& token, const char* message) {
        if (m_panic_mode) return;
        m_panic_mode = true;
        fmt::print(stderr, "[line {}] Error", token.line);
        if (token.type == TOKEN_EOF) {
            fmt::print(stderr, " at end");
        }
        else if (token.type == TOKEN_ERROR) {
            // Nothing.
        }
        else {
            fmt::print(stderr, " at ({},{})", token.length, token.start);
        }

        fmt::print(stderr, ": {}\n", message);
        m_had_error = true;
    }

    void reset_errors() {
        m_had_error = false;
        m_panic_mode = false;
    }

private:
    static ParseRule g_rules[];

    Scanner* m_scanner = nullptr;
    Chunk* m_compiling_chunk = nullptr;
    Token m_current;
    Token m_previous;
    StringInterner* m_string_interner;
    bool m_had_error = false;
    bool m_panic_mode = false;
};

#undef MEMBER_FN