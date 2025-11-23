#include "gdscript_parser.h"
#include "peglib.h"
#include <sstream>
#include <cstring>
#include <algorithm>
#include <any>

namespace gdscript {

// Helper structures to store data during parsing
// (since AST nodes use unique_ptr which can't be stored in std::any)

// Binary operation data
struct BinaryOpData {
    std::shared_ptr<ExpressionNode> left;
    std::string op;
    std::shared_ptr<ExpressionNode> right;
};

// Variable declaration data
struct VarDeclData {
    std::string name;
    std::string typeHint;
    std::any initializer; // Expression as any (shared_ptr<ExpressionNode> or BinaryOpData)
};

// Function data
struct FunctionData {
    std::string name;
    std::vector<std::pair<std::string, std::string>> parameters;
    std::string returnType;
    std::vector<std::any> body; // Statements as any
};

// Program data
struct ProgramData {
    std::vector<std::any> functions; // FunctionData
    std::vector<std::any> statements; // StatementNode or other
};

// Basic GDScript grammar (PEG syntax)
// Starting with simple subset: functions, returns, integer literals, identifiers
// Note: GDScript uses indentation for blocks, but for now we'll use newlines
// TODO: Add proper indentation handling
static const char* gdscript_grammar = R"(
    Program         <- (Function / Statement)*
    
    Function        <- 'func' _ Identifier _ '(' _ Parameters? _ ')' _ (':' _ ReturnType)? _ ':' _ NEWLINE _ Body
    Parameters      <- Parameter (',' _ Parameter)*
    Parameter       <- Identifier (':' _ TypeHint)?
    ReturnType      <- TypeHint
    Body            <- Statement+
    
    Statement       <- ReturnStmt / VarDeclStmt / ExpressionStmt
    ReturnStmt      <- 'return' _ Expression? _ NEWLINE
    VarDeclStmt     <- 'var' _ Identifier _ (':' _ TypeHint)? _ ('=' _ Expression)? _ NEWLINE
    ExpressionStmt  <- Expression _ NEWLINE
    
    Expression      <- BinaryExpr / UnaryExpr / PrimaryExpr
    BinaryExpr      <- PrimaryExpr _ BinaryOp _ Expression
    UnaryExpr       <- UnaryOp _ PrimaryExpr
    PrimaryExpr     <- Literal / IdentifierExpr / '(' _ Expression _ ')'
    
    IdentifierExpr  <- Identifier
    Literal         <- IntegerLiteral / StringLiteral / BooleanLiteral / NullLiteral
    
    IntegerLiteral  <- < '-'? [0-9]+ >
    StringLiteral   <- '"' < (!'"' .)* > '"'
    BooleanLiteral  <- 'true' / 'false'
    NullLiteral     <- 'null'
    
    BinaryOp        <- '+' / '-' / '*' / '/' / '%' / '==' / '!=' / '<' / '>' / '<=' / '>='
    UnaryOp         <- '-' / '+' / '!' / 'not'
    
    TypeHint        <- Identifier ('.' Identifier)*
    Identifier      <- < [a-zA-Z_][a-zA-Z0-9_]* >
    
    NEWLINE         <- '\n' / '\r\n' / '\r'
    _               <- [ \t]*
)";

// Note: Removed leading/trailing _ from Program rule to avoid capturing whitespace in semantic values

GDScriptParser::GDScriptParser() : last_program_data() {
    // Create PEG parser
    peg::parser* p = new peg::parser(gdscript_grammar);
    
    if (!*p) {
        delete p;
        parser = nullptr;
        return;
    }
    
    // Set up semantic actions to build AST
    // Use shared_ptr for semantic values (can be stored in std::any)
    // Then convert to unique_ptr when building final AST
    
    // Integer literal
    (*p)["IntegerLiteral"] = [](const peg::SemanticValues& sv) -> std::any {
        std::string token = sv.token_to_string();
        int64_t value = std::stoll(token);
        auto lit = std::make_shared<LiteralExpr>();
        lit->value = value;
        return lit;
    };
    
    // String literal
    (*p)["StringLiteral"] = [](const peg::SemanticValues& sv) -> std::any {
        std::string value = sv.token_to_string();
        auto lit = std::make_shared<LiteralExpr>();
        lit->value = value;
        return lit;
    };
    
    // Boolean literal
    (*p)["BooleanLiteral"] = [](const peg::SemanticValues& sv) -> std::any {
        bool value = (sv.token_to_string() == "true");
        auto lit = std::make_shared<LiteralExpr>();
        lit->value = value;
        return lit;
    };
    
    // Null literal
    (*p)["NullLiteral"] = [](const peg::SemanticValues& sv) -> std::any {
        auto lit = std::make_shared<LiteralExpr>();
        lit->value = nullptr;
        return lit;
    };
    
    // Identifier
    (*p)["Identifier"] = [](const peg::SemanticValues& sv) -> std::any {
        return sv.token_to_string();
    };
    
    // Identifier expression
    (*p)["IdentifierExpr"] = [](const peg::SemanticValues& sv) -> std::any {
        std::string name = std::any_cast<std::string>(sv[0]);
        auto expr = std::make_shared<IdentifierExpr>();
        expr->name = name;
        return expr;
    };
    
    // Literal (dispatches to specific literal types)
    (*p)["Literal"] = [](const peg::SemanticValues& sv) -> std::any {
        return sv[0]; // Pass through the literal from child rule
    };
    
    // Primary expression
    (*p)["PrimaryExpr"] = [](const peg::SemanticValues& sv) -> std::any {
        if (sv.size() > 0) {
            return sv[0]; // Pass through expression or literal
        }
        return std::any();
    };
    
    // Binary operator - return operator string directly
    (*p)["BinaryOp"] = [](const peg::SemanticValues& sv) -> std::any {
        return sv.token_to_string();
    };
    
    // Binary expression: PrimaryExpr _ BinaryOp _ Expression
    // Store binary op info in a way that can be converted later
    (*p)["BinaryExpr"] = [](const peg::SemanticValues& sv) -> std::any {
        auto left = std::any_cast<std::shared_ptr<ExpressionNode>>(sv[0]);
        // BinaryOp is at sv[1] (after PrimaryExpr and whitespace)
        std::string op = std::any_cast<std::string>(sv[1]);
        auto right = std::any_cast<std::shared_ptr<ExpressionNode>>(sv[2]);
        
        BinaryOpData data;
        data.left = left;
        data.op = op;
        data.right = right;
        return data;
    };
    
    // Expression (dispatches to binary/unary/primary)
    // Need to handle BinaryOpData specially
    (*p)["Expression"] = [](const peg::SemanticValues& sv) -> std::any {
        // Check if it's BinaryOpData (from BinaryExpr) or regular ExpressionNode
        if (sv.size() > 0) {
            // Try to detect BinaryOpData - if it has a specific structure
            // For now, just pass through and handle in conversion
            return sv[0];
        }
        return std::any();
    };
    
    // Return statement
    // Grammar: 'return' _ Expression? _ NEWLINE
    // The Expression is optional. If present, it will be the only non-empty semantic value
    // (since 'return', whitespace, and NEWLINE don't produce semantic values)
    (*p)["ReturnStmt"] = [](const peg::SemanticValues& sv) -> std::any {
        // Find the first non-empty value - it should be the expression if present
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value()) {
                // This is the expression (if return has a value) or empty (if return has no value)
                // Return it directly - empty any means no return value
                return sv[i];
            }
        }
        // No expression found - return empty any (means return with no value)
        return std::any();
    };
    
    // Variable declaration statement
    // Grammar: 'var' _ Identifier _ (':' _ TypeHint)? _ ('=' _ Expression)? _ NEWLINE
    // Semantic values: [0] = Identifier (string), [1] = TypeHint (string, optional), [2] = Expression (optional)
    (*p)["VarDeclStmt"] = [](const peg::SemanticValues& sv) -> std::any {
        VarDeclData data;
        data.name = std::any_cast<std::string>(sv[0]);
        
        // Find type hint (if present) - it's a string that's not the identifier
        for (size_t i = 1; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::string)) {
                std::string str = std::any_cast<std::string>(sv[i]);
                if (str != data.name) {
                    data.typeHint = str;
                    break;
                }
            }
        }
        
        // Find initializer expression (if present) - it's a shared_ptr<ExpressionNode> or BinaryOpData
        for (size_t i = 1; i < sv.size(); ++i) {
            if (sv[i].has_value()) {
                std::string type_name = sv[i].type().name();
                // Check if it's an expression type
                if (type_name.find("shared_ptr") != std::string::npos && 
                    (type_name.find("ExpressionNode") != std::string::npos ||
                     type_name.find("LiteralExpr") != std::string::npos ||
                     type_name.find("IdentifierExpr") != std::string::npos ||
                     type_name.find("BinaryOpExpr") != std::string::npos)) {
                    data.initializer = sv[i];
                    break;
                } else if (type_name.find("BinaryOpData") != std::string::npos) {
                    data.initializer = sv[i];
                    break;
                }
            }
        }
        
        return data;
    };
    
    // Statement (dispatches to return/var/expression)
    // ReturnStmt returns std::any (expression), others return shared_ptr<StatementNode>
    (*p)["Statement"] = [](const peg::SemanticValues& sv) -> std::any {
        return sv[0]; // Pass through (could be std::any from ReturnStmt or shared_ptr<StatementNode>)
    };
    
    // Body (statements)
    // Statements could be std::any (from ReturnStmt) or shared_ptr<StatementNode>
    (*p)["Body"] = [](const peg::SemanticValues& sv) -> std::any {
        std::vector<std::any> statements;
        for (size_t i = 0; i < sv.size(); ++i) {
            statements.push_back(sv[i]);
        }
        return statements;
    };
    
    // Type hint - concatenate identifiers with dots
    (*p)["TypeHint"] = [](const peg::SemanticValues& sv) -> std::any {
        std::string result;
        for (size_t i = 0; i < sv.size(); ++i) {
            if (i > 0) result += ".";
            result += std::any_cast<std::string>(sv[i]);
        }
        return result;
    };
    
    // Return type - same as TypeHint
    (*p)["ReturnType"] = [](const peg::SemanticValues& sv) -> std::any {
        return sv[0]; // Pass through TypeHint
    };
    
    // Parameter
    (*p)["Parameter"] = [](const peg::SemanticValues& sv) -> std::any {
        std::string name = std::any_cast<std::string>(sv[0]);
        std::string typeHint;
        if (sv.size() > 1) {
            typeHint = std::any_cast<std::string>(sv[1]);
        }
        return std::make_pair(name, typeHint);
    };
    
    // Parameters
    (*p)["Parameters"] = [](const peg::SemanticValues& sv) -> std::any {
        std::vector<std::pair<std::string, std::string>> params;
        for (size_t i = 0; i < sv.size(); ++i) {
            params.push_back(std::any_cast<std::pair<std::string, std::string>>(sv[i]));
        }
        return params;
    };
    
    // Function
    // Grammar: 'func' _ Identifier _ '(' _ Parameters? _ ')' _ (':' _ ReturnType)? _ ':' _ NEWLINE _ Body
    // Semantic values structure:
    // - sv[1] = Identifier (string) - function name
    // - sv[?] = Parameters (vector<pair<string, string>>) - optional
    // - sv[?] = ReturnType (string) - optional
    // - sv[last] = Body (vector<any>) - statements
    (*p)["Function"] = [](const peg::SemanticValues& sv) -> std::any {
        FunctionData data;
        
        // Find Identifier (first string value)
        bool found_name = false;
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::string)) {
                try {
                    std::string str = std::any_cast<std::string>(sv[i]);
                    // Check if it's not a return type (return types are usually longer or have dots)
                    // For now, assume first string is the function name
                    if (!found_name) {
                        data.name = str;
                        found_name = true;
                    } else if (str != data.name) {
                        // This might be return type
                        data.returnType = str;
                    }
                } catch (...) {
                    continue;
                }
            }
        }
        
        if (data.name.empty()) {
            return FunctionData(); // Return empty if no name found
        }
        
        // Find Parameters (vector<pair<string, string>>)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::vector<std::pair<std::string, std::string>>)) {
                try {
                    data.parameters = std::any_cast<std::vector<std::pair<std::string, std::string>>>(sv[i]);
                    break;
                } catch (...) {
                    continue;
                }
            }
        }
        
        // Find Body (vector<any>) - should be the last non-empty value
        for (size_t i = sv.size(); i > 0; --i) {
            size_t idx = i - 1;
            if (sv[idx].has_value() && sv[idx].type() == typeid(std::vector<std::any>)) {
                try {
                    data.body = std::any_cast<std::vector<std::any>>(sv[idx]);
                    break;
                } catch (...) {
                    continue;
                }
            }
        }
        
        return data;
    };
    
    // Program
    // Grammar: (Function / Statement)*
    // Collect Functions and Statements
    // Note: cpp-peglib doesn't store the root rule's semantic value in parse result,
    // so we store it in the parser instance's last_program_data member as a workaround
    (*p)["Program"] = [this](const peg::SemanticValues& sv) -> std::any {
        ProgramData data;
        
        for (size_t i = 0; i < sv.size(); ++i) {
            if (!sv[i].has_value()) continue; // Skip empty values
            
            // Try to cast to FunctionData
            try {
                auto func_data = std::any_cast<FunctionData>(sv[i]);
                // Successfully cast - it's FunctionData
                data.functions.push_back(sv[i]);
                continue;
            } catch (const std::bad_any_cast& e) {
                // Not FunctionData, try StatementNode
            } catch (...) {
                // Other error, skip
                continue;
            }
            
            // Try to cast to StatementNode
            try {
                auto stmt = std::any_cast<std::shared_ptr<StatementNode>>(sv[i]);
                // Successfully cast - it's StatementNode
                data.statements.push_back(sv[i]);
                continue;
            } catch (const std::bad_any_cast& e) {
                // Not StatementNode either, skip
            } catch (...) {
                // Other error, skip
                continue;
            }
        }
        
        // Workaround: Store in parser instance member variable
        // cpp-peglib doesn't store the root rule's semantic value in parse result
        this->last_program_data = data;
        
        return std::any(data);
    };
    
    // Store parser pointer
    parser = p;
}

GDScriptParser::~GDScriptParser() {
    if (parser) {
        delete static_cast<peg::parser*>(parser);
    }
}

bool GDScriptParser::isValid() const {
    return parser != nullptr && static_cast<peg::parser*>(parser)->operator bool();
}

std::string GDScriptParser::getErrorMessage() const {
    // Return formatted error message from error collection if available
    if (errors.hasErrors()) {
        return errors.getFormattedMessage();
    }
    // Fallback to last_error_message for backward compatibility
    return last_error_message;
}

// Helper to convert shared_ptr ExpressionNode or BinaryOpData to unique_ptr
std::unique_ptr<ExpressionNode> convertExpression(const std::any& expr_any) {
    if (!expr_any.has_value()) {
        return nullptr;
    }
    
    // Check if it's BinaryOpData
    if (expr_any.type() == typeid(BinaryOpData)) {
        auto data = std::any_cast<BinaryOpData>(expr_any);
        auto result = std::make_unique<BinaryOpExpr>();
        result->left = convertExpression(data.left);
        result->op = data.op;
        result->right = convertExpression(data.right);
        return result;
    }
    
    // Otherwise, it could be shared_ptr<ExpressionNode> or shared_ptr<LiteralExpr>, etc.
    // Try to cast to shared_ptr<ExpressionNode> first (this works for any ExpressionNode subclass)
    try {
        // Try casting to shared_ptr<ExpressionNode> - this will work for LiteralExpr, IdentifierExpr, etc.
        auto expr = std::any_cast<std::shared_ptr<ExpressionNode>>(expr_any);
        if (!expr) {
            return nullptr;
        }
        
        // Handle different expression types
        if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
            auto result = std::make_unique<LiteralExpr>();
            result->value = lit->value; // Copy the variant
            return result;
        } else if (auto ident = std::dynamic_pointer_cast<IdentifierExpr>(expr)) {
            auto result = std::make_unique<IdentifierExpr>();
            result->name = ident->name;
            return result;
        }
    } catch (const std::bad_any_cast& e) {
        // Not a shared_ptr<ExpressionNode>, try other types
        try {
            auto lit = std::any_cast<std::shared_ptr<LiteralExpr>>(expr_any);
            auto result = std::make_unique<LiteralExpr>();
            result->value = lit->value;
            return result;
        } catch (const std::bad_any_cast& e2) {
            try {
                auto ident = std::any_cast<std::shared_ptr<IdentifierExpr>>(expr_any);
                auto result = std::make_unique<IdentifierExpr>();
                result->name = ident->name;
                return result;
            } catch (const std::bad_any_cast& e3) {
                // Unknown type
            }
        }
    }
    // TODO: Handle other expression types
    return nullptr;
}

// Overload for direct shared_ptr conversion
std::unique_ptr<ExpressionNode> convertExpression(std::shared_ptr<ExpressionNode> expr) {
    if (!expr) return nullptr;
    std::any expr_any = expr;
    return convertExpression(expr_any);
}

// Helper to convert shared_ptr StatementNode to unique_ptr
std::unique_ptr<StatementNode> convertStatement(std::shared_ptr<StatementNode> stmt) {
    if (!stmt) return nullptr;
    
    if (auto ret = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        auto result = std::make_unique<ReturnStatement>();
        // ret->value is unique_ptr in AST, but shared_ptr in semantic actions
        // This shouldn't happen - ReturnStatement should be created from expression any
        // For now, skip this case
        return result;
    } else if (auto var = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
        auto result = std::make_unique<VariableDeclaration>();
        result->name = var->name;
        result->typeHint = var->typeHint;
        // var->initializer is unique_ptr in AST, but shared_ptr in semantic actions
        // This shouldn't happen - VariableDeclaration should be created from VarDeclData
        // For now, skip initializer
        return result;
    }
    // TODO: Handle other statement types
    return nullptr;
}

// Helper to convert VarDeclData to unique_ptr VariableDeclaration
std::unique_ptr<VariableDeclaration> convertVarDecl(const std::any& varDecl_any) {
    if (!varDecl_any.has_value()) {
        return nullptr;
    }
    
    // Check if it's VarDeclData
    if (varDecl_any.type() == typeid(VarDeclData)) {
        auto data = std::any_cast<VarDeclData>(varDecl_any);
        auto result = std::make_unique<VariableDeclaration>();
        result->name = data.name;
        result->typeHint = data.typeHint;
        
        // Convert initializer expression if present
        if (data.initializer.has_value()) {
            result->initializer = convertExpression(data.initializer);
        }
        
        return result;
    }
    
    return nullptr;
}

std::unique_ptr<ProgramNode> GDScriptParser::parse(const std::string& source) {
    if (!isValid()) {
        last_error_message = "Parser initialization failed";
        errors.addError(ErrorType::Parse, "Parser initialization failed");
        return nullptr;
    }
    
    peg::parser* p = static_cast<peg::parser*>(parser);
    
    // Clear previous error
    last_error_message.clear();
    errors.clear();
    
    // Parse with semantic actions
    std::any result;
    bool success = p->parse(source, result);
    
    if (!success) {
        // Store a generic error message (cpp-peglib doesn't expose detailed error info easily)
        // We could enhance this by using a custom log callback, but for now use a simple message
        last_error_message = "Parse error: Failed to parse GDScript source code";
        errors.addError(ErrorType::Parse, "Failed to parse GDScript source code");
        return nullptr;
    }
    
    // Workaround: cpp-peglib doesn't store the root rule's semantic value in parse result
    // Use the member variable that was set by the Program semantic action
    ProgramData program_data = this->last_program_data;
    auto program = std::make_unique<ProgramNode>();
    
    // Convert functions
    for (const auto& func_any : program_data.functions) {
        try {
            auto func_data = std::any_cast<FunctionData>(func_any);
            auto func = std::make_unique<FunctionNode>();
            func->name = func_data.name;
            func->parameters = func_data.parameters;
            func->returnType = func_data.returnType;
            
            // Convert body statements
            for (const auto& stmt_any : func_data.body) {
                std::unique_ptr<StatementNode> stmt;
                
                // Check if it's VarDeclData
                if (stmt_any.type() == typeid(VarDeclData)) {
                    stmt = convertVarDecl(stmt_any);
                }
                // Check if it's a ReturnStmt (just expression any)
                // ReturnStmt returns std::any (expression), so if it's not shared_ptr<StatementNode>, it's ReturnStmt
                else if (stmt_any.type() != typeid(std::shared_ptr<StatementNode>)) {
                    // It's a ReturnStmt - create ReturnStatement with the expression
                    auto ret_stmt = std::make_unique<ReturnStatement>();
                    ret_stmt->value = convertExpression(stmt_any);
                    stmt = std::move(ret_stmt);
                } else {
                    // It's a shared_ptr<StatementNode>
                    auto shared_stmt = std::any_cast<std::shared_ptr<StatementNode>>(stmt_any);
                    stmt = convertStatement(shared_stmt);
                }
                
                if (stmt) {
                    func->body.push_back(std::move(stmt));
                }
            }
            
            program->functions.push_back(std::move(func));
        } catch (const std::exception& e) {
            // Skip invalid function data
            continue;
        } catch (...) {
            // Skip invalid function data
            continue;
        }
    }
    
    // Convert top-level statements
    for (const auto& stmt_any : program_data.statements) {
        try {
            std::unique_ptr<StatementNode> stmt;
            
            // Check if it's VarDeclData
            if (stmt_any.type() == typeid(VarDeclData)) {
                stmt = convertVarDecl(stmt_any);
            }
            // Check if it's a shared_ptr<StatementNode>
            else if (stmt_any.type() == typeid(std::shared_ptr<StatementNode>)) {
                auto shared_stmt = std::any_cast<std::shared_ptr<StatementNode>>(stmt_any);
                stmt = convertStatement(shared_stmt);
            }
            
            if (stmt) {
                program->statements.push_back(std::move(stmt));
            }
        } catch (...) {
            // Skip invalid statement
            continue;
        }
    }
    
    return program;
}

} // namespace gdscript
