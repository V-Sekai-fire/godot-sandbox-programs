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
    std::unique_ptr<ProgramNode> program = std::make_unique<ProgramNode>();
    
    while (!is_at_end()) {
        // Skip newlines and indentation at top level
        while (match(TokenType::NEWLINE) || match(TokenType::INDENT) || match(TokenType::DEDENT)) {
            // Skip
        }
        
        if (is_at_end()) break;
        
        if (check(TokenType::FUNC)) {
            std::unique_ptr<FunctionNode> func = parse_function();
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
    
    std::unique_ptr<FunctionNode> func = std::make_unique<FunctionNode>();
    
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
        
        std::unique_ptr<StatementNode> stmt = parse_statement();
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
    if (check(TokenType::IF)) {
        return parse_if_statement();
    }
    if (check(TokenType::WHILE)) {
        return parse_while_statement();
    }
    if (check(TokenType::FOR)) {
        return parse_for_statement();
    }
    
    // Try assignment statement: identifier = expression
    // Simple approach: if we see identifier followed by =, it's an assignment
    // We'll check by looking ahead one token
    if (check(TokenType::IDENTIFIER)) {
        // Temporarily advance to peek at next token
        Token saved_prev = previous;
        Token saved_curr = current;
        advance(); // Move to next token
        bool is_assignment = check(TokenType::EQUAL);
        // Restore (we can't rewind tokenizer, but we can restore our tokens)
        // Actually, we need a different approach - just try parsing assignment first
        // and if it fails, fall back to expression
        previous = saved_prev;
        current = saved_curr;
        
        if (is_assignment) {
            return parse_assignment_statement();
        }
    }
    
    // Expression statement (fallback for function calls, etc.)
    std::unique_ptr<ExpressionNode> expr = parse_expression();
    if (expr) {
        std::unique_ptr<ExpressionStatement> stmt = std::make_unique<ExpressionStatement>();
        stmt->expression = std::move(expr);
        match(TokenType::NEWLINE); // Consume newline
        return stmt;
    }
    
    return nullptr;
}

std::unique_ptr<ReturnStatement> GDScriptParser::parse_return_statement() {
    consume(TokenType::RETURN, "Expected 'return'");
    
    std::unique_ptr<ReturnStatement> stmt = std::make_unique<ReturnStatement>();
    
    if (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !is_at_end()) {
        stmt->value = parse_expression();
        }
    
    // Consume newline if present
    match(TokenType::NEWLINE);
    
    return stmt;
        }

std::unique_ptr<VariableDeclaration> GDScriptParser::parse_variable_declaration() {
    consume(TokenType::VAR, "Expected 'var'");
    
    std::unique_ptr<VariableDeclaration> decl = std::make_unique<VariableDeclaration>();
    
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

std::unique_ptr<AssignmentStatement> GDScriptParser::parse_assignment_statement() {
    // Target (must be identifier)
    if (!check(TokenType::IDENTIFIER)) {
        return nullptr; // Not an assignment, fail silently
    }
    
    std::unique_ptr<IdentifierExpr> target = make_identifier(current);
    advance();
    
    // Assignment operator - if not present, this isn't an assignment
    if (!check(TokenType::EQUAL)) {
        return nullptr; // Not an assignment, fail silently
    }
    advance(); // consume '='
    std::string op = "="; // For now, only support simple assignment
    
    // Value expression
    std::unique_ptr<ExpressionNode> value = parse_expression();
    if (!value) {
        error_at_current("Expected expression for assignment value");
        return nullptr;
    }
    
    std::unique_ptr<AssignmentStatement> stmt = std::make_unique<AssignmentStatement>();
    stmt->target = std::move(target);
    stmt->op = op;
    stmt->value = std::move(value);
    
    // Consume newline
    match(TokenType::NEWLINE);
    
    return stmt;
}

std::unique_ptr<IfStatement> GDScriptParser::parse_if_statement() {
    consume(TokenType::IF, "Expected 'if'");
    
    std::unique_ptr<IfStatement> stmt = std::make_unique<IfStatement>();
    stmt->condition = parse_expression();
    if (!stmt->condition) {
        error_at_current("Expected condition expression after 'if'");
        return nullptr;
    }
    
    consume(TokenType::COLON, "Expected ':' after if condition");
    
    // Parse then body
    parse_suite("if", stmt->then_body);
    
    // Parse elif branches
    while (check(TokenType::ELIF)) {
        advance(); // consume ELIF
        std::unique_ptr<ExpressionNode> elif_cond = parse_expression();
        if (!elif_cond) {
            error_at_current("Expected condition expression after 'elif'");
            break;
        }
        consume(TokenType::COLON, "Expected ':' after elif condition");
        
        std::vector<std::unique_ptr<StatementNode>> elif_body;
        parse_suite("elif", elif_body);
        stmt->elif_branches.push_back({std::move(elif_cond), std::move(elif_body)});
    }
    
    // Parse else branch
    if (check(TokenType::ELSE)) {
        advance(); // consume ELSE
        consume(TokenType::COLON, "Expected ':' after 'else'");
        parse_suite("else", stmt->else_body);
    }
    
    return stmt;
}

std::unique_ptr<WhileStatement> GDScriptParser::parse_while_statement() {
    consume(TokenType::WHILE, "Expected 'while'");
    
    std::unique_ptr<WhileStatement> stmt = std::make_unique<WhileStatement>();
    stmt->condition = parse_expression();
    if (!stmt->condition) {
        error_at_current("Expected condition expression after 'while'");
        return nullptr;
    }
    
    consume(TokenType::COLON, "Expected ':' after while condition");
    
    // Parse body
    parse_suite("while", stmt->body);
    
    return stmt;
}

std::unique_ptr<ForStatement> GDScriptParser::parse_for_statement() {
    consume(TokenType::FOR, "Expected 'for'");
    
    std::unique_ptr<ForStatement> stmt = std::make_unique<ForStatement>();
    
    // Variable name
    if (!check(TokenType::IDENTIFIER)) {
        error_at_current("Expected identifier for for loop variable");
        return nullptr;
    }
    stmt->variable_name = current.literal;
    advance();
    
    // "in" keyword
    // Note: GDScript uses "in" but we might not have that token yet
    // For now, assume the iterable expression follows
    if (check(TokenType::IDENTIFIER) && current.literal == "in") {
        advance(); // consume "in"
    }
    
    // Iterable expression
    stmt->iterable = parse_expression();
    if (!stmt->iterable) {
        error_at_current("Expected iterable expression after 'for'");
        return nullptr;
    }
    
    consume(TokenType::COLON, "Expected ':' after for loop");
    
    // Parse body
    parse_suite("for", stmt->body);
    
    return stmt;
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_expression() {
    return parse_equality();
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_equality() {
    std::unique_ptr<ExpressionNode> expr = parse_comparison();
    
    while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
        Token op = previous;
        std::unique_ptr<ExpressionNode> right = parse_comparison();
        std::unique_ptr<BinaryOpExpr> binop = std::make_unique<BinaryOpExpr>();
        binop->left = std::move(expr);
        binop->right = std::move(right);
        binop->op = (op.type == TokenType::EQUAL_EQUAL) ? "==" : "!=";
        expr = std::move(binop);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_comparison() {
    std::unique_ptr<ExpressionNode> expr = parse_term();
    
    while (match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL) ||
           match(TokenType::LESS) || match(TokenType::LESS_EQUAL)) {
        Token op = previous;
        std::unique_ptr<ExpressionNode> right = parse_term();
        std::unique_ptr<BinaryOpExpr> binop = std::make_unique<BinaryOpExpr>();
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
    std::unique_ptr<ExpressionNode> expr = parse_factor();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token op = previous;
        std::unique_ptr<ExpressionNode> right = parse_factor();
        std::unique_ptr<BinaryOpExpr> binop = std::make_unique<BinaryOpExpr>();
        binop->left = std::move(expr);
        binop->right = std::move(right);
        binop->op = (op.type == TokenType::PLUS) ? "+" : "-";
        expr = std::move(binop);
    }
    
    return expr;
}

std::unique_ptr<ExpressionNode> GDScriptParser::parse_factor() {
    std::unique_ptr<ExpressionNode> expr = parse_unary();
    
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        Token op = previous;
        std::unique_ptr<ExpressionNode> right = parse_unary();
        std::unique_ptr<BinaryOpExpr> binop = std::make_unique<BinaryOpExpr>();
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
        std::unique_ptr<ExpressionNode> right = parse_unary();
        std::unique_ptr<UnaryOpExpr> unary = std::make_unique<UnaryOpExpr>();
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
        std::unique_ptr<ExpressionNode> expr = make_identifier(previous);
        
        // Check if this is a function call: identifier(...)
        if (check(TokenType::PARENTHESIS_OPEN)) {
            advance(); // consume '('
            std::unique_ptr<CallExpr> call = std::make_unique<CallExpr>();
            call->callee = std::move(expr);
            
            // Parse arguments
            if (!check(TokenType::PARENTHESIS_CLOSE)) {
                do {
                    std::unique_ptr<ExpressionNode> arg = parse_expression();
                    if (arg) {
                        call->arguments.push_back(std::move(arg));
                    }
                } while (match(TokenType::COMMA));
            }
            
            consume(TokenType::PARENTHESIS_CLOSE, "Expected ')' after function arguments");
            return call;
        }
        
        return expr;
    }
    
    if (match(TokenType::PARENTHESIS_OPEN)) {
        std::unique_ptr<ExpressionNode> expr = parse_expression();
        consume(TokenType::PARENTHESIS_CLOSE, "Expected ')' after expression");
        return expr;
        }
        
    error_at_current("Expected expression");
    return nullptr;
}

bool GDScriptParser::is_valid_number(const std::string& str, bool& is_float) {
    is_float = str.find('.') != std::string::npos;
    
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (i == 0 && c == '-') continue; // Allow negative sign
        if (c == '.' && is_float) continue; // Allow decimal point
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return true;
}

bool GDScriptParser::parse_integer(const std::string& str, int64_t& out) {
    char* end = nullptr;
    int64_t val = std::strtoll(str.c_str(), &end, 10);
    if (end != str.c_str() && *end == '\0') {
        out = val;
        return true;
    }
    return false;
}

bool GDScriptParser::parse_float(const std::string& str, double& out) {
    char* end = nullptr;
    double val = std::strtod(str.c_str(), &end);
    if (end != str.c_str() && *end == '\0') {
        out = val;
        return true;
    }
    return false;
}

std::unique_ptr<LiteralExpr> GDScriptParser::make_literal(const Token& token) {
    std::unique_ptr<LiteralExpr> lit = std::make_unique<LiteralExpr>();
        
    // Parse literal value
    if (token.literal == "true") {
        lit->value = true;
    } else if (token.literal == "false") {
        lit->value = false;
    } else if (token.literal == "null") {
        lit->value = nullptr;
    } else {
        // Try to parse as number (without exceptions)
        bool is_float = false;
        bool is_number = is_valid_number(token.literal, is_float);
        
        if (is_number) {
            if (is_float) {
                double val;
                if (parse_float(token.literal, val)) {
                    lit->value = val;
                } else {
                    // String literal (parsing failed)
                    lit->value = token.literal;
                }
            } else {
                int64_t val;
                if (parse_integer(token.literal, val)) {
                    lit->value = val;
                } else {
                    // String literal (parsing failed)
                    lit->value = token.literal;
                }
            }
        } else {
            // String literal (already has quotes stripped)
            lit->value = token.literal;
        }
    }
    
    return lit;
}
    
std::unique_ptr<IdentifierExpr> GDScriptParser::make_identifier(const Token& token) {
    std::unique_ptr<IdentifierExpr> ident = std::make_unique<IdentifierExpr>();
    ident->name = token.literal;
    return ident;
}

std::unique_ptr<ProgramNode> GDScriptParser::parse(const std::string& source) {
    last_error_message.clear();
    _errors.clear();
    
    tokenizer.set_source(source);
    current = tokenizer.scan();
    previous = current;
    
    return parse_program();
                }

std::string GDScriptParser::getErrorMessage() const {
    return last_error_message;
}

} // namespace gdscript
