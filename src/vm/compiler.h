#pragma once

#include "vm/chunk.h"
#include "vm/parser.h"
#include "vm/string_interner.h"
#include "vm/table.h"
#include "vm/function.h"

#include "core/array.h"

#define UINT8_COUNT (UINT8_MAX + 1)

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

enum class InterpretResult {
    Ok,
    CompileError,
    RuntimeError
};

enum class FunctionType {
    Function, Script
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
    bool is_captured;
};

struct Upvalue {
    uint8_t index;
    bool is_local;
};

#define MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

class Compiler {
public:
    Compiler(Parser* parser, StringInterner* string_interner)
    : m_parser(parser), m_string_interner(string_interner) {}

    void init_script() {
        m_function = create_obj_function();
        m_function_type = FunctionType::Script;
        m_local_count = 0;
        m_scope_depth = 0;

        Local& local = m_locals[m_local_count++];
        local.depth = 0;
        local.name.start = "";
        local.name.length = 0;
        local.is_captured = false;
    }

    ObjFunction* compile_function(Compiler* enclosing) {
        m_function = create_obj_function();
        ObjString* fn_name = create_obj_string(m_parser->previous().start, m_parser->previous().length);
        m_function->name = fn_name;
        m_function_type = FunctionType::Function;

        m_enclosing = enclosing;

        m_local_count = 0;
        m_scope_depth = 0;

        Local& local = m_locals[m_local_count++];
        local.depth = 0;
        local.name.start = "";
        local.name.length = 0;
        local.is_captured = false;

        begin_scope();
        m_parser->consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
        if (!m_parser->check(TOKEN_RIGHT_PAREN)) {
            do {
                m_function->arity++;
                if (m_function->arity > 255) {
                    m_parser->error_at_current("Can't have more than 255 parameters.");
                }
                uint8_t constant = parse_variable("Expect parameter name.");
                define_variable(constant);
            } while (m_parser->match(TOKEN_COMMA));
        }
        m_parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
        m_parser->consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
        block();

        m_enclosing = nullptr;

        return end();
    }

    Chunk* current_chunk() const {
        return &m_function->chunk;
    }

    ObjFunction* compile() {
        m_parser->advance();
        while (!m_parser->match(TOKEN_EOF)) {
            declaration();
        }
        ObjFunction* function = end();
        return m_parser->had_error()? nullptr : function;
    }

    void emit_byte(uint8_t byte) {
        current_chunk()->write(byte, m_parser->previous().line);
    }

    void emit_bytes(uint8_t byte1, uint8_t byte2) {
        emit_byte(byte1);
        emit_byte(byte2);
    }

    void emit_loop(int32_t loop_start) {
        emit_byte(OP_LOOP);

        int32_t offset = current_chunk()->code_count() - loop_start + 2;
        if (offset > UINT16_MAX) m_parser->error("Loop body too large.");

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
            m_parser->error("Too much code to jump over.");
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
        emit_byte(OP_NIL);
        emit_byte(OP_RETURN);
    }

    uint8_t make_constant(Value value) {
        int32_t constant = current_chunk()->add_constant(value);
        if (constant > UINT8_MAX) {
            m_parser->error("Too many constants in one chunk.");
            return 0;
        }
        return (uint8_t) constant;
    }

    void emit_constant(Value value) {
        emit_bytes(OP_CONSTANT, make_constant(value));
    }

    ObjFunction* end() {
        emit_return();
        ObjFunction* function = m_function;
#ifdef DEBUG_PRINT_CODE
        if (!m_parser->had_error()) {
            current_chunk()->print_disassembly(
                function->name != nullptr ? function->name->chars : "<script>");
        }
#endif
        return function;
    }

    void begin_scope() {
        m_scope_depth++;
    }

    void end_scope() {
        m_scope_depth--;

        while (m_local_count > 0 &&
               m_locals[m_local_count - 1].depth > m_scope_depth) {
            if (m_locals[m_local_count - 1].is_captured) {
                emit_byte(OP_CLOSE_UPVALUE);
            }
            else {
                emit_byte(OP_POP);
            }
            m_local_count--;
        }
    }

    void expression() {
        parse_precedence(PREC_ASSIGNMENT);
    }

    void block() {
        while (!m_parser->check(TOKEN_RIGHT_BRACE) && !m_parser->check(TOKEN_EOF)) {
            declaration();
        }

        m_parser->consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    }

    void function(FunctionType type) {
        Compiler compiler(m_parser, m_string_interner);
        ObjFunction* function = compiler.compile_function(this);
        Value val_fn = Value(function);
        emit_bytes(OP_CLOSURE, make_constant(val_fn));

        for (int32_t i = 0; i < function->upvalue_count; i++) {
            emit_byte(compiler.m_upvalues[i].is_local ? 1 : 0);
            emit_byte(compiler.m_upvalues[i].index);
        }
    }

    void fun_declaration() {
        uint8_t global = parse_variable("Expect function name.");
        mark_initialized();
        function(FunctionType::Function);
        define_variable(global);
    }

    void var_declaration() {
        uint8_t global = parse_variable("Expect variable name.");
        if (m_parser->match(TOKEN_EQUAL)) {
            expression();
        }
        else {
            emit_byte(OP_NIL);
        }
        m_parser->consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
        define_variable(global);
    }

    void return_statement() {
        if (m_function_type == FunctionType::Script) {
            m_parser->error("Can't return from top-level code.");
        }
        if (m_parser->match(TOKEN_SEMICOLON)) {
            emit_return();
        }
        else {
            expression();
            m_parser->consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
            emit_byte(OP_RETURN);
        }
    }

    void expression_statement() {
        expression();
        m_parser->consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
        emit_byte(OP_POP);
    }

    void for_statement() {
        begin_scope();
        m_parser->consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
        // intializer clause
        if (m_parser->match(TOKEN_SEMICOLON)) {
            // No initializer.
        }
        else if (m_parser->match(TOKEN_VAR)) {
            var_declaration();
        }
        else {
            expression_statement();
        }

        int32_t loop_start = current_chunk()->code_count();
        int32_t exit_jump = -1;
        if (!m_parser->match(TOKEN_SEMICOLON)) {
            expression(); // condition expression
            m_parser->consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

            // Jump out of the loop if the condition is false.
            exit_jump = emit_jump(OP_JUMP_IF_FALSE);
            emit_byte(OP_POP);
        }

        if (!m_parser->match(TOKEN_RIGHT_PAREN)) {
            int32_t body_jump = emit_jump(OP_JUMP);
            int32_t increment_start = current_chunk()->code_count();
            expression(); // increment expression
            emit_byte(OP_POP);
            m_parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

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
        m_parser->consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
        expression();
        m_parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int32_t then_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
        statement();

        int32_t else_jump = emit_jump(OP_JUMP);

        patch_jump(then_jump);
        emit_byte(OP_POP);

        if (m_parser->match(TOKEN_ELSE)) statement();
        patch_jump(else_jump);
    }

    void while_statement() {
        int32_t loop_start = current_chunk()->code_count();
        m_parser->consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
        expression();
        m_parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

        int32_t exit_jump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
        statement();
        emit_loop(loop_start);

        patch_jump(exit_jump);
        emit_byte(OP_POP);
    }

    void declaration() {
        if (m_parser->match(TOKEN_FUN)) {
            fun_declaration();
        }
        else if (m_parser->match(TOKEN_VAR)) {
            var_declaration();
        }
        else {
            statement();
        }

        if (m_parser->panic_mode()) m_parser->synchronize();
    }

    void statement() {
        if (m_parser->match(TOKEN_FOR)) {
            for_statement();
        }
        else if (m_parser->match(TOKEN_IF)) {
            if_statement();
        }
        else if (m_parser->match(TOKEN_RETURN)) {
            return_statement();
        }
        else if (m_parser->match(TOKEN_WHILE)) {
            while_statement();
        }
        else if (m_parser->match(TOKEN_LEFT_BRACE)) {
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
        m_parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
    }

    void number(bool can_assign) {
        double value = strtod(m_parser->previous().start, nullptr);
        emit_constant(Value(value));
    }

    void string(bool can_assign) {
        ObjString* str = m_string_interner->create_string(m_parser->previous().start + 1, m_parser->previous().length - 2);
        auto value = Value(str);
        value.obj_incref();
        emit_constant(value);
    }

    void table(bool can_assign) {
        emit_byte(OP_TABLE_NEW);

        while (m_parser->match(TOKEN_IDENTIFIER)) {
            ObjString* str = m_string_interner->create_string(m_parser->previous().start, m_parser->previous().length);
            auto key = Value(str);
            key.obj_incref();
            emit_constant(key);

            m_parser->consume(TOKEN_EQUAL, "Expect '=' after identifier in table initializer list.");

            expression();

            emit_byte(OP_SET_NOPOP);

            if (!m_parser->match(TOKEN_COMMA)) break;
        }

        m_parser->consume(TOKEN_RIGHT_BRACE, "Expect '}' after table initializer list.");
    }

    void array(bool can_assign) {
        int32_t array_size_offset = emit_array_new();

        int32_t index = 0;
        while (true) {
            emit_constant(Value((double)index));
            expression();
            emit_byte(OP_SET_NOPOP);
            index++;
            if (!m_parser->match(TOKEN_COMMA)) break;
        }

        patch_array_new(array_size_offset, index);
        m_parser->consume(TOKEN_RIGHT_BRACKET, "Expect ']' after array initializer list.");
    }

    void subscript(bool can_assign) {
        expression();
        m_parser->consume(TOKEN_RIGHT_BRACKET, "Expect ']' after expression.");
        if (can_assign && m_parser->match(TOKEN_EQUAL)) {
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
        else if ((arg = resolve_upvalue(name)) != -1) {
            get_op = OP_GET_UPVALUE;
            set_op = OP_SET_UPVALUE;
        }
        else {
            arg = identifier_constant(name);
            get_op = OP_GET_GLOBAL;
            set_op = OP_SET_GLOBAL;
        }

        if (can_assign && m_parser->match(TOKEN_EQUAL)) {
            expression();
            emit_bytes(set_op, arg);
        }
        else {
            emit_bytes(get_op, arg);
        }
    }

    void variable(bool can_assign) {
        named_variable(m_parser->previous(), can_assign);
    }

    void unary(bool can_assign) {
        TokenType op_type = m_parser->previous().type;

        parse_precedence(PREC_UNARY);

        switch (op_type) {
            case TOKEN_BANG: emit_byte(OP_NOT); break;
            case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
            default: return;
        }
    }

    void binary(bool can_assign) {
        TokenType op_type = m_parser->previous().type;
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

    void call(bool can_assign) {
        uint8_t arg_count = argument_list();
        emit_bytes(OP_CALL, arg_count);
    }

    void literal(bool can_assign) {
        switch (m_parser->previous().type) {
            case TOKEN_FALSE: emit_byte(OP_FALSE); break;
            case TOKEN_NIL: emit_byte(OP_NIL); break;
            case TOKEN_TRUE: emit_byte(OP_TRUE); break;
            default: return; // Unreachable.
        }
    }

    void parse_precedence(Precedence precedence) {
        m_parser->advance();
        ParseFn prefix_rule = get_rule(m_parser->previous().type).prefix;
        if (prefix_rule == nullptr) {
            m_parser->error("Expect expression.");
            return;
        }

        bool can_assign = precedence <= PREC_ASSIGNMENT;
        MEMBER_FN(*this, prefix_rule)(can_assign);

        while (precedence <= get_rule(m_parser->current().type).precedence) {
            m_parser->advance();
            ParseFn infix_rule = get_rule(m_parser->previous().type).infix;
            MEMBER_FN(*this, infix_rule)(can_assign);
        }

        if (can_assign && m_parser->match(TOKEN_EQUAL)) {
            m_parser->error("Invalid assignment target.");
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
                    m_parser->error("Can't read local variable in its own initializer.");
                }
                return i;
            }
        }

        return -1;
    }

    int32_t add_upvalue(uint8_t index, bool is_local) {
        int32_t upvalue_count = m_function->upvalue_count;

        for (int32_t i = 0; i < upvalue_count; i++) {
            const Upvalue& upvalue = m_upvalues[i];
            if (upvalue.index == index && upvalue.is_local == is_local) {
                return i;
            }
        }
        if (upvalue_count == UINT8_COUNT) {
            m_parser->error("Too many closure variables in function.");
            return 0;
        }
        m_upvalues[upvalue_count].is_local = is_local;
        m_upvalues[upvalue_count].index = index;

        return m_function->upvalue_count++;
    }

    int32_t resolve_upvalue(Token name) {
        if (m_enclosing == nullptr) return -1;

        int32_t local = m_enclosing->resolve_local(name);
        if (local != -1) {
            m_enclosing->m_locals[local].is_captured = true;
            return add_upvalue(local, true);
        }

        int32_t upvalue = m_enclosing->resolve_upvalue(name);
        if (upvalue != -1) {
            return add_upvalue((uint8_t)upvalue, false);
        }

        return -1;
    }

    void add_local(Token name) {
        if (m_local_count == UINT8_COUNT) {
            m_parser->error("Too many local variables in function.");
            return;
        }
        Local& local = m_locals[m_local_count++];
        local.name = name;
        local.depth = -1;
        local.is_captured = false;
    }

    void declare_variable() {
        if (m_scope_depth == 0) return;

        Token name = m_parser->previous();
        for (int i = m_local_count - 1; i >= 0; i--) {
            Local& local = m_locals[i];
            if (local.depth != -1 && local.depth < m_scope_depth) {
                break;
            }
            if (identifiers_equal(name, local.name)) {
                m_parser->error("Already a variable with this name in this scope.");
            }
        }

        add_local(name);
    }

    uint8_t parse_variable(const char* error_message) {
        m_parser->consume(TOKEN_IDENTIFIER, error_message);

        declare_variable();
        if (m_scope_depth > 0) return 0;

        return identifier_constant(m_parser->previous());
    }

    void mark_initialized() {
        if (m_scope_depth == 0) return;
        m_locals[m_local_count - 1].depth = m_scope_depth;
    }

    void define_variable(uint8_t global) {
        if (m_scope_depth > 0) {
            mark_initialized();
            return;
        }

        emit_bytes(OP_DEFINE_GLOBAL, global);
    }

    uint8_t argument_list() {
        uint8_t arg_count = 0;
        if (!m_parser->check(TOKEN_RIGHT_PAREN)) {
            do {
                expression();
                if (arg_count == 255) {
                    m_parser->error("Can't have more than 255 arguments.");
                }
                arg_count++;
            } while (m_parser->match(TOKEN_COMMA));
        }
        m_parser->consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
        return arg_count;
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
        m_parser->consume(TOKEN_COLON, "Expect ':' after expression.");

        int32_t end_jump = emit_jump(OP_JUMP);

        patch_jump(else_jump);

        emit_byte(OP_POP);
        parse_precedence(PREC_TERNARY);

        patch_jump(end_jump);
    }

    ParseRule get_rule(TokenType type) const {
        return g_rules[type];
    }

    void reset_errors() {
        m_parser->reset_errors();
    }

private:
    static ParseRule g_rules[];

    Parser* m_parser = nullptr;
    StringInterner* m_string_interner;
    ObjFunction* m_function;
    FunctionType m_function_type = FunctionType::Script;

    Local m_locals[UINT8_COUNT];
    int32_t m_local_count = 0;

    Upvalue m_upvalues[UINT8_COUNT];
    int32_t m_scope_depth = 0;
    Compiler* m_enclosing = nullptr;
};

#undef MEMBER_FN