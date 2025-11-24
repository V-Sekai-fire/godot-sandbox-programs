#include "ast_to_c.h"
#include <sstream>
#include <variant>

namespace gdscript {

using namespace kainjow::mustache;

ASTToCEmitter::ASTToCEmitter() : _current_function(nullptr) {
}

void ASTToCEmitter::clear() {
    _current_function = nullptr;
}

std::string ASTToCEmitter::emit(const ProgramNode* program) {
    clear();
    
    if (!program || program->functions.empty()) {
        return "";
    }
    
    // Check if any function uses vectorization (SIMD/RVV)
    bool uses_simd = false;
    for (const std::unique_ptr<FunctionNode>& func : program->functions) {
        for (const std::unique_ptr<StatementNode>& stmt : func->body) {
            if (_is_vectorizable_loop(stmt.get())) {
                uses_simd = true;
                break;
            }
        }
        if (uses_simd) break;
    }
    
    // Build program data for Mustache template
    data program_data;
    program_data["use_simd"] = uses_simd;
    
    // Forward declarations
    data forward_decls{data::type::list};
    for (const std::unique_ptr<FunctionNode>& func : program->functions) {
        std::ostringstream decl;
        decl << "int64_t " << func->name << "(";
        for (size_t i = 0; i < func->parameters.size(); ++i) {
            if (i > 0) decl << ", ";
            decl << "int64_t " << func->parameters[i].first;
        }
        decl << ");";
        forward_decls << data{"declaration", decl.str()};
    }
    program_data["forward_declarations"] = forward_decls;
    
    // Functions
    mustache func_tmpl{FUNCTION_TEMPLATE};
    data functions_list{data::type::list};
    
    for (const std::unique_ptr<FunctionNode>& func : program->functions) {
        data func_data = _build_function_data(func.get());
        
        // Render function using template
        std::ostringstream func_out;
        func_tmpl.render(func_data, func_out);
        functions_list << data{"function_code", func_out.str()};
    }
    program_data["functions"] = functions_list;
    
    // Render final program using template
    mustache prog_tmpl{PROGRAM_TEMPLATE};
    std::ostringstream result;
    prog_tmpl.render(program_data, result);
    return result.str();
}

data ASTToCEmitter::_build_function_data(const FunctionNode* func) {
    if (!func) {
        return data{};
    }
    
    _current_function = func;
    data func_data;
    func_data["name"] = func->name;
    
    // Parameters
    data params_list{data::type::list};
    for (size_t i = 0; i < func->parameters.size(); ++i) {
        data param;
        param["type"] = "int64_t";
        param["name"] = func->parameters[i].first;
        param["last"] = (i == func->parameters.size() - 1);
        params_list << param;
    }
    func_data["parameters"] = params_list;
    
    // Body statements
    data body_list{data::type::list};
    bool has_return = false;
    for (const std::unique_ptr<StatementNode>& stmt : func->body) {
        data stmt_data = _build_statement_data(stmt.get());
        body_list << stmt_data;
        
        if (stmt->get_type() == ASTNode::NodeType::ReturnStatement) {
            has_return = true;
        }
    }
    func_data["body"] = body_list;
    func_data["has_return"] = has_return;
    
    return func_data;
}

data ASTToCEmitter::_build_statement_data(const StatementNode* stmt) {
    if (!stmt) {
        return data{"statement", "/* empty */"};
    }
    
    switch (stmt->get_type()) {
        case ASTNode::NodeType::ReturnStatement:
            return _build_return_data(static_cast<const ReturnStatement*>(stmt));
        
        case ASTNode::NodeType::VariableDeclaration:
            return _build_variable_declaration_data(static_cast<const VariableDeclaration*>(stmt));
        
        case ASTNode::NodeType::ForStatement:
            return _build_for_loop_data(static_cast<const ForStatement*>(stmt));
        
        case ASTNode::NodeType::WhileStatement:
            return _build_while_loop_data(static_cast<const WhileStatement*>(stmt));
        
        default:
            return data{"statement", "/* unknown statement */"};
    }
}

data ASTToCEmitter::_build_return_data(const ReturnStatement* ret) {
    std::ostringstream out;
    out << "return ";
    if (ret->value) {
        out << _expression_to_string(ret->value.get());
    } else {
        out << "0";
    }
    out << ";";
    return data{"statement", out.str()};
}

data ASTToCEmitter::_build_variable_declaration_data(const VariableDeclaration* var_decl) {
    std::ostringstream out;
    out << "int64_t " << var_decl->name;
    if (var_decl->initializer) {
        out << " = " << _expression_to_string(var_decl->initializer.get());
    }
    out << ";";
    return data{"statement", out.str()};
}

std::string ASTToCEmitter::_expression_to_string(const ExpressionNode* expr) {
    if (!expr) {
        return "0";
    }
    
    std::ostringstream out;
    
    switch (expr->get_type()) {
        case ASTNode::NodeType::LiteralExpr:
            {
                const LiteralExpr* lit = static_cast<const LiteralExpr*>(expr);
                if (std::holds_alternative<int64_t>(lit->value)) {
                    out << std::get<int64_t>(lit->value);
                } else if (std::holds_alternative<double>(lit->value)) {
                    out << std::get<double>(lit->value);
                } else if (std::holds_alternative<bool>(lit->value)) {
                    out << (std::get<bool>(lit->value) ? "1" : "0");
                } else if (std::holds_alternative<std::nullptr_t>(lit->value)) {
                    out << "0";
                } else if (std::holds_alternative<std::string>(lit->value)) {
                    out << "0 /* string: " << _escape_c_string(std::get<std::string>(lit->value)) << " */";
                } else {
                    out << "0";
                }
            }
            break;
        
        case ASTNode::NodeType::IdentifierExpr:
            {
                const IdentifierExpr* ident = static_cast<const IdentifierExpr*>(expr);
                out << ident->name;
            }
            break;
        
        case ASTNode::NodeType::BinaryOpExpr:
            {
                const BinaryOpExpr* binop = static_cast<const BinaryOpExpr*>(expr);
                if (binop->left && binop->right) {
                    out << "(" << _expression_to_string(binop->left.get()) 
                        << " " << binop->op << " " 
                        << _expression_to_string(binop->right.get()) << ")";
                } else {
                    out << "0";
                }
            }
            break;
        
        case ASTNode::NodeType::CallExpr:
            {
                const CallExpr* call = static_cast<const CallExpr*>(expr);
                if (call->callee && call->callee->get_type() == ASTNode::NodeType::IdentifierExpr) {
                    const IdentifierExpr* func_name = static_cast<const IdentifierExpr*>(call->callee.get());
                    out << func_name->name << "(";
                    for (size_t i = 0; i < call->arguments.size(); ++i) {
                        if (i > 0) out << ", ";
                        out << _expression_to_string(call->arguments[i].get());
                    }
                    out << ")";
                } else {
                    out << "0 /* unsupported call */";
                }
            }
            break;
        
        default:
            out << "0 /* unknown expression */";
            break;
    }
    
    return out.str();
}

std::string ASTToCEmitter::_escape_c_string(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

std::string ASTToCEmitter::_c_type_name(const std::string& gdscript_type) {
    // Map GDScript types to C types
    if (gdscript_type == "int" || gdscript_type.empty()) {
        return "int64_t";
    } else if (gdscript_type == "float") {
        return "double";
    } else if (gdscript_type == "bool") {
        return "bool";
    }
    return "int64_t"; // Default
}

data ASTToCEmitter::_build_for_loop_data(const ForStatement* for_stmt) {
    if (!for_stmt) {
        return data{"statement", "/* empty for loop */"};
    }
    
    // For now, generate simple for loop without OpenMP
    // TODO: Implement parallelization detection
    std::ostringstream out;
    out << "for (int64_t " << for_stmt->variable_name << " = 0; "
        << for_stmt->variable_name << " < /* iterable size */; "
        << for_stmt->variable_name << "++) {\n";
    
    for (const std::unique_ptr<StatementNode>& stmt : for_stmt->body) {
        out << "    ";
        data stmt_data = _build_statement_data(stmt.get());
        if (stmt_data.is_string()) {
            out << stmt_data.string_value();
        }
        out << "\n";
    }
    
    out << "}";
    return data{"statement", out.str()};
}

data ASTToCEmitter::_build_while_loop_data(const WhileStatement* while_stmt) {
    if (!while_stmt) {
        return data{"statement", "/* empty while loop */"};
    }
    
    std::ostringstream out;
    out << "while (";
    out << _expression_to_string(while_stmt->condition.get());
    out << ") {\n";
    
    for (const std::unique_ptr<StatementNode>& stmt : while_stmt->body) {
        out << "    ";
        data stmt_data = _build_statement_data(stmt.get());
        if (stmt_data.is_string()) {
            out << stmt_data.string_value();
        }
        out << "\n";
    }
    
    out << "}";
    return data{"statement", out.str()};
}

bool ASTToCEmitter::_is_vectorizable_loop(const StatementNode* stmt) {
    if (!stmt) {
        return false;
    }
    
    // Check if it's a for loop that can be vectorized
    if (stmt->get_type() == ASTNode::NodeType::ForStatement) {
        const ForStatement* for_stmt = static_cast<const ForStatement*>(stmt);
        // Vectorizable if no data dependencies and element-wise operations
        return !_has_data_dependencies(for_stmt);
    }
    
    return false;
}

bool ASTToCEmitter::_has_data_dependencies(const ForStatement* for_stmt) {
    if (!for_stmt) {
        return true; // Conservative: assume dependencies if unknown
    }
    
    // Simple heuristic: if loop body only reads from array and writes to result array,
    // it's likely parallelizable. More sophisticated analysis needed.
    // For now, return false (no dependencies) - can be improved later
    return false;
}

bool ASTToCEmitter::_can_use_riscv_vector(const ForStatement* for_stmt) {
    if (!for_stmt) {
        return false;
    }
    
    // Check if loop can use RISC-V Vector Extension
    // Requirements:
    // - Element-wise operations on arrays
    // - No dependencies between iterations
    // - Simple arithmetic operations (add, sub, mul, etc.)
    
    // For now, return false (use compiler auto-vectorization instead)
    // TODO: Implement RVV code generation for large arrays
    return false;
}

} // namespace gdscript

