#include "riscv_code_templates.h"
#include "constants.h"

namespace gdscript {

// Function prologue template (similar to BeamAsm's function entry)
// Sets up stack frame, saves registers
void RISCVCodeTemplates::emit_function_prologue(biscuit::Assembler* a, int stack_size, size_t num_params) {
    if (!a) return;
    
    using namespace constants;
    
    // Allocate stack space
    a->ADDI(biscuit::sp, biscuit::sp, -stack_size);
    
    // Save return address and frame pointer
    a->SD(biscuit::ra, stack_size - 8, biscuit::sp);
    a->SD(biscuit::s0, stack_size - 16, biscuit::sp);
    
    // Set frame pointer
    a->ADDI(biscuit::s0, biscuit::sp, stack_size);
    
    // Store incoming parameters from registers to stack
    static constexpr biscuit::GPR argRegs[] = {
        biscuit::a0, biscuit::a1, biscuit::a2, biscuit::a3,
        biscuit::a4, biscuit::a5, biscuit::a6, biscuit::a7
    };
    
    for (size_t i = 0; i < num_params && i < MAX_ARGUMENT_REGISTERS; ++i) {
        int offset = SAVED_REGISTERS_SIZE + static_cast<int>(i * BYTES_PER_PARAMETER);
        a->SD(argRegs[i], offset, biscuit::sp);
    }
}

// Function epilogue template (similar to BeamAsm's return instruction)
// Restores registers and returns
void RISCVCodeTemplates::emit_function_epilogue(biscuit::Assembler* a, int stack_size) {
    if (!a) return;
    
    using namespace constants;
    
    // Restore saved registers
    a->LD(biscuit::ra, stack_size - 8, biscuit::sp);
    a->LD(biscuit::s0, stack_size - 16, biscuit::sp);
    
    // Restore stack pointer
    a->ADDI(biscuit::sp, biscuit::sp, stack_size);
    
    // Return (JALR x0, 0, ra)
    a->JALR(biscuit::x0, 0, biscuit::ra);
}

// Return statement template
void RISCVCodeTemplates::emit_return(biscuit::Assembler* a, biscuit::GPR return_reg) {
    if (!a) return;
    
    // Return value should already be in return_reg (typically a0)
    // Jump to epilogue (epilogue label should be set by caller)
    // For now, this is a placeholder - actual return jumps to epilogue
}

// Load immediate template (LI instruction)
void RISCVCodeTemplates::emit_load_immediate(biscuit::Assembler* a, biscuit::GPR dest, int64_t value) {
    if (!a) return;
    a->LI(dest, value);
}

// Load variable from stack template
void RISCVCodeTemplates::emit_load_from_stack(biscuit::Assembler* a, biscuit::GPR dest, int stack_offset) {
    if (!a) return;
    a->LD(dest, stack_offset, biscuit::sp);
}

// Store variable to stack template
void RISCVCodeTemplates::emit_store_to_stack(biscuit::Assembler* a, biscuit::GPR src, int stack_offset) {
    if (!a) return;
    a->SD(src, stack_offset, biscuit::sp);
}

// Binary operation templates
void RISCVCodeTemplates::emit_add(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    a->ADD(dest, left, right);
}

void RISCVCodeTemplates::emit_sub(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    a->SUB(dest, left, right);
}

void RISCVCodeTemplates::emit_mul(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    a->MUL(dest, left, right);
}

void RISCVCodeTemplates::emit_div(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    a->DIV(dest, left, right);
}

void RISCVCodeTemplates::emit_mod(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    a->REM(dest, left, right);
}

// Comparison operation templates
void RISCVCodeTemplates::emit_eq(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    // EQ: (left == right) ? 1 : 0
    // Use XOR to check equality, then set to 1 if zero
    a->XOR(dest, left, right);
    a->SLTIU(dest, dest, 1); // dest = (dest == 0) ? 1 : 0
}

void RISCVCodeTemplates::emit_ne(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    // NE: (left != right) ? 1 : 0
    a->XOR(dest, left, right);
    a->SLTU(dest, biscuit::x0, dest); // dest = (dest != 0) ? 1 : 0
}

void RISCVCodeTemplates::emit_lt(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    a->SLT(dest, left, right);
}

void RISCVCodeTemplates::emit_gt(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    // GT: (left > right) = (right < left)
    a->SLT(dest, right, left);
}

void RISCVCodeTemplates::emit_le(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    // LE: (left <= right) = !(left > right) = !(right < left)
    a->SLT(dest, right, left);
    a->XORI(dest, dest, 1); // Invert
}

void RISCVCodeTemplates::emit_ge(biscuit::Assembler* a, biscuit::GPR dest, biscuit::GPR left, biscuit::GPR right) {
    if (!a) return;
    // GE: (left >= right) = !(left < right)
    a->SLT(dest, left, right);
    a->XORI(dest, dest, 1); // Invert
}

// Control flow templates
void RISCVCodeTemplates::emit_branch_if_zero(biscuit::Assembler* a, biscuit::GPR cond, biscuit::Label* target) {
    if (!a) return;
    a->BEQZ(cond, target);
}

void RISCVCodeTemplates::emit_jump(biscuit::Assembler* a, biscuit::Label* target) {
    if (!a) return;
    a->JAL(target);
}

void RISCVCodeTemplates::emit_jump_and_link(biscuit::Assembler* a, biscuit::Label* target) {
    if (!a) return;
    a->JAL(target);
}

// Parameter handling templates
void RISCVCodeTemplates::emit_store_parameter(biscuit::Assembler* a, biscuit::GPR param_reg, int stack_offset) {
    if (!a) return;
    a->SD(param_reg, stack_offset, biscuit::sp);
}

void RISCVCodeTemplates::emit_load_parameter(biscuit::Assembler* a, biscuit::GPR dest, int stack_offset) {
    if (!a) return;
    a->LD(dest, stack_offset, biscuit::sp);
}

} // namespace gdscript

