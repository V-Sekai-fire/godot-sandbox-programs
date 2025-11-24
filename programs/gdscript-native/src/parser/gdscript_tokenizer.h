#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace gdscript {

// Token types matching Godot's tokenizer design
enum class TokenType {
    EMPTY,
    // Basic
    IDENTIFIER,
    LITERAL,
    // Comparison
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    EQUAL_EQUAL,
    BANG_EQUAL,
    // Logical
    AND,
    OR,
    NOT,
    // Math
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    // Assignment
    EQUAL,
    // Control flow
    IF,
    ELIF,
    ELSE,
    FOR,
    WHILE,
    BREAK,
    CONTINUE,
    RETURN,
    // Keywords
    FUNC,
    VAR,
    CONST,
    CLASS,
    EXTENDS,
    // Punctuation
    PARENTHESIS_OPEN,
    PARENTHESIS_CLOSE,
    BRACKET_OPEN,
    BRACKET_CLOSE,
    BRACE_OPEN,
    BRACE_CLOSE,
    COMMA,
    PERIOD,
    COLON,
    FORWARD_ARROW,
    // Whitespace
    NEWLINE,
    INDENT,
    DEDENT,
    // End of file
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string literal; // For IDENTIFIER, LITERAL
    int line;
    int column;
    
    Token() : type(TokenType::EMPTY), line(0), column(0) {}
    Token(TokenType t, int l, int c) : type(t), line(l), column(c) {}
    Token(TokenType t, const std::string& lit, int l, int c) 
        : type(t), literal(lit), line(l), column(c) {}
};

// GDScript tokenizer - converts source code to tokens (like Godot's tokenizer)
class GDScriptTokenizer {
private:
    const std::string* source;
    size_t current;
    size_t start;
    int line;
    int column;
    int tab_size;
    
    // Indentation tracking (like Godot's tokenizer)
    std::vector<int> indent_stack;
    int pending_indents;
    char indent_char; // ' ' or '\t'
    
    // Helper methods
    bool is_at_end() const;
    char advance();
    char peek() const;
    char peek_next() const;
    bool match(char expected);
    bool is_digit(char c) const;
    bool is_alpha(char c) const;
    bool is_alphanumeric(char c) const;
    
    // Token scanning methods
    void skip_whitespace();
    void skip_comment();
    Token scan_string();
    Token scan_number();
    Token scan_identifier();
    Token scan_token();
    
    // Indentation handling (like Godot's check_indent)
    void check_indent();
    int count_indent();
    int indent_level() const { return indent_stack.size(); }
    
public:
    GDScriptTokenizer();
    
    // Initialize with source code
    void set_source(const std::string& src);
    
    // Get next token (like Godot's scan())
    Token scan();
    
    // Get current position
    size_t get_current_position() const { return current; }
    int get_line() const { return line; }
    int get_column() const { return column; }
};

// Helper function to get token type name as string (for debugging)
std::string token_type_name(TokenType type);

} // namespace gdscript

