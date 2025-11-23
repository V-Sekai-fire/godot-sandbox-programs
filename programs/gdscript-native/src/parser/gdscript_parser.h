#pragma once

#include "ast.h"
#include "errors.h"
#include "gdscript_tokenizer.h"
#include <string>
#include <memory>
#include <vector>

namespace gdscript {

// GDScript parser using recursive descent (like Godot's parser)
class GDScriptParser {
private:
    GDScriptTokenizer tokenizer;
    Token current;
    Token previous;
    
    // Error collection
    ErrorCollection _errors;
    std::string last_error_message;
    
    // Helper methods (like Godot's parser)
    bool is_at_end() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token advance();
    Token consume(TokenType type, const std::string& message);
    void synchronize();
    
    // Error handling
    void error(const Token& token, const std::string& message);
    void error_at_current(const std::string& message);
    
    // Parsing methods (recursive descent, like Godot)
    std::unique_ptr<ProgramNode> parse_program();
    std::unique_ptr<FunctionNode> parse_function();
    std::unique_ptr<StatementNode> parse_statement();
    void parse_suite(const std::string& context, std::vector<std::unique_ptr<StatementNode>>& statements);
    
    // Expression parsing
    std::unique_ptr<ExpressionNode> parse_expression();
    std::unique_ptr<ExpressionNode> parse_equality();
    std::unique_ptr<ExpressionNode> parse_comparison();
    std::unique_ptr<ExpressionNode> parse_term();
    std::unique_ptr<ExpressionNode> parse_factor();
    std::unique_ptr<ExpressionNode> parse_unary();
    std::unique_ptr<ExpressionNode> parse_primary();
    
    // Statement parsing
    std::unique_ptr<ReturnStatement> parse_return_statement();
    std::unique_ptr<VariableDeclaration> parse_variable_declaration();
    
    // Helper to build AST nodes
    std::unique_ptr<LiteralExpr> make_literal(const Token& token);
    std::unique_ptr<IdentifierExpr> make_identifier(const Token& token);
    
public:
    GDScriptParser();
    
    // Parse GDScript source code and return AST
    std::unique_ptr<ProgramNode> parse(const std::string& source);
    
    // Get error message if parsing failed
    std::string getErrorMessage() const;
    
    // Get error collection
    const ErrorCollection& get_errors() const { return _errors; }
    ErrorCollection& get_errors() { return _errors; }
    
    // Check if parser is valid
    bool is_valid() const { return true; }
};

} // namespace gdscript
