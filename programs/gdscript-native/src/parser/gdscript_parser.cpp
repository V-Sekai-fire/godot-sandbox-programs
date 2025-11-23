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
    std::string type_hint;
    std::any initializer; // Expression as any (shared_ptr<ExpressionNode> or BinaryOpData)
};

// Assignment statement data
struct AssignmentData {
    std::shared_ptr<ExpressionNode> target; // IdentifierExpr
    std::any value; // Expression as any
};

// If statement data
struct IfStmtData {
    std::any condition; // Expression
    std::vector<std::any> then_body; // Statements
    std::vector<std::pair<std::any, std::vector<std::any>>> elif_branches; // (condition, body)
    std::vector<std::any> else_body; // Statements (optional)
};

// For statement data
struct ForStmtData {
    std::string variable_name;
    std::any iterable; // Expression
    std::vector<std::any> body; // Statements
};

// While statement data
struct WhileStmtData {
    std::any condition; // Expression
    std::vector<std::any> body; // Statements
};

// Match statement data
struct MatchStmtData {
    std::any expression; // Expression
    std::vector<std::pair<std::any, std::vector<std::any>>> branches; // (pattern, body)
};

// Function data
struct FunctionData {
    std::string name;
    std::vector<std::pair<std::string, std::string>> parameters;
    std::string return_type;
    std::vector<std::any> body; // Statements as any
};

// Program data
struct ProgramData {
    std::vector<std::any> functions; // FunctionData
    std::vector<std::any> statements; // StatementNode or other
};

// Helper function to count indentation (spaces/tabs)
static int count_indent(const std::string& line) {
    int indent = 0;
    for (char c : line) {
        if (c == ' ') {
            indent++;
        } else if (c == '\t') {
            indent += 4; // Convert tabs to 4 spaces
        } else {
            break;
        }
    }
    return indent;
}

// Pre-process source to convert indentation to explicit block markers
// This is necessary because PEG parsers don't natively support indentation-sensitive syntax
// The standard approach for Python-like languages is to convert indentation to explicit markers
static std::string preprocess_indentation(const std::string& source) {
    std::istringstream iss(source);
    std::ostringstream oss;
    std::vector<int> indent_stack = {0};
    std::string line;
    bool first_line = true;
    bool expecting_block = false;
    int block_indent = 0;
    
    while (std::getline(iss, line)) {
        // Count leading spaces/tabs
        int indent = count_indent(line);
        
        // Handle dedent (close blocks)
        while (indent_stack.size() > 1 && indent_stack.back() > indent) {
            oss << "}\n"; // Close block
            indent_stack.pop_back();
        }
        
        // Check if line ends with ':' (block start)
        // Trim trailing whitespace to check
        std::string trimmed = line;
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) {
            trimmed.pop_back();
        }
        
        if (!trimmed.empty() && trimmed.back() == ':') {
            // This is a block start (if, elif, else, for, while, match, func, etc.)
            // Remove trailing colon and whitespace, add it back with opening brace
            std::string line_without_colon = trimmed;
            if (!line_without_colon.empty()) {
                line_without_colon.pop_back(); // Remove ':'
            }
            // Trim trailing whitespace from line_without_colon
            while (!line_without_colon.empty() && (line_without_colon.back() == ' ' || line_without_colon.back() == '\t')) {
                line_without_colon.pop_back();
            }
            oss << line_without_colon << ":\n"; // Keep original line structure
            // Add opening brace on same line or next line (grammar allows whitespace)
            oss << "{\n";
            // Next line should be indented more
            block_indent = indent;
            indent_stack.push_back(indent + 4); // Expect at least 4 spaces more
            expecting_block = true;
        } else {
            // Regular line - preserve indentation
            oss << line << "\n";
            expecting_block = false;
        }
        first_line = false;
    }
    
    // Close remaining blocks
    while (indent_stack.size() > 1) {
        oss << "}\n";
        indent_stack.pop_back();
    }
    
    return oss.str();
}

// GDScript grammar - indentation handled via preprocessing to braces
// This is the standard approach for indentation-sensitive languages with PEG parsers
// The preprocessing step converts indentation to explicit block markers ({})
static const char* gdscript_grammar = R"(
    Program         <- (Function / Statement)*
    
    Function        <- 'func' _ Identifier _ '(' _ Parameters? _ ')' _ (':' _ ReturnType)? _ ':' _ NEWLINE Body
    Parameters      <- Parameter (',' _ Parameter)*
    Parameter       <- Identifier (':' _ TypeHint)?
    ReturnType      <- TypeHint
    Body            <- '{' WS Statement+ WS '}'
    
    Statement       <- ReturnStmt / VarDeclStmt / AssignmentStmt / IfStmt / ForStmt / WhileStmt / MatchStmt / ExpressionStmt
    ReturnStmt      <- 'return' _ Expression? _ NEWLINE
    VarDeclStmt     <- 'var' _ Identifier _ (':' _ TypeHint)? _ ('=' _ Expression)? _ NEWLINE
    AssignmentStmt  <- IdentifierExpr _ '=' _ Expression _ NEWLINE
    IfStmt          <- 'if' _ Expression _ ':' _ NEWLINE Body (ElifBranch)* (ElseBranch)?
    ElifBranch      <- 'elif' _ Expression _ ':' _ NEWLINE Body
    ElseBranch      <- 'else' _ ':' _ NEWLINE Body
    ForStmt         <- 'for' _ Identifier _ 'in' _ Expression _ ':' _ NEWLINE Body
    WhileStmt       <- 'while' _ Expression _ ':' _ NEWLINE Body
    MatchStmt       <- 'match' _ Expression _ ':' _ NEWLINE MatchBranches
    MatchBranches   <- MatchBranch+
    MatchBranch     <- Pattern _ ':' _ NEWLINE Body
    Pattern         <- Literal / Identifier / '_'
    ExpressionStmt  <- Expression _ NEWLINE
    
    Expression      <- BinaryExpr / UnaryExpr / PrimaryExpr
    BinaryExpr      <- PrimaryExpr _ BinaryOp _ Expression
    UnaryExpr       <- UnaryOp _ PrimaryExpr
    PrimaryExpr     <- MemberAccessExpr / CallExpr / Literal / IdentifierExpr / '(' _ Expression _ ')'
    MemberAccessExpr <- (CallExpr / Literal / IdentifierExpr / '(' _ Expression _ ')') _ '.' _ Identifier
    
    CallExpr        <- IdentifierExpr _ '(' _ Arguments? _ ')'
    Arguments       <- Expression (',' _ Expression)*
    
    IdentifierExpr  <- Identifier
    Literal         <- IntegerLiteral / StringLiteral / BooleanLiteral / NullLiteral
    
    IntegerLiteral  <- < '-'? [0-9]+ >
    StringLiteral   <- '"' < (!'"' .)* > '"'
    BooleanLiteral  <- 'true' / 'false'
    NullLiteral     <- 'null'
    
    BinaryOp        <- '+' / '-' / '*' / '/' / '%' / '==' / '!=' / '<' / '>' / '<=' / '>='
    UnaryOp         <- '-' / '+' / '!' / 'not'
    
    TypeHint        <- Identifier ('.' Identifier)*
    Identifier      <- < [a-zA-Z_\u0080-\uFFFF] [a-zA-Z0-9_\u0080-\uFFFF]* >
    
    NEWLINE         <- '\n' / '\r\n' / '\r'
    WS              <- [ \t\n\r]*
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
    
    // String literal (supports Unicode via UTF-8)
    // GDScript strings are UTF-8 encoded, and std::string can hold UTF-8 bytes
    (*p)["StringLiteral"] = [](const peg::SemanticValues& sv) -> std::any {
        // token_to_string() returns UTF-8 encoded string
        // std::string in C++ can hold UTF-8 bytes (it's just a byte sequence)
        std::string value = sv.token_to_string();
        auto lit = std::make_shared<LiteralExpr>();
        lit->value = value; // Store as UTF-8 encoded std::string
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
    
    // Identifier (supports Unicode via UTF-8)
    // GDScript allows Unicode characters in identifiers (e.g., Japanese, Chinese, etc.)
    // The token is UTF-8 encoded, and std::string can hold UTF-8 bytes
    (*p)["Identifier"] = [](const peg::SemanticValues& sv) -> std::any {
        // token_to_string() returns UTF-8 encoded string
        // std::string stores UTF-8 bytes (it's byte-oriented, not character-oriented)
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
    
    // Function call expression: IdentifierExpr _ '(' _ Arguments? _ ')'
    // sv[0] = IdentifierExpr (shared_ptr<IdentifierExpr>)
    // sv[1] = Arguments (optional, vector of expressions)
    (*p)["CallExpr"] = [](const peg::SemanticValues& sv) -> std::any {
        auto call = std::make_shared<CallExpr>();
        
        // Get callee (IdentifierExpr)
        if (sv.size() > 0 && sv[0].has_value()) {
            try {
                auto ident = std::any_cast<std::shared_ptr<IdentifierExpr>>(sv[0]);
                // Convert shared_ptr<IdentifierExpr> to unique_ptr<IdentifierExpr>
                auto callee = std::make_unique<IdentifierExpr>();
                callee->name = ident->name;
                call->callee = std::move(callee);
            } catch (const std::bad_any_cast&) {
                // Try as string (direct identifier)
                try {
                    std::string name = std::any_cast<std::string>(sv[0]);
                    auto callee = std::make_unique<IdentifierExpr>();
                    callee->name = name;
                    call->callee = std::move(callee);
                } catch (const std::bad_any_cast&) {
                    // Ignore - callee will be null
                }
            }
        }
        
        // Get arguments (if present)
        // Arguments are stored as a vector in sv[1] if present
        // For now, skip arguments - stub implementation
        // TODO: Parse arguments properly
        
        return call;
    };
    
    // Arguments: Expression (',' _ Expression)*
    // For now, just return the first expression (stub)
    (*p)["Arguments"] = [](const peg::SemanticValues& sv) -> std::any {
        // Return first expression as placeholder
        if (sv.size() > 0 && sv[0].has_value()) {
            return sv[0];
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
                    data.type_hint = str;
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
    
    // ElifBranch
    // Grammar: 'elif' _ Expression _ ':' _ NEWLINE _ Body
    (*p)["ElifBranch"] = [](const peg::SemanticValues& sv) -> std::any {
        std::pair<std::any, std::vector<std::any>> branch;
        // Find expression (first non-empty value that's not a vector)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() != typeid(std::vector<std::any>)) {
                branch.first = sv[i];
                break;
            }
        }
        // Find body (vector<any>)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::vector<std::any>)) {
                branch.second = std::any_cast<std::vector<std::any>>(sv[i]);
                break;
            }
        }
        return branch;
    };
    
    // ElseBranch
    // Grammar: 'else' _ ':' _ NEWLINE _ Body
    (*p)["ElseBranch"] = [](const peg::SemanticValues& sv) -> std::any {
        // Find body (vector<any>)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::vector<std::any>)) {
                return sv[i];
            }
        }
        return std::vector<std::any>();
    };
    
    // IfStmt
    // Grammar: 'if' _ Expression _ ':' _ NEWLINE _ Body (ElifBranch)* (ElseBranch)?
    (*p)["IfStmt"] = [](const peg::SemanticValues& sv) -> std::any {
        IfStmtData data;
        
        // Find condition (first non-empty value that's not a vector)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() != typeid(std::vector<std::any>) &&
                sv[i].type() != typeid(std::pair<std::any, std::vector<std::any>>)) {
                data.condition = sv[i];
                break;
            }
        }
        
        // Find then body (first vector<any>)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::vector<std::any>)) {
                data.then_body = std::any_cast<std::vector<std::any>>(sv[i]);
                break;
            }
        }
        
        // Find elif branches (pair<any, vector<any>>)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::pair<std::any, std::vector<std::any>>)) {
                data.elif_branches.push_back(std::any_cast<std::pair<std::any, std::vector<std::any>>>(sv[i]));
            }
        }
        
        // Find else body (last vector<any> that's not in elif)
        // This is a simplification - we'll use the last vector that's not in elif
        size_t last_vector_idx = sv.size();
        for (size_t i = sv.size(); i > 0; --i) {
            size_t idx = i - 1;
            if (sv[idx].has_value() && sv[idx].type() == typeid(std::vector<std::any>)) {
                // Check if it's not the then body or an elif body
                if (idx > 0) { // Rough check - could be improved
                    data.else_body = std::any_cast<std::vector<std::any>>(sv[idx]);
                    break;
                }
            }
        }
        
        return data;
    };
    
    // ForStmt
    // Grammar: 'for' _ Identifier _ 'in' _ Expression _ ':' _ NEWLINE _ Body
    (*p)["ForStmt"] = [](const peg::SemanticValues& sv) -> std::any {
        ForStmtData data;
        
        // Find variable name (string from Identifier)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::string)) {
                data.variable_name = std::any_cast<std::string>(sv[i]);
                break;
            }
        }
        
        // Find iterable expression (first non-string, non-vector value)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() != typeid(std::string) && 
                sv[i].type() != typeid(std::vector<std::any>)) {
                data.iterable = sv[i];
                break;
            }
        }
        
        // Find body (vector<any>)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::vector<std::any>)) {
                data.body = std::any_cast<std::vector<std::any>>(sv[i]);
                break;
            }
        }
        
        return data;
    };
    
    // WhileStmt
    // Grammar: 'while' _ Expression _ ':' _ NEWLINE _ Body
    (*p)["WhileStmt"] = [](const peg::SemanticValues& sv) -> std::any {
        WhileStmtData data;
        
        // Find condition (first non-vector value)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() != typeid(std::vector<std::any>)) {
                data.condition = sv[i];
                break;
            }
        }
        
        // Find body (vector<any>)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::vector<std::any>)) {
                data.body = std::any_cast<std::vector<std::any>>(sv[i]);
                break;
            }
        }
        
        return data;
    };
    
    // MatchStmt
    // Grammar: 'match' _ Expression _ ':' _ NEWLINE _ MatchBranches
    (*p)["MatchStmt"] = [](const peg::SemanticValues& sv) -> std::any {
        MatchStmtData data;
        
        // Find expression (first non-vector value)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() != typeid(std::vector<std::pair<std::any, std::vector<std::any>>>)) {
                data.expression = sv[i];
                break;
            }
        }
        
        // Find branches (vector of pairs)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::vector<std::pair<std::any, std::vector<std::any>>>)) {
                data.branches = std::any_cast<std::vector<std::pair<std::any, std::vector<std::any>>>>(sv[i]);
                break;
            }
        }
        
        return data;
    };
    
    // MatchBranch
    // Grammar: Pattern _ ':' _ NEWLINE _ Body
    (*p)["MatchBranch"] = [](const peg::SemanticValues& sv) -> std::any {
        std::pair<std::any, std::vector<std::any>> branch;
        
        // Find pattern (first non-vector value)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() != typeid(std::vector<std::any>)) {
                branch.first = sv[i];
                break;
            }
        }
        
        // Find body (vector<any>)
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::vector<std::any>)) {
                branch.second = std::any_cast<std::vector<std::any>>(sv[i]);
                break;
            }
        }
        
        return branch;
    };
    
    // MatchBranches
    (*p)["MatchBranches"] = [](const peg::SemanticValues& sv) -> std::any {
        std::vector<std::pair<std::any, std::vector<std::any>>> branches;
        for (size_t i = 0; i < sv.size(); ++i) {
            if (sv[i].has_value() && sv[i].type() == typeid(std::pair<std::any, std::vector<std::any>>)) {
                branches.push_back(std::any_cast<std::pair<std::any, std::vector<std::any>>>(sv[i]));
            }
        }
        return branches;
    };
    
    // Pattern (Literal / Identifier / '_')
    (*p)["Pattern"] = [](const peg::SemanticValues& sv) -> std::any {
        if (sv.size() > 0) {
            return sv[0]; // Pass through literal, identifier, or '_'
        }
        return std::any();
    };
    
    // MemberAccessExpr
    // Grammar: (CallExpr / Literal / IdentifierExpr / '(' _ Expression _ ')') _ '.' _ Identifier
    (*p)["MemberAccessExpr"] = [](const peg::SemanticValues& sv) -> std::any {
        auto member = std::make_shared<MemberAccessExpr>();
        
        // Get object (first value)
        if (sv.size() > 0 && sv[0].has_value()) {
            // Convert to expression - this is a stub, proper conversion needed
            // For now, just create an IdentifierExpr as placeholder
            member->object = std::make_unique<IdentifierExpr>();
            static_cast<IdentifierExpr*>(member->object.get())->name = "obj"; // Placeholder
        }
        
        // Get member name (last value should be string from Identifier)
        if (sv.size() > 1 && sv[sv.size() - 1].has_value()) {
            try {
                member->member = std::any_cast<std::string>(sv[sv.size() - 1]);
            } catch (const std::bad_any_cast&) {
                member->member = ""; // Placeholder
            }
        }
        
        return member;
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
        std::string type_hint;
        if (sv.size() > 1) {
            type_hint = std::any_cast<std::string>(sv[1]);
        }
        return std::make_pair(name, type_hint);
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
                        data.return_type = str;
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

bool GDScriptParser::is_valid() const {
    return parser != nullptr && static_cast<peg::parser*>(parser)->operator bool();
}

std::string GDScriptParser::getErrorMessage() const {
    // Return formatted error message from error collection if available
    if (_errors.has_errors()) {
        return _errors.get_formatted_message();
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
                // Try CallExpr
                try {
                    auto call = std::any_cast<std::shared_ptr<CallExpr>>(expr_any);
                    auto result = std::make_unique<CallExpr>();
                    // Convert callee (call->callee is unique_ptr, but we need to clone it)
                    if (call->callee) {
                        // Check if callee is IdentifierExpr
                        if (auto ident = dynamic_cast<IdentifierExpr*>(call->callee.get())) {
                            auto callee = std::make_unique<IdentifierExpr>();
                            callee->name = ident->name;
                            result->callee = std::move(callee);
                        } else {
                            // For other expression types, recursively convert
                            // But since call->callee is already unique_ptr, we need to clone it
                            // For now, just handle IdentifierExpr
                        }
                    }
                    // Convert arguments (for now, skip - stub implementation)
                    // TODO: Convert arguments properly
                    return result;
                } catch (const std::bad_any_cast& e4) {
                    // Try MemberAccessExpr
                    try {
                        auto member = std::any_cast<std::shared_ptr<MemberAccessExpr>>(expr_any);
                        auto result = std::make_unique<MemberAccessExpr>();
                        result->member = member->member;
                        // Convert object (stub - proper conversion needed)
                        if (member->object) {
                            // For now, create placeholder IdentifierExpr
                            result->object = std::make_unique<IdentifierExpr>();
                            static_cast<IdentifierExpr*>(result->object.get())->name = "obj";
                        }
                        return result;
                    } catch (const std::bad_any_cast& e5) {
                        // Unknown type
                    }
                }
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

// Forward declarations
std::unique_ptr<VariableDeclaration> convertVarDecl(const std::any& varDecl_any);
std::unique_ptr<AssignmentStatement> convertAssignment(const std::any& assign_any);
std::unique_ptr<StatementNode> convertStatement(std::shared_ptr<StatementNode> stmt);

// Helper to convert AssignmentData to unique_ptr AssignmentStatement
std::unique_ptr<AssignmentStatement> convertAssignment(const std::any& assign_any) {
    if (!assign_any.has_value()) {
        return nullptr;
    }
    
    // Check if it's AssignmentData
    if (assign_any.type() == typeid(AssignmentData)) {
        auto data = std::any_cast<AssignmentData>(assign_any);
        auto result = std::make_unique<AssignmentStatement>();
        
        // Convert target (IdentifierExpr)
        if (data.target) {
            if (auto ident = std::dynamic_pointer_cast<IdentifierExpr>(data.target)) {
                auto target = std::make_unique<IdentifierExpr>();
                target->name = ident->name;
                result->target = std::move(target);
            }
        }
        
        // Convert value expression
        if (data.value.has_value()) {
            result->value = convertExpression(data.value);
        }
        
        result->op = "="; // Default assignment operator
        
        return result;
    }
    
    return nullptr;
}

// Helper to convert IfStmtData to unique_ptr IfStatement
std::unique_ptr<IfStatement> convertIfStmt(const std::any& if_any) {
    if (!if_any.has_value()) {
        return nullptr;
    }
    
    if (if_any.type() == typeid(IfStmtData)) {
        auto data = std::any_cast<IfStmtData>(if_any);
        auto result = std::make_unique<IfStatement>();
        
        // Convert condition
        if (data.condition.has_value()) {
            result->condition = convertExpression(data.condition);
        }
        
        // Convert then body
        for (const auto& stmt_any : data.then_body) {
            std::unique_ptr<StatementNode> stmt;
            if (stmt_any.type() == typeid(VarDeclData)) {
                stmt = convertVarDecl(stmt_any);
            } else if (stmt_any.type() == typeid(AssignmentData)) {
                stmt = convertAssignment(stmt_any);
            } else if (stmt_any.type() != typeid(std::shared_ptr<StatementNode>)) {
                // ReturnStmt
                auto ret_stmt = std::make_unique<ReturnStatement>();
                ret_stmt->value = convertExpression(stmt_any);
                stmt = std::move(ret_stmt);
            } else {
                auto shared_stmt = std::any_cast<std::shared_ptr<StatementNode>>(stmt_any);
                stmt = convertStatement(shared_stmt);
            }
            if (stmt) {
                result->then_body.push_back(std::move(stmt));
            }
        }
        
        // Convert elif branches
        for (const auto& branch : data.elif_branches) {
            std::unique_ptr<ExpressionNode> cond = convertExpression(branch.first);
            std::vector<std::unique_ptr<StatementNode>> body;
            for (const auto& stmt_any : branch.second) {
                std::unique_ptr<StatementNode> stmt;
                if (stmt_any.type() == typeid(VarDeclData)) {
                    stmt = convertVarDecl(stmt_any);
                } else if (stmt_any.type() == typeid(AssignmentData)) {
                    stmt = convertAssignment(stmt_any);
                } else if (stmt_any.type() != typeid(std::shared_ptr<StatementNode>)) {
                    auto ret_stmt = std::make_unique<ReturnStatement>();
                    ret_stmt->value = convertExpression(stmt_any);
                    stmt = std::move(ret_stmt);
                } else {
                    auto shared_stmt = std::any_cast<std::shared_ptr<StatementNode>>(stmt_any);
                    stmt = convertStatement(shared_stmt);
                }
                if (stmt) {
                    body.push_back(std::move(stmt));
                }
            }
            result->elif_branches.push_back({std::move(cond), std::move(body)});
        }
        
        // Convert else body
        for (const auto& stmt_any : data.else_body) {
            std::unique_ptr<StatementNode> stmt;
            if (stmt_any.type() == typeid(VarDeclData)) {
                stmt = convertVarDecl(stmt_any);
            } else if (stmt_any.type() == typeid(AssignmentData)) {
                stmt = convertAssignment(stmt_any);
            } else if (stmt_any.type() != typeid(std::shared_ptr<StatementNode>)) {
                auto ret_stmt = std::make_unique<ReturnStatement>();
                ret_stmt->value = convertExpression(stmt_any);
                stmt = std::move(ret_stmt);
            } else {
                auto shared_stmt = std::any_cast<std::shared_ptr<StatementNode>>(stmt_any);
                stmt = convertStatement(shared_stmt);
            }
            if (stmt) {
                result->else_body.push_back(std::move(stmt));
            }
        }
        
        return result;
    }
    
    return nullptr;
}

// Helper to convert ForStmtData to unique_ptr ForStatement
std::unique_ptr<ForStatement> convertForStmt(const std::any& for_any) {
    if (!for_any.has_value()) {
        return nullptr;
    }
    
    if (for_any.type() == typeid(ForStmtData)) {
        auto data = std::any_cast<ForStmtData>(for_any);
        auto result = std::make_unique<ForStatement>();
        result->variable_name = data.variable_name;
        result->iterable = convertExpression(data.iterable);
        
        // Convert body
        for (const auto& stmt_any : data.body) {
            std::unique_ptr<StatementNode> stmt;
            if (stmt_any.type() == typeid(VarDeclData)) {
                stmt = convertVarDecl(stmt_any);
            } else if (stmt_any.type() == typeid(AssignmentData)) {
                stmt = convertAssignment(stmt_any);
            } else if (stmt_any.type() != typeid(std::shared_ptr<StatementNode>)) {
                auto ret_stmt = std::make_unique<ReturnStatement>();
                ret_stmt->value = convertExpression(stmt_any);
                stmt = std::move(ret_stmt);
            } else {
                auto shared_stmt = std::any_cast<std::shared_ptr<StatementNode>>(stmt_any);
                stmt = convertStatement(shared_stmt);
            }
            if (stmt) {
                result->body.push_back(std::move(stmt));
            }
        }
        
        return result;
    }
    
    return nullptr;
}

// Helper to convert WhileStmtData to unique_ptr WhileStatement
std::unique_ptr<WhileStatement> convertWhileStmt(const std::any& while_any) {
    if (!while_any.has_value()) {
        return nullptr;
    }
    
    if (while_any.type() == typeid(WhileStmtData)) {
        auto data = std::any_cast<WhileStmtData>(while_any);
        auto result = std::make_unique<WhileStatement>();
        result->condition = convertExpression(data.condition);
        
        // Convert body
        for (const auto& stmt_any : data.body) {
            std::unique_ptr<StatementNode> stmt;
            if (stmt_any.type() == typeid(VarDeclData)) {
                stmt = convertVarDecl(stmt_any);
            } else if (stmt_any.type() == typeid(AssignmentData)) {
                stmt = convertAssignment(stmt_any);
            } else if (stmt_any.type() != typeid(std::shared_ptr<StatementNode>)) {
                auto ret_stmt = std::make_unique<ReturnStatement>();
                ret_stmt->value = convertExpression(stmt_any);
                stmt = std::move(ret_stmt);
            } else {
                auto shared_stmt = std::any_cast<std::shared_ptr<StatementNode>>(stmt_any);
                stmt = convertStatement(shared_stmt);
            }
            if (stmt) {
                result->body.push_back(std::move(stmt));
            }
        }
        
        return result;
    }
    
    return nullptr;
}

// Helper to convert MatchStmtData to unique_ptr MatchStatement
std::unique_ptr<MatchStatement> convertMatchStmt(const std::any& match_any) {
    if (!match_any.has_value()) {
        return nullptr;
    }
    
    if (match_any.type() == typeid(MatchStmtData)) {
        auto data = std::any_cast<MatchStmtData>(match_any);
        auto result = std::make_unique<MatchStatement>();
        result->expression = convertExpression(data.expression);
        
        // Convert branches
        for (const auto& branch : data.branches) {
            std::unique_ptr<ExpressionNode> pattern = convertExpression(branch.first);
            std::vector<std::unique_ptr<StatementNode>> body;
            for (const auto& stmt_any : branch.second) {
                std::unique_ptr<StatementNode> stmt;
                if (stmt_any.type() == typeid(VarDeclData)) {
                    stmt = convertVarDecl(stmt_any);
                } else if (stmt_any.type() == typeid(AssignmentData)) {
                    stmt = convertAssignment(stmt_any);
                } else if (stmt_any.type() != typeid(std::shared_ptr<StatementNode>)) {
                    auto ret_stmt = std::make_unique<ReturnStatement>();
                    ret_stmt->value = convertExpression(stmt_any);
                    stmt = std::move(ret_stmt);
                } else {
                    auto shared_stmt = std::any_cast<std::shared_ptr<StatementNode>>(stmt_any);
                    stmt = convertStatement(shared_stmt);
                }
                if (stmt) {
                    body.push_back(std::move(stmt));
                }
            }
            result->branches.push_back({std::move(pattern), std::move(body)});
        }
        
        return result;
    }
    
    return nullptr;
}

// Helper to convert shared_ptr StatementNode to unique_ptr
std::unique_ptr<StatementNode> convertStatement(std::shared_ptr<StatementNode> stmt) {
    if (!stmt) return nullptr;
    
    if (auto assign = std::dynamic_pointer_cast<AssignmentStatement>(stmt)) {
        auto result = std::make_unique<AssignmentStatement>();
        result->op = assign->op;
        // Note: target and value conversion would need the original any values
        // This is a limitation - we'll handle it differently
        return result;
    }
    
    if (auto ret = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        auto result = std::make_unique<ReturnStatement>();
        // ret->value is unique_ptr in AST, but shared_ptr in semantic actions
        // This shouldn't happen - ReturnStatement should be created from expression any
        // For now, skip this case
        return result;
    } else if (auto var = std::dynamic_pointer_cast<VariableDeclaration>(stmt)) {
        auto result = std::make_unique<VariableDeclaration>();
        result->name = var->name;
        result->type_hint = var->type_hint;
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
        result->type_hint = data.type_hint;
        
        // Convert initializer expression if present
        if (data.initializer.has_value()) {
            result->initializer = convertExpression(data.initializer);
        }
        
        return result;
    }
    
    return nullptr;
}

std::unique_ptr<ProgramNode> GDScriptParser::parse(const std::string& source) {
    if (!is_valid()) {
        last_error_message = "Parser initialization failed";
        _errors.add_error(ErrorType::Parse, "Parser initialization failed");
        return nullptr;
    }
    
    peg::parser* p = static_cast<peg::parser*>(parser);
    
    // Clear previous error
    last_error_message.clear();
    _errors.clear();
    
    // Preprocess indentation to braces (standard approach for indentation-sensitive languages)
    // This converts GDScript's indentation-based blocks to explicit { } markers
    // The PEG parser then parses the preprocessed source normally
    std::string processed_source = preprocess_indentation(source);
    
    std::any result;
    bool success = p->parse(processed_source, result);
    
    if (!success) {
        // Store a generic error message (cpp-peglib doesn't expose detailed error info easily)
        // We could enhance this by using a custom log callback, but for now use a simple message
        last_error_message = "Parse error: Failed to parse GDScript source code";
        _errors.add_error(ErrorType::Parse, "Failed to parse GDScript source code");
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
            func->return_type = func_data.return_type;
            
            // Convert body statements
            for (const auto& stmt_any : func_data.body) {
                std::unique_ptr<StatementNode> stmt;
                
                // Check if it's VarDeclData
                if (stmt_any.type() == typeid(VarDeclData)) {
                    stmt = convertVarDecl(stmt_any);
                }
                // Check if it's AssignmentData
                else if (stmt_any.type() == typeid(AssignmentData)) {
                    stmt = convertAssignment(stmt_any);
                }
                // Check if it's IfStmtData
                else if (stmt_any.type() == typeid(IfStmtData)) {
                    stmt = convertIfStmt(stmt_any);
                }
                // Check if it's ForStmtData
                else if (stmt_any.type() == typeid(ForStmtData)) {
                    stmt = convertForStmt(stmt_any);
                }
                // Check if it's WhileStmtData
                else if (stmt_any.type() == typeid(WhileStmtData)) {
                    stmt = convertWhileStmt(stmt_any);
                }
                // Check if it's MatchStmtData
                else if (stmt_any.type() == typeid(MatchStmtData)) {
                    stmt = convertMatchStmt(stmt_any);
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
            // Check if it's AssignmentData
            else if (stmt_any.type() == typeid(AssignmentData)) {
                stmt = convertAssignment(stmt_any);
            }
            // Check if it's IfStmtData
            else if (stmt_any.type() == typeid(IfStmtData)) {
                stmt = convertIfStmt(stmt_any);
            }
            // Check if it's ForStmtData
            else if (stmt_any.type() == typeid(ForStmtData)) {
                stmt = convertForStmt(stmt_any);
            }
            // Check if it's WhileStmtData
            else if (stmt_any.type() == typeid(WhileStmtData)) {
                stmt = convertWhileStmt(stmt_any);
            }
            // Check if it's MatchStmtData
            else if (stmt_any.type() == typeid(MatchStmtData)) {
                stmt = convertMatchStmt(stmt_any);
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
