#pragma once

enum TokenType : uint8_t {
    // Single-character tokens.
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
    TOKEN_QUESTION_MARK, TOKEN_COLON,
    // One or two character tokens.
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,
    // Literals.
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
    // Keywords.
    TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
    TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

    TOKEN_ERROR, TOKEN_EOF
};

struct Token {
    const char* start;
    int32_t line;
    int32_t length;
    TokenType type;
};

inline bool identifiers_equal(const Token& a, const Token& b) {
    if (a.length != b.length) return false;
    return memcmp(a.start, b.start, a.length) == 0;
}

inline bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'A') ||
           c == '_';
}

inline bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

class Scanner {
public:
    void init(const char* source) {
        m_start = source;
        m_current = source;
        m_line = 1;
    }

    Token scan_token() {
        skip_whitespace();

        m_start = m_current;
        if (is_at_end()) return make_token(TOKEN_EOF);

        char c = advance();
        if (is_alpha(c)) return identifier();
        if (is_digit(c)) return number();

        switch (c) {
            case '(': return make_token(TOKEN_LEFT_PAREN);
            case ')': return make_token(TOKEN_RIGHT_PAREN);
            case '{': return make_token(TOKEN_LEFT_BRACE);
            case '}': return make_token(TOKEN_RIGHT_BRACE);
            case ';': return make_token(TOKEN_SEMICOLON);
            case ',': return make_token(TOKEN_COMMA);
            case '.': return make_token(TOKEN_DOT);
            case '-': return make_token(TOKEN_MINUS);
            case '+': return make_token(TOKEN_PLUS);
            case '/': return make_token(TOKEN_SLASH);
            case '*': return make_token(TOKEN_STAR);
            case '?': return make_token(TOKEN_QUESTION_MARK);
            case ':': return make_token(TOKEN_COLON);
            case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
            case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
            case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
            case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
            case '"': return string();
        }
        return error_token("Unexpected character.");
    }

    bool is_at_end() const {
        return *m_current == '\0';
    }

    char advance() {
        m_current++;
        return m_current[-1];
    }

    char peek() const {
        return *m_current;
    }

    char peek_next() const {
        if (is_at_end()) return '\0';
        return m_current[1];
    }

    bool match(char expected) {
        if (is_at_end()) return false;
        if (*m_current != expected) return false;
        m_current++;
        return true;
    }

    Token make_token(TokenType type) const {
        return {
            .start = m_start,
            .line = m_line,
            .length = (int32_t)(m_current - m_start),
            .type = type,
        };
    }

    Token error_token(const char* message) const {
        return {
            .start = message,
            .line = m_line,
            .length = (int32_t)strlen(message),
            .type = TOKEN_ERROR
        };
    }

    void skip_whitespace() {
        for (;;) {
            char c = peek();
            switch (c) {
                case ' ':
                case '\r':
                case '\t':
                    advance();
                    break;
                case '\n':
                    m_line++;
                    advance();
                    break;
                case '/':
                    if (peek_next() == '/') {
                        while (peek() != '\n' && !is_at_end()) advance();
                    }
                    else {
                        return;
                    }
                default:
                    return;
            }
        }
    }

    TokenType check_keyword(int32_t start, int32_t length, const char* rest, TokenType type) {
        if (m_current - m_start == start + length &&
            memcmp(m_start + start, rest, length) == 0) {
            return type;
        }
        return TOKEN_IDENTIFIER;
    }

    TokenType identifier_type() {
        switch (m_start[0]) {
            case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
            case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
            case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
            case 'f':
                if (m_current - m_start > 1) {
                    switch (m_start[1]) {
                        case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                        case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
                        case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
                    }
                }
                break;
            case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
            case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
            case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
            case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
            case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
            case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
            case 't':
                if (m_current - m_start > 1) {
                    switch (m_start[1]) {
                        case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
                        case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
                    }
                }
            case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
            case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
        }

        return TOKEN_IDENTIFIER;
    }

    Token identifier() {
        while (is_alpha(peek()) || is_digit(peek())) advance();
        return make_token(identifier_type());
    }

    Token number() {
        while (is_digit(peek())) advance();

        if (peek() == '.' && is_digit(peek_next())) {
            advance();
            while (is_digit(peek())) advance();
        }

        return make_token(TOKEN_NUMBER);
    }

    Token string() {
        while (peek() != '"' && !is_at_end()) {
            if (peek() == '\n') m_line++;
            advance();
        }
        if (is_at_end()) return error_token("Unterminated string.");
        advance();
        return make_token(TOKEN_STRING);
    }

private:
    const char* m_start;
    const char* m_current;
    int32_t m_line;
};