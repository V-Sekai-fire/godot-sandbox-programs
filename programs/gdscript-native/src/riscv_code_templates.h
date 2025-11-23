#pragma once

#include <biscuit/assembler.hpp>
#include <string>
#include <memory>

namespace gdscript {

// RISC-V code templates (similar to BeamAsm's instruction templates)
// Each template is a function that emits RISC-V machine code for a specific AST pattern

class RISCVCodeTemplates {
public:
    // Function prologue template
    // Parameters: stack_size, num_params
    static void emit_function_prologue(biscuit::Assembler* a, int stack_size, size_t num_params);
    
    // Function epilogue template
    // Parameters: stack_size
    static void emit_function_epilogue(biscuit::Assembler* a, int stack_size);
    
    // Return statement template
    // Parameters: return_value_register (a0)
    static void emit_return(biscuit::Assembler* a, biscuit::GPR return_reg);
    
    // Load immediate template
    // Parameters: destination_register, immediate_value
    static void emit_load_immediate(biscuit::Assembler* a, biscuit::GPR dest, int64_t value);
    
    // Load variable from stack template
    // Parameters: destination_register, stack_offset
    static void emit_load_from_stack(biscuit::Assembler* a, biscuit::GPR dest, int stack_offset);
    
    // Store variable to stack template
    // Parameters: source_register, stack_offset
    static void emit_store_to_stack(biscuit::Assembler* a, biscuit::GPR src, int stack_offset);
    
    // Binary operation templates
    static void emit_add(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_sub(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_mul(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_div(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_mod(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    
    // Comparison operation templates
    static void emit_eq(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_ne(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_lt(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_gt(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_le(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    static void emit_ge(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right);
    
    // Control flow templates
    static void emit_branch_if_zero(biscuit::Assembler* a, biscuit::GPR cond, biscuit::Label* target);
    static void emit_jump(biscuit::Assembler* a, biscuit::Label* target);
    static void emit_jump_and_link(biscuit::Assembler* a, biscuit::Label* target);
    
    // Parameter handling templates
    static void emit_store_parameter(biscuit::Assembler* a, biscuit::GPR param_reg, int stack_offset);
    static void emit_load_parameter(biscuit::Assembler* a, biscuit::GPR dest, int stack_offset);
};

} // namespace gdscript

