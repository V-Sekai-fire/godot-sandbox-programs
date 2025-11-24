#include "ast_to_bytecode_adapter.h"
#include "gdscript.h"
#include <cstring>

namespace gdscript {

ASTToBytecodeAdapter::ASTToBytecodeAdapter() {
}

HashMap<StringName, GDScriptFunction*> ASTToBytecodeAdapter::compile_program(const ProgramNode* program) {
    HashMap<StringName, GDScriptFunction*> functions;
    
    if (!program) {
        return functions;
    }
    
    // Create a minimal GDScript instance for compilation
    // TODO: This should be provided by the caller
    GDScript* script = nullptr; // Will need to be created properly
    
    // Compile each function
    for (const auto& func_node : program->functions) {
        if (!func_node) continue;
        
        StringName func_name(func_node->name.c_str());
        GDScriptFunction* compiled_func = compile_function(func_node.get(), script);
        if (compiled_func) {
            functions[func_name] = compiled_func;
        }
    }
    
    return functions;
}

GDScriptFunction* ASTToBytecodeAdapter::compile_function(const FunctionNode* func_node, GDScript* script) {
    if (!func_node || !script) {
        return nullptr;
    }
    
    current_script = script;
    
    // Create bytecode generator
    GDScriptByteCodeGenerator gen;
    
    // Convert return type
    GDScriptDataType return_type = string_to_gdscript_type(func_node->return_type);
    
    // Convert RPC config (if any)
    Variant rpc_config;
    if (!func_node->rpc_annotation.empty()) {
        // TODO: Parse RPC annotation properly
        rpc_config = Dictionary(); // Placeholder
    }
    
    // Start function compilation
    gen.write_start(script, StringName(func_node->name.c_str()), func_node->is_static, rpc_config, return_type);
    
    // Add parameters
    gen.start_parameters();
    for (const auto& param : func_node->parameters) {
        GDScriptDataType param_type = string_to_gdscript_type(param.second);
        gen.add_parameter(StringName(param.first.c_str()), false, param_type); // TODO: Handle optional parameters
    }
    gen.end_parameters();
    
    // Compile function body
    gen.start_block();
    for (const auto& stmt : func_node->body) {
        if (stmt) {
            compile_statement(&gen, stmt.get());
        }
    }
    gen.end_block();
    
    // End function compilation and get result
    return gen.write_end();
}

GDScriptByteCodeGenerator::Address ASTToBytecodeAdapter::compile_expression(
    GDScriptByteCodeGenerator* gen,
    const ExpressionNode* expr
) {
    if (!expr || !gen) {
        return GDScriptByteCodeGenerator::Address(GDScriptByteCodeGenerator::Address::NIL);
    }
    
    switch (expr->get_type()) {
        case ASTNode::NodeType::LiteralExpr: {
            const LiteralExpr* lit = static_cast<const LiteralExpr*>(expr);
            Variant value = ast_literal_to_variant(lit);
            uint32_t const_idx = gen->add_or_get_constant(value);
            GDScriptDataType type = string_to_gdscript_type(""); // Infer from variant
            return GDScriptByteCodeGenerator::Address(
                GDScriptByteCodeGenerator::Address::CONSTANT,
                const_idx,
                type
            );
        }
        
        case ASTNode::NodeType::IdentifierExpr: {
            const IdentifierExpr* ident = static_cast<const IdentifierExpr*>(expr);
            StringName name(ident->name.c_str());
            uint32_t name_idx = gen->add_or_get_name(name);
            // TODO: Determine if this is a local variable, member, or global
            // For now, assume it's a local variable
            GDScriptDataType type = string_to_gdscript_type("");
            return GDScriptByteCodeGenerator::Address(
                GDScriptByteCodeGenerator::Address::LOCAL_VARIABLE,
                name_idx,
                type
            );
        }
        
        case ASTNode::NodeType::BinaryOpExpr: {
            const BinaryOpExpr* binop = static_cast<const BinaryOpExpr*>(expr);
            
            // Compile left and right operands
            GDScriptByteCodeGenerator::Address left = compile_expression(gen, binop->left.get());
            GDScriptByteCodeGenerator::Address right = compile_expression(gen, binop->right.get());
            
            // Allocate temporary for result
            GDScriptDataType result_type = string_to_gdscript_type("");
            uint32_t temp_idx = gen->add_temporary(result_type);
            GDScriptByteCodeGenerator::Address result(
                GDScriptByteCodeGenerator::Address::TEMPORARY,
                temp_idx,
                result_type
            );
            
            // Convert operator string to Variant::Operator
            Variant::Operator op = Variant::OP_ADD; // Default
            if (binop->op == "+") op = Variant::OP_ADD;
            else if (binop->op == "-") op = Variant::OP_SUBTRACT;
            else if (binop->op == "*") op = Variant::OP_MULTIPLY;
            else if (binop->op == "/") op = Variant::OP_DIVIDE;
            else if (binop->op == "%") op = Variant::OP_MODULE;
            else if (binop->op == "==") op = Variant::OP_EQUAL;
            else if (binop->op == "!=") op = Variant::OP_NOT_EQUAL;
            else if (binop->op == "<") op = Variant::OP_LESS;
            else if (binop->op == ">") op = Variant::OP_GREATER;
            else if (binop->op == "<=") op = Variant::OP_LESS_EQUAL;
            else if (binop->op == ">=") op = Variant::OP_GREATER_EQUAL;
            // TODO: Handle logical operators (and, or) separately
            
            // Write binary operator
            gen->write_binary_operator(result, op, left, right);
            
            return result;
        }
        
        case ASTNode::NodeType::UnaryOpExpr: {
            const UnaryOpExpr* unary = static_cast<const UnaryOpExpr*>(expr);
            
            // Compile operand
            GDScriptByteCodeGenerator::Address operand = compile_expression(gen, unary->operand.get());
            
            // Allocate temporary for result
            GDScriptDataType result_type = string_to_gdscript_type("");
            uint32_t temp_idx = gen->add_temporary(result_type);
            GDScriptByteCodeGenerator::Address result(
                GDScriptByteCodeGenerator::Address::TEMPORARY,
                temp_idx,
                result_type
            );
            
            // Convert operator string to Variant::Operator
            Variant::Operator op = Variant::OP_NEGATE; // Default
            if (unary->op == "-" || unary->op == "MINUS") op = Variant::OP_NEGATE;
            else if (unary->op == "!" || unary->op == "not") op = Variant::OP_NOT;
            // TODO: Handle other unary operators
            
            // Write unary operator
            gen->write_unary_operator(result, op, operand);
            
            return result;
        }
        
        case ASTNode::NodeType::CallExpr: {
            const CallExpr* call = static_cast<const CallExpr*>(expr);
            
            // Compile callee (function name or object.method)
            GDScriptByteCodeGenerator::Address callee = compile_expression(gen, call->callee.get());
            
            // Compile arguments
            Vector<GDScriptByteCodeGenerator::Address> args;
            for (const auto& arg : call->arguments) {
                args.push_back(compile_expression(gen, arg.get()));
            }
            
            // Allocate temporary for result
            GDScriptDataType result_type = string_to_gdscript_type("");
            uint32_t temp_idx = gen->add_temporary(result_type);
            GDScriptByteCodeGenerator::Address result(
                GDScriptByteCodeGenerator::Address::TEMPORARY,
                temp_idx,
                result_type
            );
            
            // TODO: Determine if this is a method call, utility call, or function call
            // For now, assume it's a function call
            // This is a simplified version - real implementation needs to handle different call types
            gen->write_call(result, callee, StringName(""), args);
            
            return result;
        }
        
        default:
            // Unsupported expression type
            return GDScriptByteCodeGenerator::Address(GDScriptByteCodeGenerator::Address::NIL);
    }
}

void ASTToBytecodeAdapter::compile_statement(
    GDScriptByteCodeGenerator* gen,
    const StatementNode* stmt
) {
    if (!stmt || !gen) {
        return;
    }
    
    switch (stmt->get_type()) {
        case ASTNode::NodeType::ReturnStatement: {
            const ReturnStatement* ret = static_cast<const ReturnStatement*>(stmt);
            if (ret->value) {
                GDScriptByteCodeGenerator::Address ret_val = compile_expression(gen, ret->value.get());
                gen->write_return(ret_val);
            } else {
                // Return nil
                Variant nil_val;
                uint32_t nil_idx = gen->add_or_get_constant(nil_val);
                GDScriptDataType nil_type = string_to_gdscript_type("");
                GDScriptByteCodeGenerator::Address nil_addr(
                    GDScriptByteCodeGenerator::Address::CONSTANT,
                    nil_idx,
                    nil_type
                );
                gen->write_return(nil_addr);
            }
            break;
        }
        
        case ASTNode::NodeType::VariableDeclaration: {
            const VariableDeclaration* var_decl = static_cast<const VariableDeclaration*>(stmt);
            GDScriptDataType var_type = string_to_gdscript_type(var_decl->type_hint);
            uint32_t var_idx = gen->add_local(StringName(var_decl->name.c_str()), var_type);
            
            if (var_decl->initializer) {
                GDScriptByteCodeGenerator::Address init_val = compile_expression(gen, var_decl->initializer.get());
                GDScriptByteCodeGenerator::Address var_addr(
                    GDScriptByteCodeGenerator::Address::LOCAL_VARIABLE,
                    var_idx,
                    var_type
                );
                gen->write_assign(var_addr, init_val);
            }
            break;
        }
        
        case ASTNode::NodeType::AssignmentStatement: {
            const AssignmentStatement* assign = static_cast<const AssignmentStatement*>(stmt);
            GDScriptByteCodeGenerator::Address target = compile_expression(gen, assign->target.get());
            GDScriptByteCodeGenerator::Address value = compile_expression(gen, assign->value.get());
            
            if (assign->op == "=") {
                gen->write_assign(target, value);
            } else {
                // TODO: Handle compound assignment operators (+=, -=, etc.)
                // For now, treat as regular assignment
                gen->write_assign(target, value);
            }
            break;
        }
        
        case ASTNode::NodeType::IfStatement: {
            const IfStatement* if_stmt = static_cast<const IfStatement*>(stmt);
            
            GDScriptByteCodeGenerator::Address condition = compile_expression(gen, if_stmt->condition.get());
            gen->write_if(condition);
            
            // Compile then body
            gen->start_block();
            for (const auto& stmt : if_stmt->then_body) {
                compile_statement(gen, stmt.get());
            }
            gen->end_block();
            
            // Compile elif branches
            for (const auto& elif : if_stmt->elif_branches) {
                gen->write_else();
                GDScriptByteCodeGenerator::Address elif_condition = compile_expression(gen, elif.first.get());
                gen->write_if(elif_condition);
                gen->start_block();
                for (const auto& stmt : elif.second) {
                    compile_statement(gen, stmt.get());
                }
                gen->end_block();
            }
            
            // Compile else body
            if (!if_stmt->else_body.empty()) {
                gen->write_else();
                gen->start_block();
                for (const auto& stmt : if_stmt->else_body) {
                    compile_statement(gen, stmt.get());
                }
                gen->end_block();
            }
            
            gen->write_endif();
            break;
        }
        
        case ASTNode::NodeType::WhileStatement: {
            const WhileStatement* while_stmt = static_cast<const WhileStatement*>(stmt);
            
            gen->start_while_condition();
            GDScriptByteCodeGenerator::Address condition = compile_expression(gen, while_stmt->condition.get());
            gen->write_while(condition);
            
            gen->start_block();
            for (const auto& stmt : while_stmt->body) {
                compile_statement(gen, stmt.get());
            }
            gen->end_block();
            
            gen->write_endwhile();
            break;
        }
        
        case ASTNode::NodeType::ForStatement: {
            const ForStatement* for_stmt = static_cast<const ForStatement*>(stmt);
            
            // TODO: Implement for loop compilation
            // For loops are complex - need to handle iteration
            break;
        }
        
        case ASTNode::NodeType::ExpressionStatement: {
            const ExpressionStatement* expr_stmt = static_cast<const ExpressionStatement*>(stmt);
            // Compile expression but discard result
            compile_expression(gen, expr_stmt->expression.get());
            break;
        }
        
        default:
            // Unsupported statement type
            break;
    }
}

Variant ASTToBytecodeAdapter::ast_literal_to_variant(const LiteralExpr* lit) {
    if (!lit) {
        return Variant();
    }
    
    // LiteralExpr uses std::variant to store different types
    if (std::holds_alternative<int64_t>(lit->value)) {
        return Variant(std::get<int64_t>(lit->value));
    } else if (std::holds_alternative<double>(lit->value)) {
        return Variant(std::get<double>(lit->value));
    } else if (std::holds_alternative<bool>(lit->value)) {
        return Variant(std::get<bool>(lit->value));
    } else if (std::holds_alternative<std::nullptr_t>(lit->value)) {
        return Variant();
    } else if (std::holds_alternative<std::string>(lit->value)) {
        return Variant(String(std::get<std::string>(lit->value).c_str()));
    }
    
    return Variant();
}

GDScriptDataType ASTToBytecodeAdapter::string_to_gdscript_type(const std::string& type_hint) {
    // Convert type hint string to GDScriptDataType
    // This is a simplified version - real implementation needs to parse type hints properly
    GDScriptDataType type;
    
    if (type_hint.empty()) {
        type.kind = GDScriptDataType::VARIANT;
    } else if (type_hint == "int") {
        type.kind = GDScriptDataType::BUILTIN;
        type.builtin_type = Variant::INT;
    } else if (type_hint == "float") {
        type.kind = GDScriptDataType::BUILTIN;
        type.builtin_type = Variant::FLOAT;
    } else if (type_hint == "bool") {
        type.kind = GDScriptDataType::BUILTIN;
        type.builtin_type = Variant::BOOL;
    } else if (type_hint == "String") {
        type.kind = GDScriptDataType::BUILTIN;
        type.builtin_type = Variant::STRING;
    } else {
        // Assume it's a variant for now
        type.kind = GDScriptDataType::VARIANT;
    }
    
    return type;
}

} // namespace gdscript

