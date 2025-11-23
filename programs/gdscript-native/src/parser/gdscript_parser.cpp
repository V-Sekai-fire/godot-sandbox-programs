#include "gdscript_parser.h"
#include <sstream>

namespace gdscript {

GDScriptParser::GDScriptParser() {
}

bool GDScriptParser::is_at_end() const {
    return current.type == TokenType::EOF_TOKEN;
}

bool GDScriptParser::check(TokenType type) const {
    if (is_at_end()) return false;
    return current.type == type;
}

bool GDScriptParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token GDScriptParser::advance() {
    if (!is_at_end()) {
        previous = current;
        current = tokenizer.scan();
    }
    return previous;
}

Token GDScriptParser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    error_at_current(message);
    return current;
}

void GDScriptParser::synchronize() {
    advance();
    while (!is_at_end()) {
        if (previous.type == TokenType::NEWLINE) return;
        
        switch (current.type) {
            case TokenType::FUNC:
            case TokenType::VAR:
            case TokenType::RETURN:
            case TokenType::IF:
            case TokenType::FOR:
            case TokenType::WHILE:
                return;
            default:
                break;
        }
        advance();
    }
}

void GDScriptParser::error(const Token& token, const std::string& message) {
    std::ostringstream oss;
    oss << "[line " << token.line << "] Error: " << message;
    last_error_message = oss.str();
    SourceLocation loc;
    loc.line = token.line;
    loc.column = token.column;
    _errors.add_error(ErrorType::Parse, message, loc);
}

void GDScriptParser::error_at_current(const std::string& message) {
    error(current, message);
}

std::unique_ptr<ProgramNode> GDScriptParser::parse_program() {
    auto program = std::make_unique<ProgramNode>();
    
    while (!is_at_end()) {
        // Skip newlines and indentation at top level
        while (match(TokenType::NEWLINE) || match(TokenType::INDENT) || match(TokenType::DEDENT)) {
            // Skip
        }
        
        if (is_at_end()) break;
        
        if (check(TokenType::FUNC)) {
            auto func = parse_function();
            if (func) {
                program->functions.push_back(std::move(func));
            }
        } else {
            // Top-level statements (not implemented yet)
            synchronize();
        }
    }
    
    return program;
}

std::unique_ptr<FunctionNode> GDScriptParser::parse_function() {
    consume(TokenType::FUNC, "Expected 'func'");
    
    auto func = std::make_unique<FunctionNode>();
    
    // Function name
    if (check(TokenType::IDENTIFIER)) {
        func->name = current.literal;
        advance();
    } else {
        error_at_current("Expected function name");
        return nullptr;
    }
    
    // Parameters
    consume(TokenType::PARENTHESIS_OPEN, "Expected '(' after function name");
    
    if (!check(TokenType::PARENTHESIS_CLOSE)) {
        // Parse parameters (simplified - not handling parameters yet)
        do {
            if (check(TokenType::IDENTIFIER)) {
                std::string param_name = current.literal;
                advance();
                std::string param_type;
                if (match(TokenType::COLON)) {
                    if (check(TokenType::IDENTIFIER)) {
                        param_type = current.literal;
                        advance();
                    }
                }
                func->parameters.push_back({param_name, param_type});
            }
        } while (match(TokenType::COMMA));
    }
    
    consume(TokenType::PARENTHESIS_CLOSE, "Expected ')' after parameters");
    
    // Return type
    if (match(TokenType::FORWARD_ARROW)) {
        if (check(TokenType::IDENTIFIER)) {
            func->return_type = current.literal;
            advance();
        }
    }
    
    // Colon
    consume(TokenType::COLON, "Expected ':' after function signature");
    
    // Body (suite)
    parse_suite("function", func->body);
    
    return func;
}

void GDScriptParser::parse_suite(const std::string& context, std::vector<std::unique_ptr<StatementNode>>& statements) {
    // Check for multiline (newline followed by indent)
    bool multiline = false;
    if (match(TokenType::NEWLINE)) {
        multiline = true;
    }
    
    if (multiline) {
        if (!check(TokenType::INDENT)) {
            error_at_current("Expected indented block after " + context);
            return;
        }
        advance();
    }
    
    // Parse statements
    while (!is_at_end()) {
        // Skip newlines
        while (match(TokenType::NEWLINE)) {
            // Skip
        }
        
        // Check for dedent (end of block)
        if (multiline && check(TokenType::DEDENT)) {
            advance();
            break;
        }
        
        if (is_at_end()) break;
        
        auto stmt = parse_statement();
        if (stmt) {
            statements.push_back(std::move(stmt));
        } else {
            synchronize();
        }
    }
    
    if (multiline && check(TokenType::DEDENT)) {
        advance();
    }
}

std::unique_ptr<StatementNode> GDScriptParser::parse_statement() {
    if (check(TokenType::RETURN)) {
        return parse_return_statement();
    }
    if (check(TokenType::VAR)) {
        return parse_variable_declaration();
    }
    
    // Expression statement (not implemented yet)
    return nullptr;
}

std::unique_ptr<ReturnStatement> GDScriptParser::parse_return_statement() {
    consume(TokenType::RETURN, "Expected 'return'");
    
    auto stmt = std::make_unique<ReturnStatement>();
    
    if (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !is_at_end()) {
        stmt->value = parse_expression();
    }
    
    // Consume newline if present
    match(TokenType::NEWLINE);
    
    return stmt;
}

std::unique_ptr<VariableDeclaration> GDScriptParser::parse_variable_declaration() {
    consume(TokenType::VAR, "Expected 'var'");
    
    auto decl = std::make_unique<VariableDeclaration>();
    
    if (check(TokenType::IDENTIFIER)) {
        decl->name = current.literal;
        advance();
    } else {
        error_at_current("Expected variable name");
        return nullptr;
    }
    
    // Type hint
    if (match(TokenType::COLON)) {
        if (check(TokenType::IDENTIFIER)) {
            decl->type_hint = current.literal;
            advance();
        }
    }
    
    // Initializer
    if (match(TokenType::EQUAL)) {
        decl->initializer = parse_expression();
    }
    
    // Consume newline
    match(TokenType::NEWLINE);
    
    return decl;
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_expression() {
    return parse_equality();
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_equality() {
    auto expr = parse_comparison();
    
    while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
        Token op = previous;
        auto right = parse_comparison();
        auto binop = std::make_unique<BinaryOpExpr>();
        binop->left = std::move(expr);
        binop->right = std::move(right);
        binop->op = (op.type == TokenType::EQUAL_EQUAL) ? "==" : "!=";
        expr = std::move(binop);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_comparison() {
    auto expr = parse_term();
    
    while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
           match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
        Token op = previous;
        auto right = parse_term();
        auto binop = std::make_unique<BinaryOpExpr>();
        binop->left = std::move(expr);
        binop->right = std::move(right);
        if (op.type == TokenType::GREATER) binop->op = ">";
        else if (op.type == TokenType::GREATER_EQUAL) binop->op = ">=";
        else if (op.type == TokenType::LESS) binop->op = "<";
        else if (op.type == TokenType::LESS_EQUAL) binop->op = "<=";
        expr = std::move(binop);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_term() {
    auto expr = parse_factor();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token op = previous;
        auto right = parse_factor();
        auto binop = std::make_unique<BinaryOpExpr>();
        binop->left = std::move(expr);
        binop->right = std::move(right);
        binop->op = (op.type == TokenType::PLUS) ? "+" : "-";
        expr = std::move(binop);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_factor() {
    auto expr = parse_unary();
    
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        Token op = previous;
        auto right = parse_unary();
        auto binop = std::make_unique<BinaryOpExpr>();
        binop->left = std::move(expr);
        binop->right = std::move(right);
        if (op.type == TokenType::STAR) binop->op = "*";
        else if (op.type == TokenType::SLASH) binop->op = "/";
        else if (op.type == TokenType::PERCENT) binop->op = "%";
        expr = std::move(binop);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_unary() {
    if (match(TokenType::MINUS) || match(TokenType::NOT)) {
        Token op = previous;
        auto right = parse_unary();
        auto unary = std::make_unique<UnaryOpExpr>();
        unary->operand = std::move(right);
        unary->op = (op.type == TokenType::MINUS) ? "-" : "!";
        return unary;
    }
    
    return parse_primary();
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_primary() {
    if (match(TokenType::LITERAL)) {
        return make_literal(previous);
    }
    
    if (match(TokenType::IDENTIFIER)) {
        return make_identifier(previous);
    }
    
    if (match(TokenType::PARENTHESIS_OPEN)) {
        auto expr = parse_expression();
        consume(TokenType::PARENTHESIS_CLOSE, "Expected ')' after expression");
        return expr;
    }
    
    error_at_current("Expected expression");
    return nullptr;
}

std::unique_ptr<LiteralExpr> GDScriptParser::make_literal(const Token& token) {
    auto lit = std::make_unique<LiteralExpr>();
    
    // Parse literal value
    if (token.literal == "true") {
        lit->value = true;
    } else if (token.literal == "false") {
        lit->value = false;
    } else if (token.literal == "null") {
        lit->value = nullptr;
    } else {
        // Try to parse as number
        try {
            if (token.literal.find('.') != std::string::npos) {
                lit->value = std::stod(token.literal);
            } else {
                lit->value = static_cast<int64_t>(std::stoll(token.literal));
            }
        } catch (...) {
            // String literal (already has quotes stripped)
            lit->value = token.literal;
        }
    }
    
    return lit;
}

std::unique_ptr<IdentifierExpr> GDScriptParser::make_identifier(const Token& token) {
    auto ident = std::make_unique<IdentifierExpr>();
    ident->name = token.literal;
    return ident;
}

std::unique_ptr<ProgramNode> GDScriptParser::parse(const std::string& source) {
    last_error_message.clear();
    _errors.clear();
    
    tokenizer.set_source(source);
    current = tokenizer.scan();
    previous = current;
    
    try {
        return parse_program();
    } catch (...) {
        return nullptr;
    }
}

std::string GDScriptParser::getErrorMessage() const {
    return last_error_message;
}

} // namespace gdscript
