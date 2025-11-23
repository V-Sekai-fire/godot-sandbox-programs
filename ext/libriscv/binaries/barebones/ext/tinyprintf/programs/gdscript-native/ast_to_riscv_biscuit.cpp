#include "ast_to_riscv_biscuit.h"
#include <cstring>

namespace gdscript {

std::pair<const uint8_t*, size_t> ASTToRISCVEmitter::emit(const ProgramNode* program) {
    clear();
    
    if (!program || program->functions.empty()) {
        return {nullptr, 0};
    }
    
    // Initialize biscuit assembler (direct machine code generation, similar to AsmJit)
    _assembler = std::make_unique<biscuit::Assembler>(_code_buffer.data(), _code_buffer.size());
    
    // Emit all functions sequentially
    for (const auto& func : program->functions) {
        _emit_function(func.get());
        
        // Check buffer capacity (grow if >90% full)
        size_t currentOffset = static_cast<size_t>(_assembler->GetCodeBuffer().GetCursorOffset());
        if (currentOffset >= _code_buffer.size() * 0.9) {
            // Buffer growth requires recreating assembler, so we allocate large initial buffer
            // TODO: Implement proper buffer growth strategy if needed
        }
    }
    
    // Get final code size
    size_t codeSize = static_cast<size_t>(_assembler->GetCodeBuffer().GetCursorOffset());
    
    _assembler.reset();
    
    return {_code_buffer.data(), codeSize};
}

void ASTToRISCVEmitter::_emit_function(const FunctionNode* func) {
    if (!func || !_assembler) {
        return;
    }
    
    _current_function = func;
    _reset_function_state();
    
    // RISC-V 64 Linux ABI stack frame layout:
    // - sp+0: saved frame pointer (s0/fp)
    // - sp+8: saved return address (ra)
    // - sp+16+: parameters and local variables
    int paramStackSize = static_cast<int>(func->parameters.size() * 8);
    int initialStackSize = 16 + paramStackSize;  // Saved regs + parameters
    int estimatedLocalVars = 64;                 // Conservative estimate for locals
    int stackSize = initialStackSize + estimatedLocalVars;
    _current_function_stack_size = stackSize;
    
    // Function prologue
    _assembler->ADDI(biscuit::sp, biscuit::sp, -stackSize);
    _assembler->SD(biscuit::ra, stackSize - 8, biscuit::sp);
    _assembler->SD(biscuit::s0, stackSize - 16, biscuit::sp);
    _assembler->ADDI(biscuit::s0, biscuit::sp, stackSize);
    
    // RISC-V 64 Linux calling convention: arguments in a0-a7, return in a0
    // Store incoming arguments from registers to stack for variable access
    static constexpr biscuit::GPR argRegs[] = {
        biscuit::a0, biscuit::a1, biscuit::a2, biscuit::a3,
        biscuit::a4, biscuit::a5, biscuit::a6, biscuit::a7
    };
    
    for (size_t i = 0; i < func->parameters.size() && i < 8; ++i) {
        int offset = 16 + static_cast<int>(i * 8); // After saved registers
        _assembler->SD(argRegs[i], offset, biscuit::sp);
        _var_to_stack_offset[func->parameters[i].first] = offset;
    }
    
    _stack_offset = static_cast<int>(func->parameters.size() * 8);
    
    // Emit function body
    for (const auto& stmt : func->body) {
        _emit_statement(stmt.get());
    }
    
    // Function epilogue: exit via syscall (for entry point functions)
    // Default return value is 0 if no explicit return statement
    // Note: a0 may already contain return value from explicit return statement
    
    // For entry point functions called by libriscv, exit via syscall instead of returning
    // Linux syscall: exit_group(a0) - syscall 94 (0x5e) for riscv64
    // a0 already contains the return value (either from explicit return or default 0)
    _assembler->LI(biscuit::a7, 94);  // syscall number for exit_group
    _assembler->ECALL();               // Make syscall (exits program)
}

void ASTToRISCVEmitter::_emit_statement(const StatementNode* stmt) {
    if (!stmt || !_assembler) {
        return;
    }
    
    switch (stmt->get_type()) {
        case ASTNode::NodeType::ReturnStatement:
            _emit_return(static_cast<const ReturnStatement*>(stmt));
            break;
        
        case ASTNode::NodeType::VariableDeclaration:
            _emit_variable_declaration(static_cast<const VariableDeclaration*>(stmt));
            break;
        
        case ASTNode::NodeType::AssignmentStatement:
            _emit_assignment(static_cast<const AssignmentStatement*>(stmt));
            break;
        
        case ASTNode::NodeType::IfStatement:
            _emit_if_statement(static_cast<const IfStatement*>(stmt));
            break;
        
        // TODO: Add other statement types
        default:
            break;
    }
    
    // Clean up registers after statement (simple liveness: expressions die at statement end)
    // For now, we keep _expr_to_reg entries for potential reuse, but could clear here
    // This is a simple approach - more sophisticated would track liveness
}

void ASTToRISCVEmitter::_emit_expression(const ExpressionNode* expr) {
    if (!expr || !_assembler) {
        return;
    }
    
    switch (expr->get_type()) {
        case ASTNode::NodeType::LiteralExpr:
            _emit_literal(static_cast<const LiteralExpr*>(expr));
            break;
        
        case ASTNode::NodeType::IdentifierExpr:
            _emit_identifier(static_cast<const IdentifierExpr*>(expr));
            break;
        
        case ASTNode::NodeType::BinaryOpExpr:
            _emit_binary_op(static_cast<const BinaryOpExpr*>(expr));
            break;
        
        case ASTNode::NodeType::CallExpr:
            _emit_call(static_cast<const CallExpr*>(expr));
            break;
        
        // TODO: Add other expression types
        default:
            break;
    }
}

void ASTToRISCVEmitter::_emit_literal(const LiteralExpr* lit) {
    if (!lit || !_assembler) {
        return;
    }
    
    biscuit::GPR reg = _allocate_register();
    
    // Extract value from variant
    if (std::holds_alternative<int64_t>(lit->value)) {
        int64_t val = std::get<int64_t>(lit->value);
        _assembler->LI(reg, val);
    } else if (std::holds_alternative<double>(lit->value)) {
        // TODO: Handle float literals (for now, convert to int)
        double val = std::get<double>(lit->value);
        _assembler->LI(reg, static_cast<int64_t>(val));
    } else if (std::holds_alternative<bool>(lit->value)) {
        bool val = std::get<bool>(lit->value);
        _assembler->LI(reg, val ? 1 : 0);
    } else if (std::holds_alternative<std::string>(lit->value)) {
        // TODO: Handle string literals
        _assembler->LI(reg, 0);
    } else {
        // Null literal
        _assembler->LI(reg, 0);
    }
    
    // Track register for this expression
    _expr_to_reg[lit] = reg;
}

void ASTToRISCVEmitter::_emit_identifier(const IdentifierExpr* ident) {
    if (!ident || !_assembler) {
        return;
    }
    
    std::string varName = ident->name;
    int offset = _get_var_stack_offset(varName);
    biscuit::GPR reg = _allocate_register();
    
    // Load variable from stack
    if (offset >= 0) {
        _assembler->LD(reg, offset, biscuit::sp);
    } else {
        // Variable not found - this is an error case
        _assembler->LI(reg, 0);
    }
    
    // Track register for this expression
    _expr_to_reg[ident] = reg;
}

void ASTToRISCVEmitter::_emit_call(const CallExpr* call) {
    if (!call || !_assembler || !_current_function) {
        return;
    }
    
    // For now, stub out function calls - return 0
    // TODO: Implement proper function call support using function registry
    biscuit::GPR result_reg = _allocate_register();
    _assembler->LI(result_reg, 0);
    
    // Map the call expression to the result register
    _expr_to_reg[call] = result_reg;
}

void ASTToRISCVEmitter::_emit_binary_op(const BinaryOpExpr* binop) {
    if (!binop || !binop->left || !binop->right || !_assembler) {
        return;
    }
    
    // Emit left and right operands
    _emit_expression(binop->left.get());
    _emit_expression(binop->right.get());
    
    // Get registers for operands
    biscuit::GPR leftReg = _get_or_allocate_register(binop->left.get());
    biscuit::GPR rightReg = _get_or_allocate_register(binop->right.get());
    biscuit::GPR resultReg = _allocate_register();
    
    // Emit operation based on operator
    if (binop->op == "+") {
        _assembler->ADD(resultReg, leftReg, rightReg);
    } else if (binop->op == "-") {
        _assembler->SUB(resultReg, leftReg, rightReg);
    } else if (binop->op == "*") {
        _assembler->MUL(resultReg, leftReg, rightReg);
    } else if (binop->op == "/") {
        _assembler->DIV(resultReg, leftReg, rightReg);
    } else if (binop->op == "%") {
        _assembler->REM(resultReg, leftReg, rightReg);
    } else if (binop->op == "==") {
        // Equality: XOR the operands, result is 0 if equal
        // Then set result to 1 if XOR result is 0, else 0
        _assembler->XOR(resultReg, leftReg, rightReg);
        // If resultReg == 0, set resultReg = 1, else resultReg = 0
        // Use: SLTIU resultReg, resultReg, 1 (sets to 1 if resultReg < 1, i.e., if resultReg == 0)
        _assembler->SLTIU(resultReg, resultReg, 1);
    } else if (binop->op == "!=") {
        // Not equal: XOR the operands, result is non-zero if not equal
        // Then set result to 1 if XOR result != 0, else 0
        _assembler->XOR(resultReg, leftReg, rightReg);
        // If resultReg != 0, set resultReg = 1, else resultReg = 0
        // Use: SLTU resultReg, zero, resultReg (sets to 1 if 0 < resultReg, i.e., if resultReg != 0)
        _assembler->SLTU(resultReg, biscuit::zero, resultReg);
    } else if (binop->op == "<") {
        // Less than (signed)
        _assembler->SLT(resultReg, leftReg, rightReg);
    } else if (binop->op == ">") {
        // Greater than (signed): swap operands for SLT
        _assembler->SLT(resultReg, rightReg, leftReg);
    } else if (binop->op == "<=") {
        // Less than or equal: !(right < left), i.e., !(left > right)
        // Use: SLT temp, rightReg, leftReg, then XOR with 1
        biscuit::GPR tempReg = _allocate_register();
        _assembler->SLT(tempReg, rightReg, leftReg);
        // Invert: resultReg = 1 - tempReg, or use XOR with 1
        _assembler->XORI(resultReg, tempReg, 1);
    } else if (binop->op == ">=") {
        // Greater than or equal: !(left < right)
        // Use: SLT temp, leftReg, rightReg, then XOR with 1
        biscuit::GPR tempReg = _allocate_register();
        _assembler->SLT(tempReg, leftReg, rightReg);
        // Invert: resultReg = 1 - tempReg
        _assembler->XORI(resultReg, tempReg, 1);
    } else {
        // Unknown operator - default to 0
        _assembler->LI(resultReg, 0);
    }
    
    // Store result register for this expression
    _expr_to_reg[binop] = resultReg;
}

void ASTToRISCVEmitter::_emit_return(const ReturnStatement* ret) {
    if (!ret || !_assembler || !_current_function) {
        return;
    }
    
    if (ret->value) {
        // Emit return value expression
        _emit_expression(ret->value.get());
        
        // Get register for return value
        biscuit::GPR retReg = _get_or_allocate_register(ret->value.get());
        
        // Move to a0 (return value register)
        if (retReg.Index() != biscuit::a0.Index()) {
            _assembler->ADD(biscuit::a0, retReg, biscuit::zero);
        }
    }
    
    // For entry point functions, exit via syscall instead of returning
    // Linux syscall: exit_group(a0) - syscall 94 (0x5e) for riscv64
    // a0 already contains the return value
    _assembler->LI(biscuit::a7, 94);  // syscall number for exit_group
    _assembler->ECALL();               // Make syscall (exits program)
}

void ASTToRISCVEmitter::_emit_assignment(const AssignmentStatement* assign) {
    if (!assign || !_assembler || !_current_function) {
        return;
    }
    
    // Get target variable name (must be IdentifierExpr)
    if (!assign->target || assign->target->get_type() != ASTNode::NodeType::IdentifierExpr) {
        return; // Only support simple variable assignments for now
    }
    
    const IdentifierExpr* target_ident = static_cast<const IdentifierExpr*>(assign->target.get());
    std::string var_name = target_ident->name;
    
    // Emit value expression
    if (assign->value) {
        _emit_expression(assign->value.get());
        biscuit::GPR value_reg = _get_or_allocate_register(assign->value.get());
        
        // Get or allocate stack offset for variable
        int stack_offset = _get_var_stack_offset(var_name);
        if (stack_offset < 0) {
            // Variable not found, allocate new stack slot
            stack_offset = _allocate_stack(var_name);
        }
        
        // Store value to variable's stack location
        _assembler->SD(value_reg, stack_offset, biscuit::sp);
        
        // Update expression-to-register mapping for the target variable
        // This allows the variable to be used in subsequent expressions
        // Note: We need to create a temporary IdentifierExpr for lookup
        // For now, we'll just store the register mapping by variable name
        // This is a simplification - proper implementation would track by expression node
    }
}

void ASTToRISCVEmitter::_emit_if_statement(const IfStatement* if_stmt) {
    if (!if_stmt || !_assembler || !_current_function) {
        return;
    }
    
    // Emit condition expression
    if (if_stmt->condition) {
        _emit_expression(if_stmt->condition.get());
        biscuit::GPR cond_reg = _get_or_allocate_register(if_stmt->condition.get());
        
        // Create labels
        auto else_label = std::make_unique<biscuit::Label>();
        auto end_label = std::make_unique<biscuit::Label>();
        biscuit::Label* else_ptr = else_label.get();
        biscuit::Label* end_ptr = end_label.get();
        _labels.push_back(std::move(else_label));
        _labels.push_back(std::move(end_label));
        
        // Branch if condition is false (0) - jump to else/end
        _assembler->BEQZ(cond_reg, else_ptr);
        
        // Emit then body
        for (const auto& stmt : if_stmt->then_body) {
            _emit_statement(stmt.get());
        }
        
        // Jump to end (skip else)
        _assembler->JAL(end_ptr);
        
        // Bind else label
        _assembler->Bind(else_ptr);
        
        // Emit elif branches
        for (const auto& branch : if_stmt->elif_branches) {
            auto elif_else_label = std::make_unique<biscuit::Label>();
            auto elif_end_label = std::make_unique<biscuit::Label>();
            biscuit::Label* elif_else_ptr = elif_else_label.get();
            biscuit::Label* elif_end_ptr = elif_end_label.get();
            _labels.push_back(std::move(elif_else_label));
            _labels.push_back(std::move(elif_end_label));
            
            // Emit elif condition
            if (branch.first) {
                _emit_expression(branch.first.get());
                biscuit::GPR elif_cond_reg = _get_or_allocate_register(branch.first.get());
                _assembler->BEQZ(elif_cond_reg, elif_else_ptr);
            }
            
            // Emit elif body
            for (const auto& stmt : branch.second) {
                _emit_statement(stmt.get());
            }
            
            // Jump to end
            _assembler->JAL(elif_end_ptr);
            
            // Bind elif else label
            _assembler->Bind(elif_else_ptr);
            
            // Update else_ptr for next elif
            else_ptr = elif_end_ptr;
        }
        
        // Emit else body
        for (const auto& stmt : if_stmt->else_body) {
            _emit_statement(stmt.get());
        }
        
        // Bind end label
        _assembler->Bind(end_ptr);
    }
}

void ASTToRISCVEmitter::_emit_variable_declaration(const VariableDeclaration* varDecl) {
    if (!varDecl || !_assembler) {
        return;
    }
    
    if (varDecl->initializer) {
        // Emit initializer expression
        _emit_expression(varDecl->initializer.get());
        
        // Get register for initializer value
        biscuit::GPR valueReg = _get_or_allocate_register(varDecl->initializer.get());
        
        // Allocate stack slot for variable
        int varOffset = _allocate_stack(varDecl->name);
        
        // Store value to variable location
        _assembler->SD(valueReg, varOffset, biscuit::sp);
    } else {
        // No initializer - just allocate stack slot
        _allocate_stack(varDecl->name);
    }
}

biscuit::GPR ASTToRISCVEmitter::_allocate_register() {
    if (_temp_reg_index < _num_temp_regs) {
        biscuit::GPR reg = _temp_regs[_temp_reg_index];
        _temp_reg_index = (_temp_reg_index + 1) % _num_temp_regs;
        return reg;
    }
    
    // Out of registers - use stack (allocate a temp slot)
    std::string tempName = "_temp_" + std::to_string(_stack_offset);
    _allocate_stack(tempName);
    // Return a register that will be used to load from stack
    return _temp_regs[0]; // Will need special handling
}

int ASTToRISCVEmitter::_allocate_stack(const std::string& varName) {
    // Check if variable already allocated
    if (_var_to_stack_offset.find(varName) != _var_to_stack_offset.end()) {
        return _var_to_stack_offset[varName];
    }
    
    // Allocate new stack slot (8 bytes for 64-bit values)
    int offset = 16 + _stack_offset; // Start after saved registers
    _var_to_stack_offset[varName] = offset;
    _stack_offset += 8;
    
    // Update function stack size if needed (track maximum)
    // Note: We can't change the prologue after it's emitted, but we track for epilogue
    int neededStackSize = 16 + _stack_offset;
    if (neededStackSize > _current_function_stack_size) {
        _current_function_stack_size = neededStackSize;
    }
    
    return offset;
}

biscuit::GPR ASTToRISCVEmitter::_get_or_allocate_register(const ExpressionNode* expr) {
    auto it = _expr_to_reg.find(expr);
    if (it != _expr_to_reg.end()) {
        return it->second;
    }
    // Expression not found - allocate a new register
    // This shouldn't happen if _emit_expression was called first
    return _allocate_register();
}

int ASTToRISCVEmitter::_get_var_stack_offset(const std::string& varName) {
    auto it = _var_to_stack_offset.find(varName);
    if (it != _var_to_stack_offset.end()) {
        return it->second;
    }
    return -1; // Error case
}

void ASTToRISCVEmitter::_reset_function_state() {
    _expr_to_reg.clear();
    // Keep _var_to_stack_offset for function scope (contains function parameters)
    // Reset _stack_offset to account for parameters already stored
    // Parameters are stored starting at offset 16, so _stack_offset should start after them
    _stack_offset = 0; // Will be set based on parameters in _emit_function
    _current_function_stack_size = 16; // Reset to minimum (saved regs only)
    _temp_reg_index = 0;
}

} // namespace gdscript

