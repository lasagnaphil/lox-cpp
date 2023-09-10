#pragma once

#include "vm/chunk.h"
#include "vm/scanner.h"
#include "vm/string_interner.h"
#include "vm/table.h"

#include "core/array.h"

#define UINT8_COUNT (UINT8_MAX + 1)

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
    PREC_TERNARY,     // ?:
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . () []
    PREC_PRIMARY
};

class Compiler;
using ParseFn = void (Compiler::*)(bool);

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

struct Local {
    Token name;
    int32_t depth;
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

    void emit_loop(int32_t loop_start) {
        emit_byte(OP_LOOP);

        int32_t offset = current_chunk()->code_count() - loop_start + 2;
        if (offset > UINT16_MAX) error("Loop body too large.");

        emit_byte((offset >> 8) & 0xff);
        emit_byte(offset & 0xff);
    }

    int32_t emit_jump(uint8_t instruction) {
        emit_byte(instruction);
        emit_byte(0xff);
        emit_byte(0xff);
        return current_chunk()->m_code.ssize() - 2;
    }

    void patch_jump(int32_t offset) {
        int32_t jump = current_chunk()->m_code.ssize() - offset - 2;
        if (jump > UINT16_MAX) {
            error("Too much code to jump over.");
        }
        current_chunk()->m_code[offset] = (jump >> 8) & 0xff;
        current_chunk()->m_code[offset + 1] = jump & 0xff;
    }

    int32_t emit_array_new() {
        emit_byte(OP_ARRAY_NEW);
        emit_byte(0xff);
        emit_byte(0xff);
        return current_chunk()->m_code.ssize() - 2;
    }

    void patch_array_new(int32_t offset, int16_t count) {
        current_chunk()->m_code[offset] = (count >> 8) & 0xff;
        current_chunk()->m_code[offset + 1] = count & 0xff;
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

    void begin_scope() {
        m_scope_depth++;
    }

    void end_scope() {
        m_scope_depth--;

        while (m_local_count > 0 &&
               m_locals[m_local_count - 1].depth > m_scope_depth) {
            emit_byte(OP_POP);
            m_local_count--;
        }
    }

    void expression() {
        parse_precedence(PREC_ASSIGNMENT);
    }

    void block() {
        while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
            declaration();
        }

        consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
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
            switch (m_current.type) {;
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
            advance();
        }
    }

    void expression_statement() {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
        emit_byte(OP_POP);
    }

    void for_statement() {
        begin_scope();
        consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
        // intializer clause
        if (match(TOKEN_SEMICOLON)) {
            // No initializer.
        }
        else if (match(TOKEN_VAR)) {
            var_declaration();
        }
        else {
            expression_statement();
        }

        int32_t loop_start = current_chunk()->code_count();
        int32_t exit_jump = -1;
        if (!match(TOKEN_SEMICOLON)) {
            expression(); // condition expression
            consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

            // Jump out of the loop if the condition is false.
            exit_jump = emit_jump(OP_JUMP_IF_FALSE);
            emit_byte(OP_POP);
        }

        if (!match(TOKEN_RIGHT_PAREN)) {
            int32_t body_jump = emit_jump(OP_JUMP);
            int32_t increment_start = current_chunk()->code_count();
            expression(); // increment expression
            emit_byte(OP_POP);
            consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

            emit_loop(loop_start);
            loop_start = increment_start;
            patch_jump(body_jump);
        }

        statement(); // body statement
        emit_loop(loop_start);

        if (exit_jump != -1) {
            patch_jump(exit_jump);
            emit_byte(OP_POP);
        }

        end_scope();
    }

    void if_statement() {
        consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int32_t then_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
        statement();

        int32_t else_jump = emit_jump(OP_JUMP);

        patch_jump(then_jump);
        emit_byte(OP_POP);

        if (match(TOKEN_ELSE)) statement();
        patch_jump(else_jump);
    }

    void while_statement() {
        int32_t loop_start = current_chunk()->code_count();
        consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
        expression();
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int32_t exit_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
        statement();
        emit_loop(loop_start);

        patch_jump(exit_jump);
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
        else if (match(TOKEN_FOR)) {
            for_statement();
        }
        else if (match(TOKEN_IF)) {
            if_statement();
        }
        else if (match(TOKEN_WHILE)) {
            while_statement();
        }
        else if (match(TOKEN_LEFT_BRACE)) {
            begin_scope();
            block();
            end_scope();
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

    void table(bool can_assign) {
        emit_byte(OP_TABLE_NEW);

        while (match(TOKEN_IDENTIFIER)) {
            ObjString* str = m_string_interner->create_string(m_previous.start, m_previous.length);
            auto key = Value(str);
            key.obj_incref();
            emit_constant(key);

            consume(TOKEN_EQUAL, "Expect '=' after identifier in table initializer list.");

            expression();

            emit_byte(OP_SET_NOPOP);

            if (!match(TOKEN_COMMA)) break;
        }

        consume(TOKEN_RIGHT_BRACE, "Expect '}' after table initializer list.");
    }

    void array(bool can_assign) {
        int32_t array_size_offset = emit_array_new();

        int32_t index = 0;
        while (true) {
            emit_constant(Value((double)index));
            expression();
            emit_byte(OP_SET_NOPOP);
            index++;
            if (!match(TOKEN_COMMA)) break;
        }

        patch_array_new(array_size_offset, index);
        consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array initializer list.");
    }

    void subscript(bool can_assign) {
        expression();
        consume(TOKEN_RIGHT_BRACKET, "Expect ']' after expression.");
        if (can_assign && match(TOKEN_EQUAL)) {
            expression();
            emit_byte(OP_SET);
        }
        else {
            emit_byte(OP_GET);
        }
    }

    void named_variable(Token name, bool can_assign) {
        uint8_t get_op, set_op;
        int32_t arg = resolve_local(name);
        if (arg != -1) {
            get_op = OP_GET_LOCAL;
            set_op = OP_SET_LOCAL;
        }
        else {
            arg = identifier_constant(name);
            get_op = OP_GET_GLOBAL;
            set_op = OP_SET_GLOBAL;
        }

        if (can_assign && match(TOKEN_EQUAL)) {
            expression();
            emit_bytes(set_op, arg);
        }
        else {
            emit_bytes(get_op, arg);
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

    void literal(bool can_assign) {
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

        if (can_assign && match(TOKEN_EQUAL)) {
            error("Invalid assignment target.");
        }
    }

    uint8_t identifier_constant(Token name) {
        ObjString* str = m_string_interner->create_string(name.start, name.length);
        Value value = Value(str);
        value.obj_incref();
        return make_constant(value);
    }

    int32_t resolve_local(Token name) {
        for (int i = m_local_count - 1; i >= 0; i--) {
            Local& local = m_locals[i];
            if (identifiers_equal(name, local.name)) {
                if (local.depth == -1) {
                    error("Can't read local variable in its own initializer.");
                }
                return i;
            }
        }

        return -1;
    }

    void add_local(Token name) {
        if (m_local_count == UINT8_COUNT) {
            error("Too many local variables in function.");
            return;
        }
        Local& local = m_locals[m_local_count++];
        local.name = name;
        local.depth = -1;
    }

    void declare_variable() {
        if (m_scope_depth == 0) return;

        Token name = m_previous;
        for (int i = m_local_count - 1; i >= 0; i--) {
            Local& local = m_locals[i];
            if (local.depth != -1 && local.depth < m_scope_depth) {
                break;
            }
            if (identifiers_equal(name, local.name)) {
                error("Already a variable with this name in this scope.");
            }
        }

        add_local(name);
    }

    uint8_t parse_variable(const char* error_message) {
        consume(TOKEN_IDENTIFIER, error_message);

        declare_variable();
        if (m_scope_depth > 0) return 0;

        return identifier_constant(m_previous);
    }

    void mark_initialized() {
        m_locals[m_local_count - 1].depth = m_scope_depth;
    }

    void define_variable(uint8_t global) {
        if (m_scope_depth > 0) {
            mark_initialized();
            return;
        }

        emit_bytes(OP_DEFINE_GLOBAL, global);
    }

    void and_(bool can_assign) {
        int32_t end_jump = emit_jump(OP_JUMP_IF_FALSE);

        emit_byte(OP_POP);
        parse_precedence(PREC_AND);

        patch_jump(end_jump);
    }

    void or_(bool can_assign) {
        int32_t else_jump = emit_jump(OP_JUMP_IF_FALSE);
        int32_t end_jump = emit_jump(OP_JUMP);

        patch_jump(else_jump);
        emit_byte(OP_POP);

        parse_precedence(PREC_OR);
        patch_jump(end_jump);
    }

    void ternary(bool can_assign) {
        int32_t else_jump = emit_jump(OP_JUMP_IF_FALSE);

        emit_byte(OP_POP);
        parse_precedence(PREC_TERNARY);
        consume(TOKEN_COLON, "Expect ':' after expression.");

        int32_t end_jump = emit_jump(OP_JUMP);

        patch_jump(else_jump);

        emit_byte(OP_POP);
        parse_precedence(PREC_TERNARY);

        patch_jump(end_jump);
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
    Local m_locals[UINT8_COUNT];
    int32_t m_local_count = 0;
    int32_t m_scope_depth = 0;
    bool m_had_error = false;
    bool m_panic_mode = false;
};

#undef MEMBER_FN