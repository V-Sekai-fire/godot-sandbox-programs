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
    // Don't clear here - preserve call stack for nested calls
    // Only clear in execute() (entry point)
    
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
    frame.should_break = false;
    frame.should_continue = false;
    
    // Initialize parameters as local variables
    for (size_t i = 0; i < func->parameters.size(); ++i) {
        frame.variables[func->parameters[i].first] = args[i];
    }
    
    _call_stack.push_back(frame);
    Frame& stack_frame = _call_stack.back(); // Get reference to frame on stack
    
    // Execute function body
    Value return_value{int64_t(0)}; // Default return value
    bool has_return = false;
    
    try {
        for (const std::unique_ptr<StatementNode>& stmt : func->body) {
            if (stmt->get_type() == ASTNode::NodeType::ReturnStatement) {
                const ReturnStatement* ret = static_cast<const ReturnStatement*>(stmt.get());
                if (ret->value) {
                    return_value = _evaluate_expression(ret->value.get(), stack_frame);
                }
                has_return = true;
                break; // Return exits function
            } else {
                _execute_statement(stmt.get(), stack_frame);
            }
        }
    } catch (const std::exception& e) {
        _call_stack.pop_back();
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
        
        case ASTNode::NodeType::UnaryOpExpr:
            return _evaluate_unary_op(static_cast<const UnaryOpExpr*>(expr), frame);
        
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
    
    // String concatenation (check first, before arithmetic)
    if (binop->op == "+" && std::holds_alternative<std::string>(left_val) && std::holds_alternative<std::string>(right_val)) {
        return Value{std::get<std::string>(left_val) + std::get<std::string>(right_val)};
    }
    
    // Handle type promotion: if either operand is float, result is float
    // Otherwise, use int64_t for arithmetic
    bool has_float = std::holds_alternative<double>(left_val) || std::holds_alternative<double>(right_val);
    
    if (has_float) {
        // Float arithmetic
        double left = 0.0;
        double right = 0.0;
        
        if (std::holds_alternative<int64_t>(left_val)) {
            left = static_cast<double>(std::get<int64_t>(left_val));
        } else if (std::holds_alternative<double>(left_val)) {
            left = std::get<double>(left_val);
        } else if (std::holds_alternative<bool>(left_val)) {
            left = std::get<bool>(left_val) ? 1.0 : 0.0;
        }
        
        if (std::holds_alternative<int64_t>(right_val)) {
            right = static_cast<double>(std::get<int64_t>(right_val));
        } else if (std::holds_alternative<double>(right_val)) {
            right = std::get<double>(right_val);
        } else if (std::holds_alternative<bool>(right_val)) {
            right = std::get<bool>(right_val) ? 1.0 : 0.0;
        }
        
        if (binop->op == "+") {
            return Value{left + right};
        } else if (binop->op == "-") {
            return Value{left - right};
        } else if (binop->op == "*") {
            return Value{left * right};
        } else if (binop->op == "/") {
            return Value{right != 0.0 ? left / right : 0.0};
        } else if (binop->op == "%") {
            return Value{right != 0.0 ? std::fmod(left, right) : 0.0};
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
    } else {
        // Integer arithmetic
        int64_t left = 0;
        int64_t right = 0;
        
        if (std::holds_alternative<int64_t>(left_val)) {
            left = std::get<int64_t>(left_val);
        } else if (std::holds_alternative<bool>(left_val)) {
            left = std::get<bool>(left_val) ? 1 : 0;
        }
        
        if (std::holds_alternative<int64_t>(right_val)) {
            right = std::get<int64_t>(right_val);
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
    }
    
    // Logical operators (short-circuit evaluation)
    if (binop->op == "and") {
        // Short-circuit: evaluate left first, only evaluate right if left is truthy
        Value left_val = _evaluate_expression(binop->left.get(), frame);
        bool left_truthy = false;
        if (std::holds_alternative<int64_t>(left_val)) {
            left_truthy = std::get<int64_t>(left_val) != 0;
        } else if (std::holds_alternative<bool>(left_val)) {
            left_truthy = std::get<bool>(left_val);
        } else if (std::holds_alternative<double>(left_val)) {
            left_truthy = std::get<double>(left_val) != 0.0;
        }
        
        if (!left_truthy) {
            return Value{int64_t(0)}; // Short-circuit: return false
        }
        
        // Left is truthy, evaluate right
        Value right_val = _evaluate_expression(binop->right.get(), frame);
        bool right_truthy = false;
        if (std::holds_alternative<int64_t>(right_val)) {
            right_truthy = std::get<int64_t>(right_val) != 0;
        } else if (std::holds_alternative<bool>(right_val)) {
            right_truthy = std::get<bool>(right_val);
        } else if (std::holds_alternative<double>(right_val)) {
            right_truthy = std::get<double>(right_val) != 0.0;
        }
        
        return Value{right_truthy ? int64_t(1) : int64_t(0)};
    } else if (binop->op == "or") {
        // Short-circuit: evaluate left first, only evaluate right if left is falsy
        Value left_val = _evaluate_expression(binop->left.get(), frame);
        bool left_truthy = false;
        if (std::holds_alternative<int64_t>(left_val)) {
            left_truthy = std::get<int64_t>(left_val) != 0;
        } else if (std::holds_alternative<bool>(left_val)) {
            left_truthy = std::get<bool>(left_val);
        } else if (std::holds_alternative<double>(left_val)) {
            left_truthy = std::get<double>(left_val) != 0.0;
        }
        
        if (left_truthy) {
            return Value{int64_t(1)}; // Short-circuit: return true
        }
        
        // Left is falsy, evaluate right
        Value right_val = _evaluate_expression(binop->right.get(), frame);
        bool right_truthy = false;
        if (std::holds_alternative<int64_t>(right_val)) {
            right_truthy = std::get<int64_t>(right_val) != 0;
        } else if (std::holds_alternative<bool>(right_val)) {
            right_truthy = std::get<bool>(right_val);
        } else if (std::holds_alternative<double>(right_val)) {
            right_truthy = std::get<double>(right_val) != 0.0;
        }
        
        return Value{right_truthy ? int64_t(1) : int64_t(0)};
    }
    
    return Value{int64_t(0)};
}

ASTInterpreter::Value ASTInterpreter::_evaluate_unary_op(const UnaryOpExpr* unary, Frame& frame) {
    if (!unary || !unary->operand) {
        return Value{int64_t(0)};
    }
    
    Value operand_val = _evaluate_expression(unary->operand.get(), frame);
    
    if (unary->op == "-" || unary->op == "MINUS") {
        // Negation
        if (std::holds_alternative<int64_t>(operand_val)) {
            return Value{-std::get<int64_t>(operand_val)};
        } else if (std::holds_alternative<double>(operand_val)) {
            return Value{-std::get<double>(operand_val)};
        } else {
            return Value{int64_t(0)};
        }
    } else if (unary->op == "+" || unary->op == "PLUS") {
        // Unary plus (no-op, but return the value)
        return operand_val;
    } else if (unary->op == "!" || unary->op == "not") {
        // Logical not
        bool truthy = false;
        if (std::holds_alternative<int64_t>(operand_val)) {
            truthy = std::get<int64_t>(operand_val) != 0;
        } else if (std::holds_alternative<bool>(operand_val)) {
            truthy = std::get<bool>(operand_val);
        } else if (std::holds_alternative<double>(operand_val)) {
            truthy = std::get<double>(operand_val) != 0.0;
        } else if (std::holds_alternative<std::nullptr_t>(operand_val)) {
            truthy = false;
        } else if (std::holds_alternative<std::string>(operand_val)) {
            truthy = !std::get<std::string>(operand_val).empty();
        }
        
        return Value{truthy ? int64_t(0) : int64_t(1)}; // Not: true -> false (0), false -> true (1)
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
    
    // Use execute_function to handle nested calls properly
    // This preserves the call stack and handles returns correctly
    ExecutionResult result = execute_function(frame.program, func_name, args);
    if (result.success) {
        return result.return_value;
    }
    
    return Value{int64_t(0)};
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
        
        case ASTNode::NodeType::ExpressionStatement:
            // Evaluate expression (for function calls, etc.) but don't use result
            {
                const ExpressionStatement* expr_stmt = static_cast<const ExpressionStatement*>(stmt);
                if (expr_stmt->expression) {
                    _evaluate_expression(expr_stmt->expression.get(), frame);
                }
            }
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
    
    // Update in current frame - this is the frame passed by reference, so update directly
    frame.variables[target->name] = value;
    
    // Also update the frame on the call stack if it's the same frame
    // (frame is a reference to the last element in _call_stack)
    if (!_call_stack.empty() && _call_stack.back().function == frame.function) {
        _call_stack.back().variables[target->name] = value;
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
        // Handle elif branches (elif_branches is vector of pairs: (condition, body))
        for (const auto& elif : if_stmt->elif_branches) {
            if (!elif.first) {
                continue;
            }
            Value elif_cond = _evaluate_expression(elif.first.get(), frame);
            bool elif_condition = false;
            if (std::holds_alternative<int64_t>(elif_cond)) {
                elif_condition = std::get<int64_t>(elif_cond) != 0;
            } else if (std::holds_alternative<bool>(elif_cond)) {
                elif_condition = std::get<bool>(elif_cond);
            }
            if (elif_condition) {
                for (const std::unique_ptr<StatementNode>& stmt : elif.second) {
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
        // Reset break/continue flags for each iteration
        frame.should_break = false;
        frame.should_continue = false;
        
        // Set loop variable
        frame.variables[for_stmt->variable_name] = Value{i};
        
        // Execute loop body
        for (const std::unique_ptr<StatementNode>& stmt : for_stmt->body) {
            _execute_statement(stmt.get(), frame);
            
            // Check for break/continue
            if (frame.should_break) {
                return; // Exit loop
            }
            if (frame.should_continue) {
                break; // Continue to next iteration
            }
        }
    }
}

void ASTInterpreter::_execute_while_statement(const WhileStatement* while_stmt, Frame& frame) {
    if (!while_stmt || !while_stmt->condition) {
        return;
    }
    
    while (true) {
        // Reset break/continue flags for each iteration
        frame.should_break = false;
        frame.should_continue = false;
        
        // Evaluate condition
        Value cond_val = _evaluate_expression(while_stmt->condition.get(), frame);
        bool condition = false;
        
        if (std::holds_alternative<int64_t>(cond_val)) {
            condition = std::get<int64_t>(cond_val) != 0;
        } else if (std::holds_alternative<bool>(cond_val)) {
            condition = std::get<bool>(cond_val);
        } else if (std::holds_alternative<double>(cond_val)) {
            condition = std::get<double>(cond_val) != 0.0;
        }
        
        if (!condition) {
            break; // Exit loop when condition is false
        }
        
        // Execute loop body
        for (const std::unique_ptr<StatementNode>& stmt : while_stmt->body) {
            _execute_statement(stmt.get(), frame);
            
            // Check for break/continue flags (for future break/continue support)
            if (frame.should_break) {
                return; // Exit loop
            }
            if (frame.should_continue) {
                break; // Continue to next iteration
            }
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
        // Convert bool to int64_t for consistency (GDScript uses 1/0 for true/false)
        return Value{std::get<bool>(lit->value) ? int64_t(1) : int64_t(0)};
    } else if (std::holds_alternative<std::nullptr_t>(lit->value)) {
        return Value{int64_t(0)}; // null is 0 in GDScript
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

