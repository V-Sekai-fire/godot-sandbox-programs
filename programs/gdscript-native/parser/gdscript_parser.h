#pragma once

#include "ast.h"
#include "errors.h"
#include <string>
#include <memory>
#include <any>
#include <vector>

namespace gdscript {

// GDScript parser using cpp-peglib
class GDScriptParser {
private:
    // PEG parser instance
    void* parser; // Will be peg::parser* (forward declaration to avoid including peglib.h in header)
    
    // Workaround for cpp-peglib issue: Program semantic value isn't stored in parse result
    // Store it as a member variable instead of relying on the parse result
    struct ProgramData {
        std::vector<std::any> functions; // FunctionData
        std::vector<std::any> statements; // StatementNode or other
    };
    ProgramData last_program_data;
    
    // Error collection
    ErrorCollection _errors;
    
    // Store last error message for retrieval (for backward compatibility)
    mutable std::string last_error_message;
    
    // Build AST from parse result
    std::unique_ptr<ProgramNode> buildAST(const void* parseResult);
    
    // Helper methods to build AST nodes from semantic values
    std::unique_ptr<FunctionNode> buildFunction(const void* sv);
    std::unique_ptr<ReturnStatement> buildReturn(const void* sv);
    std::unique_ptr<ExpressionNode> buildExpression(const void* sv);
    std::unique_ptr<LiteralExpr> buildLiteral(const void* sv);
    std::unique_ptr<IdentifierExpr> buildIdentifier(const void* sv);
    std::unique_ptr<BinaryOpExpr> buildBinaryOp(const void* sv);

public:
    GDScriptParser();
    ~GDScriptParser();
    
    // Parse GDScript source code and return AST
    std::unique_ptr<ProgramNode> parse(const std::string& source);
    
    // Get error message if parsing failed (backward compatibility)
    std::string getErrorMessage() const;
    
    // Get error collection
    const ErrorCollection& get_errors() const { return _errors; }
    ErrorCollection& get_errors() { return _errors; }
    
    // Check if parser is valid
    bool is_valid() const;
};

} // namespace gdscript

