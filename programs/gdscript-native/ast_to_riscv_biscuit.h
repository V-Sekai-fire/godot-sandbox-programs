#pragma once

#include "parser/ast.h"
#include <biscuit/assembler.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace gdscript {

// Direct AST to RISC-V machine code emitter using biscuit (following BEAM JIT pattern)
// Similar to how BEAM JIT uses AsmJit - generates machine code directly, not assembly text
class ASTToRISCVEmitter {
private:
    // Buffer for generated machine code
    std::vector<uint8_t> _code_buffer;
    std::unique_ptr<biscuit::Assembler> _assembler;
    
    // Register allocation
    std::unordered_map<const ExpressionNode*, biscuit::GPR> _expr_to_reg;
    std::unordered_map<std::string, int> _var_to_stack_offset; // Variable name -> stack offset
    
    // Stack management
    int _stack_offset;
    int _current_function_stack_size; // Track stack size for current function
    size_t _label_counter;
    size_t _temp_reg_index;
    const FunctionNode* _current_function; // Track current function being emitted
    
    // RISC-V registers: a0-a7 (arguments/returns), t0-t6 (temporaries), s0-s11 (saved)
    static constexpr biscuit::GPR _temp_regs[] = {
        biscuit::t0, biscuit::t1, biscuit::t2,
        biscuit::t3, biscuit::t4, biscuit::t5, biscuit::t6
    };
    static constexpr size_t _num_temp_regs = 7;
    
    // Helper methods (private, use _ prefix)
    biscuit::GPR _allocate_register();
    int _allocate_stack(const std::string& var_name);
    biscuit::GPR _get_or_allocate_register(const ExpressionNode* expr);
    int _get_var_stack_offset(const std::string& var_name);
    
    // Emit methods for different AST node types (private, use _ prefix)
    void _emit_program(const ProgramNode* program);
    void _emit_function(const FunctionNode* func);
    void _emit_statement(const StatementNode* stmt);
    void _emit_expression(const ExpressionNode* expr);
    
    // Expression emitters (private, use _ prefix)
    void _emit_literal(const LiteralExpr* lit);
    void _emit_identifier(const IdentifierExpr* ident);
    void _emit_binary_op(const BinaryOpExpr* binop);
    void _emit_return(const ReturnStatement* ret);
    void _emit_variable_declaration(const VariableDeclaration* var_decl);
    
    // Reset state for new function (private, use _ prefix)
    void _reset_function_state();
    
public:
    ASTToRISCVEmitter() : _stack_offset(0), _current_function_stack_size(16), 
                          _label_counter(0), _temp_reg_index(0), _current_function(nullptr) {
        _code_buffer.resize(8192); // Initial buffer size (will grow if needed)
    }
    
    // Main entry point: emit RISC-V machine code from AST
    // Returns pointer to machine code and size
    std::pair<const uint8_t*, size_t> emit(const ProgramNode* program);
    
    // Get the generated machine code
    const std::vector<uint8_t>& get_code() const { return _code_buffer; }
    
    // Clear state
    void clear() {
        _code_buffer.clear();
        _code_buffer.resize(8192);
        _expr_to_reg.clear();
        _var_to_stack_offset.clear();
        _stack_offset = 0;
        _current_function_stack_size = 16;
        _label_counter = 0;
        _temp_reg_index = 0;
        _current_function = nullptr;
    }
};

} // namespace gdscript

