#pragma once

#include "parser/ast.h"
#include "../../ext/mustache/mustache.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <memory>

namespace gdscript {

// AST to C code emitter using Mustache templates
// Generates C code that can be compiled to RISC-V ELF using GCC/Clang
class ASTToCEmitter {
public:
    ASTToCEmitter();
    ~ASTToCEmitter() = default;
    
    // Generate C code from AST using Mustache templates
    std::string emit(const ProgramNode* program);
    
    // Clear internal state
    void clear();

private:
    
    // Mustache template strings
    static constexpr const char* FUNCTION_TEMPLATE = R"(
int64_t {{name}}({{#parameters}}{{type}} {{name}}{{^last}}, {{/last}}{{/parameters}}) {
{{#body}}
    {{statement}}
{{/body}}
{{^has_return}}
    return 0;
{{/has_return}}
}
)";
    
    static constexpr const char* PROGRAM_TEMPLATE = R"(
#include <stdint.h>
#include <stdbool.h>
{{#use_simd}}
#include <riscv_vector.h>
{{/use_simd}}

{{#forward_declarations}}
{{declaration}}
{{/forward_declarations}}

{{#functions}}
{{function_code}}
{{/functions}}
)";
    
    static constexpr const char* FOR_LOOP_TEMPLATE = R"(
{{#use_simd}}
#pragma GCC ivdep
{{/use_simd}}
for ({{type}} {{var}} = {{start}}; {{var}} < {{end}}; {{var}}++) {
{{#body}}
    {{statement}}
{{/body}}
}
)";
    
    // Helper methods to build Mustache data structures
    data _build_function_data(const FunctionNode* func);
    data _build_statement_data(const StatementNode* stmt);
    data _build_expression_data(const ExpressionNode* expr);
    data _build_literal_data(const LiteralExpr* lit);
    data _build_identifier_data(const IdentifierExpr* ident);
    data _build_binary_op_data(const BinaryOpExpr* binop);
    data _build_call_data(const CallExpr* call);
    data _build_return_data(const ReturnStatement* ret);
    data _build_variable_declaration_data(const VariableDeclaration* var_decl);
    data _build_for_loop_data(const ForStatement* for_stmt);
    data _build_while_loop_data(const WhileStatement* while_stmt);
    
    // SPMD/Vectorization helpers (SIMD, no OpenMP)
    bool _is_vectorizable_loop(const StatementNode* stmt);
    bool _has_data_dependencies(const ForStatement* for_stmt);
    bool _can_use_riscv_vector(const ForStatement* for_stmt);
    
    // Helper to render expression to string (for statements)
    std::string _expression_to_string(const ExpressionNode* expr);
    
    // Helper methods
    std::string _escape_c_string(const std::string& str);
    std::string _c_type_name(const std::string& gdscript_type);
    
    // Current function context
    const FunctionNode* _current_function;
};

} // namespace gdscript

