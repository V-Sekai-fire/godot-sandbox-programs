#include "gdscript_tokenizer.h"
#include <cctype>
#include <sstream>
#include <iostream>

namespace gdscript {

GDScriptTokenizer::GDScriptTokenizer() 
    : source(nullptr), current(0), start(0), line(1), column(1), 
      tab_size(4), pending_indents(0), indent_char('\0') {
    indent_stack.push_back(0);
}

void GDScriptTokenizer::set_source(const std::string& src) {
    source = &src;
    current = 0;
    start = 0;
    line = 1;
    column = 1;
    indent_stack.clear();
    indent_stack.push_back(0);
    pending_indents = 0;
    indent_char = '\0';
}

bool GDScriptTokenizer::is_at_end() const {
    return current >= source->length();
}

char GDScriptTokenizer::advance() {
    if (is_at_end()) return '\0';
    char c = (*source)[current++];
    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

char GDScriptTokenizer::peek() const {
    if (is_at_end()) return '\0';
    return (*source)[current];
}

char GDScriptTokenizer::peek_next() const {
    if (current + 1 >= source->length()) return '\0';
    return (*source)[current + 1];
}

bool GDScriptTokenizer::match(char expected) {
    if (is_at_end()) return false;
    if ((*source)[current] != expected) return false;
    current++;
    if (expected == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return true;
}

bool GDScriptTokenizer::is_digit(char c) const {
    return std::isdigit(static_cast<unsigned char>(c));
}

bool GDScriptTokenizer::is_alpha(char c) const {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool GDScriptTokenizer::is_alphanumeric(char c) const {
    return is_alpha(c) || is_digit(c);
}

void GDScriptTokenizer::skip_whitespace() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t') {
            advance();
        } else if (c == '\n') {
            advance();
            // After newline, check indentation of next line
            // This sets pending_indents which will be emitted as INDENT tokens
            // IMPORTANT: check_indent() counts but doesn't consume the indentation
            check_indent();
            // Don't consume the indentation here - it will be consumed when we emit INDENT token
            // Just break out of whitespace skipping
            break;
        } else {
            break;
        }
    }
}

void GDScriptTokenizer::skip_comment() {
    if (peek() == '#') {
        while (peek() != '\n' && !is_at_end()) {
            advance();
        }
    }
}

Token GDScriptTokenizer::scan_string() {
    while (peek() != '"' && !is_at_end()) {
        if (peek() == '\n') {
            // Error: unterminated string
            break;
        }
        advance();
    }
    
    if (is_at_end()) {
        // Error: unterminated string
        return Token(TokenType::EOF_TOKEN, line, column);
    }
    
    // Consume closing quote
    advance();
    
    std::string value = source->substr(start + 1, current - start - 2);
    return Token(TokenType::LITERAL, value, line, column);
}

Token GDScriptTokenizer::scan_number() {
    while (is_digit(peek())) {
        advance();
    }
    
    // Look for fractional part
    if (peek() == '.' && is_digit(peek_next())) {
        advance(); // Consume '.'
        while (is_digit(peek())) {
            advance();
        }
    }
    
    std::string value = source->substr(start, current - start);
    return Token(TokenType::LITERAL, value, line, column);
}

Token GDScriptTokenizer::scan_identifier() {
    while (is_alphanumeric(peek())) {
        advance();
    }
    
    std::string text = source->substr(start, current - start);
    
    // Check for keywords
    if (text == "func") return Token(TokenType::FUNC, line, column);
    if (text == "var") return Token(TokenType::VAR, line, column);
    if (text == "return") return Token(TokenType::RETURN, line, column);
    if (text == "if") return Token(TokenType::IF, line, column);
    if (text == "elif") return Token(TokenType::ELIF, line, column);
    if (text == "else") return Token(TokenType::ELSE, line, column);
    if (text == "for") return Token(TokenType::FOR, line, column);
    if (text == "while") return Token(TokenType::WHILE, line, column);
    if (text == "and") return Token(TokenType::AND, line, column);
    if (text == "or") return Token(TokenType::OR, line, column);
    if (text == "not") return Token(TokenType::NOT, line, column);
    if (text == "true" || text == "false" || text == "null") {
        return Token(TokenType::LITERAL, text, line, column);
    }
    
    return Token(TokenType::IDENTIFIER, text, line, column);
}

int GDScriptTokenizer::count_indent() {
    size_t saved_current = current;
    int saved_line = line;
    int saved_column = column;
    
    int indent_count = 0;
    while (!is_at_end()) {
        char c = peek();
        if (c == '\t') {
            indent_count += tab_size;
            advance();
        } else if (c == ' ') {
            indent_count += 1;
            advance();
        } else {
            break;
        }
    }
    
    // Restore position
    current = saved_current;
    line = saved_line;
    column = saved_column;
    
    return indent_count;
}

void GDScriptTokenizer::check_indent() {
    if (is_at_end()) {
        // Emit DEDENT for all remaining levels
        while (indent_level() > 0) {
            indent_stack.pop_back();
            pending_indents--;
        }
        return;
    }
    
    int indent_count = count_indent();
    
    if (peek() != ' ' && peek() != '\t' && peek() != '\n' && peek() != '#') {
        // First character is not whitespace - clear all indentation
        while (indent_level() > 0) {
            indent_stack.pop_back();
            pending_indents--;
        }
        return;
    }
    
    // Skip empty lines and comments
    while (peek() == '\n' || peek() == '#') {
        if (peek() == '#') {
            skip_comment();
        }
        if (peek() == '\n') {
            advance();
        }
        if (is_at_end()) {
            while (indent_level() > 0) {
                indent_stack.pop_back();
                pending_indents--;
            }
            return;
        }
        indent_count = count_indent();
    }
    
    // Check indentation character consistency
    if (indent_count > 0) {
        char current_indent_char = peek();
        if (indent_char == '\0') {
            indent_char = current_indent_char;
        } else if (current_indent_char != indent_char) {
            // Error: mixed indentation (would be handled in full implementation)
        }
    }
    
    // Check if indent or dedent
    int previous_indent = 0;
    if (indent_level() > 0) {
        previous_indent = indent_stack.back();
    }
    
    if (indent_count == previous_indent) {
        // No change
        return;
    }
    
    if (indent_count > previous_indent) {
        // Indentation increased
        indent_stack.push_back(indent_count);
        pending_indents++;
    } else {
        // Indentation decreased
        while (indent_level() > 0 && indent_stack.back() > indent_count) {
            indent_stack.pop_back();
            pending_indents--;
        }
    }
}

Token GDScriptTokenizer::scan_token() {
    // Check for pending INDENT/DEDENT tokens
    // These are set by check_indent() which is called from skip_whitespace() when we see a newline
    if (pending_indents > 0) {
        pending_indents--;
        // After emitting INDENT, consume the indentation whitespace that was counted
        int indent_to_skip = indent_stack.empty() ? 0 : indent_stack.back();
        int skipped = 0;
        while ((peek() == ' ' || peek() == '\t') && skipped < indent_to_skip) {
            if (peek() == '\t') {
                skipped += tab_size;
            } else {
                skipped += 1;
            }
            advance();
        }
        return Token(TokenType::INDENT, line, column);
    } else if (pending_indents < 0) {
        pending_indents++;
        return Token(TokenType::DEDENT, line, column);
    }
    
    // Skip whitespace (this may call check_indent() which sets pending_indents)
    skip_whitespace();
    
    // Check again for pending_indents that were set by skip_whitespace()
    if (pending_indents > 0) {
        pending_indents--;
        // Consume the indentation whitespace
        int indent_to_skip = indent_stack.empty() ? 0 : indent_stack.back();
        int skipped = 0;
        while ((peek() == ' ' || peek() == '\t') && skipped < indent_to_skip) {
            if (peek() == '\t') {
                skipped += tab_size;
            } else {
                skipped += 1;
            }
            advance();
        }
        return Token(TokenType::INDENT, line, column);
    } else if (pending_indents < 0) {
        pending_indents++;
        return Token(TokenType::DEDENT, line, column);
    }
    
    start = current;
    int start_line = line;
    int start_column = column;
    
    if (is_at_end()) {
        return Token(TokenType::EOF_TOKEN, line, column);
    }
    
    char c = advance();
    
    // Single character tokens
    switch (c) {
        case '(': return Token(TokenType::PARENTHESIS_OPEN, start_line, start_column);
        case ')': return Token(TokenType::PARENTHESIS_CLOSE, start_line, start_column);
        case '[': return Token(TokenType::BRACKET_OPEN, start_line, start_column);
        case ']': return Token(TokenType::BRACKET_CLOSE, start_line, start_column);
        case '{': return Token(TokenType::BRACE_OPEN, start_line, start_column);
        case '}': return Token(TokenType::BRACE_CLOSE, start_line, start_column);
        case ',': return Token(TokenType::COMMA, start_line, start_column);
        case '.': return Token(TokenType::PERIOD, start_line, start_column);
        case ':': return Token(TokenType::COLON, start_line, start_column);
        case '+': return Token(TokenType::PLUS, start_line, start_column);
        case '-': 
            if (peek() == '>') {
                advance();
                return Token(TokenType::FORWARD_ARROW, start_line, start_column);
            }
            return Token(TokenType::MINUS, start_line, start_column);
        case '*': return Token(TokenType::STAR, start_line, start_column);
        case '/': return Token(TokenType::SLASH, start_line, start_column);
        case '%': return Token(TokenType::PERCENT, start_line, start_column);
        case '=':
            if (peek() == '=') {
                advance();
                return Token(TokenType::EQUAL_EQUAL, start_line, start_column);
            }
            return Token(TokenType::EQUAL, start_line, start_column);
        case '!':
            if (peek() == '=') {
                advance();
                return Token(TokenType::BANG_EQUAL, start_line, start_column);
            }
            return Token(TokenType::NOT, start_line, start_column);
        case '<':
            if (peek() == '=') {
                advance();
                return Token(TokenType::LESS_EQUAL, start_line, start_column);
            }
            return Token(TokenType::LESS, start_line, start_column);
        case '>':
            if (peek() == '=') {
                advance();
                return Token(TokenType::GREATER_EQUAL, start_line, start_column);
            }
            return Token(TokenType::GREATER, start_line, start_column);
        case '"': return scan_string();
        case '#': 
            skip_comment();
            return scan_token(); // Recursively get next token after comment
    }
    
    // Numbers
    if (is_digit(c)) {
        current--; // Back up to include the digit
        column--;
        return scan_number();
    }
    
    // Identifiers and keywords
    if (is_alpha(c)) {
        current--; // Back up to include the character
        column--;
        return scan_identifier();
    }
    
    // Unknown character
    return Token(TokenType::EMPTY, start_line, start_column);
}

Token GDScriptTokenizer::scan() {
    return scan_token();
}

std::string token_type_name(TokenType type) {
    switch (type) {
        case TokenType::EMPTY: return "EMPTY";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::LITERAL: return "LITERAL";
        case TokenType::LESS: return "LESS";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER: return "GREATER";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::BANG_EQUAL: return "BANG_EQUAL";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::IF: return "IF";
        case TokenType::ELIF: return "ELIF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::FOR: return "FOR";
        case TokenType::WHILE: return "WHILE";
        case TokenType::BREAK: return "BREAK";
        case TokenType::CONTINUE: return "CONTINUE";
        case TokenType::RETURN: return "RETURN";
        case TokenType::FUNC: return "FUNC";
        case TokenType::VAR: return "VAR";
        case TokenType::CONST: return "CONST";
        case TokenType::CLASS: return "CLASS";
        case TokenType::EXTENDS: return "EXTENDS";
        case TokenType::PARENTHESIS_OPEN: return "PARENTHESIS_OPEN";
        case TokenType::PARENTHESIS_CLOSE: return "PARENTHESIS_CLOSE";
        case TokenType::BRACKET_OPEN: return "BRACKET_OPEN";
        case TokenType::BRACKET_CLOSE: return "BRACKET_CLOSE";
        case TokenType::BRACE_OPEN: return "BRACE_OPEN";
        case TokenType::BRACE_CLOSE: return "BRACE_CLOSE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::PERIOD: return "PERIOD";
        case TokenType::COLON: return "COLON";
        case TokenType::FORWARD_ARROW: return "FORWARD_ARROW";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::INDENT: return "INDENT";
        case TokenType::DEDENT: return "DEDENT";
        case TokenType::EOF_TOKEN: return "EOF_TOKEN";
        default: return "UNKNOWN";
    }
}

} // namespace gdscript

