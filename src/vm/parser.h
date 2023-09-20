#pragma once

#include "vm/chunk.h"
#include "vm/scanner.h"

class Parser {
public:
    void init(const char* source) {
        m_scanner.init(source);
    }

    void advance() {
        m_previous = m_current;
        for (;;) {
            m_current = m_scanner.scan_token();
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
                case TOKEN_RETURN:
                    return;

                default:
                    ;
            }
            advance();
        }
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
            fmt::print(stderr, " at '{}'", std::string_view(token.start, token.length));
        }

        fmt::print(stderr, ": {}\n", message);
        m_had_error = true;
    }

    void reset_errors() {
        m_had_error = false;
        m_panic_mode = false;
    }

    Token current() const { return m_current; }
    Token previous() const { return m_previous; }
    bool had_error() const { return m_had_error; }
    bool panic_mode() const { return m_panic_mode; }

private:
    Scanner m_scanner;

    Token m_current;
    Token m_previous;
    bool m_had_error = false;
    bool m_panic_mode = false;
};
