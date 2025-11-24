#pragma once

#include "../parser/ast.h"
#include "gdscript_byte_codegen.h"
#include "gdscript_function.h"
#include <api.hpp> // For Variant, StringName, etc. from godot-sandbox

namespace gdscript {

// Adapter to convert our AST to Godot's bytecode generator
class ASTToBytecodeAdapter {
public:
    ASTToBytecodeAdapter();
    ~ASTToBytecodeAdapter() = default;
    
    // Convert our AST ProgramNode to Godot bytecode
    // Returns a map of function names to GDScriptFunction pointers
    HashMap<StringName, GDScriptFunction*> compile_program(const ProgramNode* program);
    
    // Convert a single function from our AST to bytecode
    GDScriptFunction* compile_function(const FunctionNode* func_node, GDScript* script);
    
private:
    // Helper to convert our AST expression to bytecode generator Address
    GDScriptByteCodeGenerator::Address compile_expression(
        GDScriptByteCodeGenerator* gen,
        const ExpressionNode* expr
    );
    
    // Helper to compile statements
    void compile_statement(
        GDScriptByteCodeGenerator* gen,
        const StatementNode* stmt
    );
    
    // Convert our literal to Variant
    Variant ast_literal_to_variant(const LiteralExpr* lit);
    
    // Convert our type hint string to GDScriptDataType
    GDScriptDataType string_to_gdscript_type(const std::string& type_hint);
    
    // Reference to script being compiled (needed for bytecode generator)
    GDScript* current_script = nullptr;
};

} // namespace gdscript

