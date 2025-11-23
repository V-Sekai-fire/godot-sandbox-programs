#include "ast_interpreter.h"
#include <stdexcept>
#include <cmath>

namespace gdscript {

ASTInterpreter::ASTInterpreter() {
}

void ASTInterpreter::clear() {
    _call_stack.clear();
    _globals.clear();
}

ASTInterpreter::ExecutionResult ASTInterpreter::execute(const ProgramNode* program) {
    clear();
    
    if (!program || program->functions.empty()) {
        return {Value{int64_t(0)}, false, "No functions in program"};
    }
    
    // Execute first function (entry point)
    const FunctionNode* entry = program->functions[0].get();
    return execute_function(program, entry->name);
}

ASTInterpreter::ExecutionResult ASTInterpreter::execute_function(
    const ProgramNode* program,
    const std::string& function_name,
    const std::vector<Value>& args
) {
    clear();
    
    const FunctionNode* func = _find_function(program, function_name);
    if (!func) {
        return {Value{int64_t(0)}, false, "Function not found: " + function_name};
    }
    
    // Check argument count
    if (args.size() != func->parameters.size()) {
        return {Value{int64_t(0)}, false, "Argument count mismatch"};
    }
    
    // Create frame for function execution
    Frame frame;
    frame.function = func;
    frame.program = program;
    frame.pc = 0;
    
    // Initialize parameters as local variables
    for (size_t i = 0; i < func->parameters.size(); ++i) {
        frame.variables[func->parameters[i].first] = args[i];
    }
    
    _call_stack.push_back(frame);
    
    // Execute function body
    Value return_value{int64_t(0)}; // Default return value
    bool has_return = false;
    
    try {
        for (const std::unique_ptr<StatementNode>& stmt : func->body) {
            if (stmt->get_type() == ASTNode::NodeType::ReturnStatement) {
                const ReturnStatement* ret = static_cast<const ReturnStatement*>(stmt.get());
                if (ret->value) {
                    return_value = _evaluate_expression(ret->value.get(), frame);
                }
                has_return = true;
                break; // Return exits function
            } else {
                _execute_statement(stmt.get(), frame);
            }
        }
    } catch (const std::exception& e) {
        return {Value{int64_t(0)}, false, std::string("Execution error: ") + e.what()};
    }
    
    _call_stack.pop_back();
    
    return {return_value, true, ""};
}

ASTInterpreter::Value ASTInterpreter::_evaluate_expression(const ExpressionNode* expr, Frame& frame) {
    if (!expr) {
        return Value{int64_t(0)};
    }
    
    switch (expr->get_type()) {
        case ASTNode::NodeType::LiteralExpr:
            return _evaluate_literal(static_cast<const LiteralExpr*>(expr));
        
        case ASTNode::NodeType::IdentifierExpr:
            return _evaluate_identifier(static_cast<const IdentifierExpr*>(expr), frame);
        
        case ASTNode::NodeType::BinaryOpExpr:
            return _evaluate_binary_op(static_cast<const BinaryOpExpr*>(expr), frame);
        
        case ASTNode::NodeType::CallExpr:
            return _evaluate_call(static_cast<const CallExpr*>(expr), frame);
        
        default:
            return Value{int64_t(0)};
    }
}

ASTInterpreter::Value ASTInterpreter::_evaluate_literal(const LiteralExpr* lit) {
    return _literal_to_value(lit);
}

ASTInterpreter::Value ASTInterpreter::_evaluate_identifier(const IdentifierExpr* ident, Frame& frame) {
    // Look up in current frame
    if (frame.variables.find(ident->name) != frame.variables.end()) {
        return frame.variables[ident->name];
    }
    
    // Look up in globals
    if (_globals.find(ident->name) != _globals.end()) {
        return _globals[ident->name];
    }
    
    // Variable not found - return 0
    return Value{int64_t(0)};
}

ASTInterpreter::Value ASTInterpreter::_evaluate_binary_op(const BinaryOpExpr* binop, Frame& frame) {
    if (!binop || !binop->left || !binop->right) {
        return Value{int64_t(0)};
    }
    
    Value left_val = _evaluate_expression(binop->left.get(), frame);
    Value right_val = _evaluate_expression(binop->right.get(), frame);
    
    // For simplicity, convert everything to int64_t for arithmetic
    // In a full implementation, we'd handle type promotion properly
    int64_t left = 0;
    int64_t right = 0;
    
    if (std::holds_alternative<int64_t>(left_val)) {
        left = std::get<int64_t>(left_val);
    } else if (std::holds_alternative<double>(left_val)) {
        left = static_cast<int64_t>(std::get<double>(left_val));
    } else if (std::holds_alternative<bool>(left_val)) {
        left = std::get<bool>(left_val) ? 1 : 0;
    }
    
    if (std::holds_alternative<int64_t>(right_val)) {
        right = std::get<int64_t>(right_val);
    } else if (std::holds_alternative<double>(right_val)) {
        right = static_cast<int64_t>(std::get<double>(right_val));
    } else if (std::holds_alternative<bool>(right_val)) {
        right = std::get<bool>(right_val) ? 1 : 0;
    }
    
    if (binop->op == "+") {
        return Value{left + right};
    } else if (binop->op == "-") {
        return Value{left - right};
    } else if (binop->op == "*") {
        return Value{left * right};
    } else if (binop->op == "/") {
        return Value{right != 0 ? left / right : int64_t(0)};
    } else if (binop->op == "%") {
        return Value{right != 0 ? left % right : int64_t(0)};
    } else if (binop->op == "==") {
        return Value{left == right ? int64_t(1) : int64_t(0)};
    } else if (binop->op == "!=") {
        return Value{left != right ? int64_t(1) : int64_t(0)};
    } else if (binop->op == "<") {
        return Value{left < right ? int64_t(1) : int64_t(0)};
    } else if (binop->op == ">") {
        return Value{left > right ? int64_t(1) : int64_t(0)};
    } else if (binop->op == "<=") {
        return Value{left <= right ? int64_t(1) : int64_t(0)};
    } else if (binop->op == ">=") {
        return Value{left >= right ? int64_t(1) : int64_t(0)};
    }
    
    return Value{int64_t(0)};
}

ASTInterpreter::Value ASTInterpreter::_evaluate_call(const CallExpr* call, Frame& frame) {
    if (!call || !call->callee) {
        return Value{int64_t(0)};
    }
    
    // Get function name from callee (must be IdentifierExpr)
    if (call->callee->get_type() != ASTNode::NodeType::IdentifierExpr) {
        return Value{int64_t(0)};
    }
    
    const IdentifierExpr* func_name_expr = static_cast<const IdentifierExpr*>(call->callee.get());
    std::string func_name = func_name_expr->name;
    
    // Find function in program
    if (!frame.program) {
        return Value{int64_t(0)};
    }
    
    const FunctionNode* target_func = _find_function(frame.program, func_name);
    if (!target_func) {
        return Value{int64_t(0)};
    }
    
    // Evaluate arguments
    std::vector<Value> args;
    for (const std::unique_ptr<ExpressionNode>& arg : call->arguments) {
        args.push_back(_evaluate_expression(arg.get(), frame));
    }
    
    // Check argument count
    if (args.size() != target_func->parameters.size()) {
        return Value{int64_t(0)};
    }
    
    // Create new frame for called function
    Frame call_frame;
    call_frame.function = target_func;
    call_frame.program = frame.program;
    call_frame.pc = 0;
    
    // Initialize parameters
    for (size_t i = 0; i < target_func->parameters.size(); ++i) {
        call_frame.variables[target_func->parameters[i].first] = args[i];
    }
    
    // Push frame onto call stack
    _call_stack.push_back(call_frame);
    
    // Execute function body
    Value return_value{int64_t(0)};
    bool has_return = false;
    
    for (const std::unique_ptr<StatementNode>& stmt : target_func->body) {
        if (stmt->get_type() == ASTNode::NodeType::ReturnStatement) {
            const ReturnStatement* ret = static_cast<const ReturnStatement*>(stmt.get());
            if (ret->value) {
                return_value = _evaluate_expression(ret->value.get(), call_frame);
            }
            has_return = true;
            break;
        } else {
            _execute_statement(stmt.get(), call_frame);
        }
    }
    
    // Pop frame from call stack
    _call_stack.pop_back();
    
    return return_value;
}

void ASTInterpreter::_execute_statement(const StatementNode* stmt, Frame& frame) {
    if (!stmt) {
        return;
    }
    
    switch (stmt->get_type()) {
        case ASTNode::NodeType::ReturnStatement:
            _execute_return(static_cast<const ReturnStatement*>(stmt), frame);
            break;
        
        case ASTNode::NodeType::VariableDeclaration:
            _execute_variable_declaration(static_cast<const VariableDeclaration*>(stmt), frame);
            break;
        
        case ASTNode::NodeType::AssignmentStatement:
            _execute_assignment(static_cast<const AssignmentStatement*>(stmt), frame);
            break;
        
        case ASTNode::NodeType::IfStatement:
            _execute_if_statement(static_cast<const IfStatement*>(stmt), frame);
            break;
        
        case ASTNode::NodeType::ForStatement:
            _execute_for_statement(static_cast<const ForStatement*>(stmt), frame);
            break;
        
        case ASTNode::NodeType::WhileStatement:
            _execute_while_statement(static_cast<const WhileStatement*>(stmt), frame);
            break;
        
        default:
            break;
    }
}

void ASTInterpreter::_execute_return(const ReturnStatement* ret, Frame& frame) {
    // Return is handled in execute_function
    // This is a no-op here
}

void ASTInterpreter::_execute_variable_declaration(const VariableDeclaration* var_decl, Frame& frame) {
    if (!var_decl) {
        return;
    }
    
    Value value{int64_t(0)};
    if (var_decl->initializer) {
        value = _evaluate_expression(var_decl->initializer.get(), frame);
    }
    
    frame.variables[var_decl->name] = value;
}

void ASTInterpreter::_execute_assignment(const AssignmentStatement* assign, Frame& frame) {
    if (!assign || !assign->target || !assign->value) {
        return;
    }
    
    if (assign->target->get_type() != ASTNode::NodeType::IdentifierExpr) {
        return; // Only support simple variable assignments
    }
    
    const IdentifierExpr* target = static_cast<const IdentifierExpr*>(assign->target.get());
    Value value = _evaluate_expression(assign->value.get(), frame);
    
    // Update in current frame or globals
    if (frame.variables.find(target->name) != frame.variables.end()) {
        frame.variables[target->name] = value;
    } else {
        _globals[target->name] = value;
    }
}

void ASTInterpreter::_execute_if_statement(const IfStatement* if_stmt, Frame& frame) {
    if (!if_stmt || !if_stmt->condition) {
        return;
    }
    
    Value cond_val = _evaluate_expression(if_stmt->condition.get(), frame);
    bool condition = false;
    
    if (std::holds_alternative<int64_t>(cond_val)) {
        condition = std::get<int64_t>(cond_val) != 0;
    } else if (std::holds_alternative<bool>(cond_val)) {
        condition = std::get<bool>(cond_val);
    }
    
    if (condition) {
        for (const std::unique_ptr<StatementNode>& stmt : if_stmt->then_body) {
            _execute_statement(stmt.get(), frame);
        }
    } else if (!if_stmt->elif_branches.empty()) {
        // Handle elif branches
        for (const auto& elif : if_stmt->elif_branches) {
            if (!elif.condition) {
                continue;
            }
            Value elif_cond = _evaluate_expression(elif.condition.get(), frame);
            bool elif_condition = false;
            if (std::holds_alternative<int64_t>(elif_cond)) {
                elif_condition = std::get<int64_t>(elif_cond) != 0;
            } else if (std::holds_alternative<bool>(elif_cond)) {
                elif_condition = std::get<bool>(elif_cond);
            }
            if (elif_condition) {
                for (const std::unique_ptr<StatementNode>& stmt : elif.body) {
                    _execute_statement(stmt.get(), frame);
                }
                return;
            }
        }
        // Handle else branch
        if (!if_stmt->else_body.empty()) {
            for (const std::unique_ptr<StatementNode>& stmt : if_stmt->else_body) {
                _execute_statement(stmt.get(), frame);
            }
        }
    } else if (!if_stmt->else_body.empty()) {
        for (const std::unique_ptr<StatementNode>& stmt : if_stmt->else_body) {
            _execute_statement(stmt.get(), frame);
        }
    }
}

void ASTInterpreter::_execute_for_statement(const ForStatement* for_stmt, Frame& frame) {
    if (!for_stmt) {
        return;
    }
    
    // For now, handle simple range-based loops: for i in range(N)
    // The iterable expression should be evaluated to get the range
    // For simplicity, assume iterable is a literal or identifier representing a number
    
    if (!for_stmt->iterable) {
        return;
    }
    
    Value iterable_val = _evaluate_expression(for_stmt->iterable.get(), frame);
    int64_t end_value = 0;
    
    if (std::holds_alternative<int64_t>(iterable_val)) {
        end_value = std::get<int64_t>(iterable_val);
    } else if (std::holds_alternative<double>(iterable_val)) {
        end_value = static_cast<int64_t>(std::get<double>(iterable_val));
    } else if (std::holds_alternative<bool>(iterable_val)) {
        end_value = std::get<bool>(iterable_val) ? 1 : 0;
    }
    
    // Execute loop: for i in range(N) means i goes from 0 to N-1
    for (int64_t i = 0; i < end_value; ++i) {
        // Set loop variable
        frame.variables[for_stmt->variable_name] = Value{i};
        
        // Execute loop body
        for (const std::unique_ptr<StatementNode>& stmt : for_stmt->body) {
            _execute_statement(stmt.get(), frame);
        }
    }
}

void ASTInterpreter::_execute_while_statement(const WhileStatement* while_stmt, Frame& frame) {
    if (!while_stmt || !while_stmt->condition) {
        return;
    }
    
    while (true) {
        Value cond_val = _evaluate_expression(while_stmt->condition.get(), frame);
        bool condition = false;
        
        if (std::holds_alternative<int64_t>(cond_val)) {
            condition = std::get<int64_t>(cond_val) != 0;
        } else if (std::holds_alternative<bool>(cond_val)) {
            condition = std::get<bool>(cond_val);
        }
        
        if (!condition) {
            break;
        }
        
        for (const std::unique_ptr<StatementNode>& stmt : while_stmt->body) {
            _execute_statement(stmt.get(), frame);
        }
    }
}

ASTInterpreter::Value ASTInterpreter::_literal_to_value(const LiteralExpr* lit) {
    if (!lit) {
        return Value{int64_t(0)};
    }
    
    if (std::holds_alternative<int64_t>(lit->value)) {
        return Value{std::get<int64_t>(lit->value)};
    } else if (std::holds_alternative<double>(lit->value)) {
        return Value{std::get<double>(lit->value)};
    } else if (std::holds_alternative<bool>(lit->value)) {
        return Value{std::get<bool>(lit->value)};
    } else if (std::holds_alternative<std::nullptr_t>(lit->value)) {
        return Value{nullptr};
    } else if (std::holds_alternative<std::string>(lit->value)) {
        return Value{std::get<std::string>(lit->value)};
    }
    
    return Value{int64_t(0)};
}

const FunctionNode* ASTInterpreter::_find_function(const ProgramNode* program, const std::string& name) {
    if (!program) {
        return nullptr;
    }
    
    for (const std::unique_ptr<FunctionNode>& func : program->functions) {
        if (func->name == name) {
            return func.get();
        }
    }
    
    return nullptr;
}

} // namespace gdscript

