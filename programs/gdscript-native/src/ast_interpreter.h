#pragma once

#include "parser/ast.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>
#include <variant>

namespace gdscript {

// AST Interpreter - executes AST directly without code generation
// Similar to a VM but operates on AST nodes instead of bytecode
class ASTInterpreter {
public:
    // Value type for interpreter (can hold different types)
    using Value = std::variant<int64_t, double, bool, std::nullptr_t, std::string>;
    
    // Execution result
    struct ExecutionResult {
        Value return_value;
        bool success;
        std::string error_message;
    };
    
    ASTInterpreter();
    ~ASTInterpreter() = default;
    
    // Execute a program (calls the first function)
    ExecutionResult execute(const ProgramNode* program);
    
    // Execute a specific function by name
    ExecutionResult execute_function(const ProgramNode* program, const std::string& function_name, const std::vector<Value>& args = {});
    
    // Clear interpreter state
    void clear();

private:
    // Frame for function execution (stack frame)
    struct Frame {
        std::unordered_map<std::string, Value> variables; // Local variables
        const FunctionNode* function; // Current function
        const ProgramNode* program; // Program context (for function calls)
        size_t pc; // Program counter (statement index)
        bool should_break; // Flag for break statement
        bool should_continue; // Flag for continue statement
    };
    
    // Execution stack (for function calls)
    std::vector<Frame> _call_stack;
    
    // Global variables (if needed)
    std::unordered_map<std::string, Value> _globals;
    
    // Helper methods
    Value _evaluate_expression(const ExpressionNode* expr, Frame& frame);
    void _execute_statement(const StatementNode* stmt, Frame& frame);
    
    // Expression evaluators
    Value _evaluate_literal(const LiteralExpr* lit);
    Value _evaluate_identifier(const IdentifierExpr* ident, Frame& frame);
    Value _evaluate_binary_op(const BinaryOpExpr* binop, Frame& frame);
    Value _evaluate_unary_op(const UnaryOpExpr* unary, Frame& frame);
    Value _evaluate_call(const CallExpr* call, Frame& frame);
    
    // Statement executors
    void _execute_return(const ReturnStatement* ret, Frame& frame);
    void _execute_variable_declaration(const VariableDeclaration* var_decl, Frame& frame);
    void _execute_assignment(const AssignmentStatement* assign, Frame& frame);
    void _execute_if_statement(const IfStatement* if_stmt, Frame& frame);
    void _execute_for_statement(const ForStatement* for_stmt, Frame& frame);
    void _execute_while_statement(const WhileStatement* while_stmt, Frame& frame);
    
    // Helper to convert AST literal to Value
    Value _literal_to_value(const LiteralExpr* lit);
    
    // Helper to get function by name
    const FunctionNode* _find_function(const ProgramNode* program, const std::string& name);
};

} // namespace gdscript

