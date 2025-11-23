#include "ast_to_riscv_biscuit.h"
#include <cstring>

namespace gdscript {

std::pair<const uint8_t*, size_t> ASTToRISCVEmitter::emit(const ProgramNode* program) {
    clear();
    
    if (!program || program->functions.empty()) {
        return {nullptr, 0};
    }
    
    // Initialize biscuit assembler with our buffer
    // Biscuit assembler takes buffer and size, similar to AsmJit
    assembler = std::make_unique<biscuit::Assembler>(codeBuffer.data(), codeBuffer.size());
    
    // Emit all functions
    for (const auto& func : program->functions) {
        emitFunction(func.get());
        
        // Check if buffer needs to grow
        size_t currentOffset = assembler->GetCursorOffset();
        if (currentOffset >= codeBuffer.size() * 0.9) { // 90% full
            // Grow buffer by 2x
            size_t oldSize = codeBuffer.size();
            size_t newSize = codeBuffer.size() * 2;
            codeBuffer.resize(newSize);
            
            // Preserve existing code and recreate assembler
            // The existing code is already in codeBuffer, we just need to update the assembler
            // Note: biscuit assembler writes directly to buffer, so existing code is preserved
            assembler = std::make_unique<biscuit::Assembler>(codeBuffer.data() + currentOffset, 
                                                              codeBuffer.size() - currentOffset);
        }
    }
    
    // Get the actual size of generated code
    // Biscuit writes directly to buffer, GetCursorOffset() returns current position
    size_t codeSize = assembler->GetCursorOffset();
    
    assembler.reset();
    
    return {codeBuffer.data(), codeSize};
}

void ASTToRISCVEmitter::emitFunction(const FunctionNode* func) {
    if (!func || !assembler) {
        return;
    }
    
    resetFunctionState();
    
    // RISC-V 64 Linux function prologue
    // Stack frame layout (RISC-V 64 ABI):
    // - sp+0: saved frame pointer (s0/fp)
    // - sp+8: saved return address (ra)
    // - sp+16+: local variables and spilled registers
    int paramStackSize = static_cast<int>(func->parameters.size() * 8);
    // Start with minimum: saved regs (16) + parameters
    // Will grow dynamically as variables are allocated
    int initialStackSize = 16 + paramStackSize;
    // Allocate extra space for local variables (conservative estimate)
    // We'll track actual usage and adjust in epilogue
    int estimatedLocalVars = 64; // Estimate 8 local variables (8 bytes each)
    int stackSize = initialStackSize + estimatedLocalVars;
    currentFunctionStackSize = stackSize;
    
    // Function prologue
    assembler->ADDI(biscuit::GPR::SP, biscuit::GPR::SP, -stackSize);
    assembler->SD(biscuit::GPR::RA, biscuit::GPR::SP, stackSize - 8);
    assembler->SD(biscuit::GPR::S0, biscuit::GPR::SP, stackSize - 16);
    assembler->ADDI(biscuit::GPR::S0, biscuit::GPR::SP, stackSize);
    
    // RISC-V 64 Linux calling convention:
    // - Arguments: a0-a7 (x10-x17) for first 8 arguments
    // - Return value: a0 (x10)
    // Store function arguments from registers to stack slots
    biscuit::GPR argRegs[] = {
        biscuit::GPR::A0, biscuit::GPR::A1, biscuit::GPR::A2, biscuit::GPR::A3,
        biscuit::GPR::A4, biscuit::GPR::A5, biscuit::GPR::A6, biscuit::GPR::A7
    };
    
    for (size_t i = 0; i < func->parameters.size() && i < 8; ++i) {
        const auto& param = func->parameters[i];
        std::string varName = param.first;
        // Arguments arrive in a0-a7, store them to stack for variable access
        int offset = 16 + static_cast<int>(i * 8); // Start after saved registers
        assembler->SD(argRegs[i], biscuit::GPR::SP, offset);
        varToStackOffset[varName] = offset;
    }
    
    // Update stackOffset to account for parameters
    stackOffset = static_cast<int>(func->parameters.size() * 8);
    
    // Emit function body
    for (const auto& stmt : func->body) {
        emitStatement(stmt.get());
    }
    
    // Function epilogue (fallback if no explicit return)
    // RISC-V 64 Linux: return value should be in a0
    assembler->LI(biscuit::GPR::A0, 0);  // Default return value 0
    
    // Calculate actual stack size needed (same logic as emitReturn)
    int paramStackSize = static_cast<int>(func->parameters.size() * 8);
    int actualStackNeeded = 16 + paramStackSize;
    if (stackOffset > paramStackSize) {
        actualStackNeeded = 16 + stackOffset;
    }
    int actualStackSize = (actualStackNeeded > currentFunctionStackSize) ? actualStackNeeded : currentFunctionStackSize;
    
    assembler->LD(biscuit::GPR::RA, biscuit::GPR::SP, actualStackSize - 8);
    assembler->LD(biscuit::GPR::S0, biscuit::GPR::SP, actualStackSize - 16);
    assembler->ADDI(biscuit::GPR::SP, biscuit::GPR::SP, actualStackSize);
    assembler->RET();
}

void ASTToRISCVEmitter::emitStatement(const StatementNode* stmt) {
    if (!stmt || !assembler) {
        return;
    }
    
    switch (stmt->getType()) {
        case ASTNode::NodeType::ReturnStatement:
            emitReturn(static_cast<const ReturnStatement*>(stmt));
            break;
        
        case ASTNode::NodeType::VariableDeclaration:
            emitVariableDeclaration(static_cast<const VariableDeclaration*>(stmt));
            break;
        
        // TODO: Add other statement types
        default:
            break;
    }
    
    // Clean up registers after statement (simple liveness: expressions die at statement end)
    // For now, we keep exprToReg entries for potential reuse, but could clear here
    // This is a simple approach - more sophisticated would track liveness
}

void ASTToRISCVEmitter::emitExpression(const ExpressionNode* expr) {
    if (!expr || !assembler) {
        return;
    }
    
    switch (expr->getType()) {
        case ASTNode::NodeType::LiteralExpr:
            emitLiteral(static_cast<const LiteralExpr*>(expr));
            break;
        
        case ASTNode::NodeType::IdentifierExpr:
            emitIdentifier(static_cast<const IdentifierExpr*>(expr));
            break;
        
        case ASTNode::NodeType::BinaryOpExpr:
            emitBinaryOp(static_cast<const BinaryOpExpr*>(expr));
            break;
        
        // TODO: Add other expression types
        default:
            break;
    }
}

void ASTToRISCVEmitter::emitLiteral(const LiteralExpr* lit) {
    if (!lit || !assembler) {
        return;
    }
    
    biscuit::GPR reg = allocateRegister();
    
    // Extract value from variant
    if (std::holds_alternative<int64_t>(lit->value)) {
        int64_t val = std::get<int64_t>(lit->value);
        assembler->LI(reg, val);
    } else if (std::holds_alternative<double>(lit->value)) {
        // TODO: Handle float literals (for now, convert to int)
        double val = std::get<double>(lit->value);
        assembler->LI(reg, static_cast<int64_t>(val));
    } else if (std::holds_alternative<bool>(lit->value)) {
        bool val = std::get<bool>(lit->value);
        assembler->LI(reg, val ? 1 : 0);
    } else if (std::holds_alternative<std::string>(lit->value)) {
        // TODO: Handle string literals
        assembler->LI(reg, 0);
    } else {
        // Null literal
        assembler->LI(reg, 0);
    }
    
    // Track register for this expression
    exprToReg[lit] = reg;
}

void ASTToRISCVEmitter::emitIdentifier(const IdentifierExpr* ident) {
    if (!ident || !assembler) {
        return;
    }
    
    std::string varName = ident->name;
    int offset = getVarStackOffset(varName);
    biscuit::GPR reg = allocateRegister();
    
    // Load variable from stack
    if (offset >= 0) {
        assembler->LD(reg, biscuit::GPR::SP, offset);
    } else {
        // Variable not found - this is an error case
        assembler->LI(reg, 0);
    }
    
    // Track register for this expression
    exprToReg[ident] = reg;
}

void ASTToRISCVEmitter::emitBinaryOp(const BinaryOpExpr* binop) {
    if (!binop || !binop->left || !binop->right || !assembler) {
        return;
    }
    
    // Emit left and right operands
    emitExpression(binop->left.get());
    emitExpression(binop->right.get());
    
    // Get registers for operands
    biscuit::GPR leftReg = getOrAllocateRegister(binop->left.get());
    biscuit::GPR rightReg = getOrAllocateRegister(binop->right.get());
    biscuit::GPR resultReg = allocateRegister();
    
    // Emit operation based on operator
    if (binop->op == "+") {
        assembler->ADD(resultReg, leftReg, rightReg);
    } else if (binop->op == "-") {
        assembler->SUB(resultReg, leftReg, rightReg);
    } else if (binop->op == "*") {
        assembler->MUL(resultReg, leftReg, rightReg);
    } else if (binop->op == "/") {
        assembler->DIV(resultReg, leftReg, rightReg);
    } else if (binop->op == "%") {
        assembler->REM(resultReg, leftReg, rightReg);
    } else if (binop->op == "==") {
        // Equality: XOR the operands, result is 0 if equal
        // Then set result to 1 if XOR result is 0, else 0
        assembler->XOR(resultReg, leftReg, rightReg);
        // If resultReg == 0, set resultReg = 1, else resultReg = 0
        // Use: SLTIU resultReg, resultReg, 1 (sets to 1 if resultReg < 1, i.e., if resultReg == 0)
        assembler->SLTIU(resultReg, resultReg, 1);
    } else if (binop->op == "!=") {
        // Not equal: XOR the operands, result is non-zero if not equal
        // Then set result to 1 if XOR result != 0, else 0
        assembler->XOR(resultReg, leftReg, rightReg);
        // If resultReg != 0, set resultReg = 1, else resultReg = 0
        // Use: SLTU resultReg, zero, resultReg (sets to 1 if 0 < resultReg, i.e., if resultReg != 0)
        assembler->SLTU(resultReg, biscuit::GPR::ZERO, resultReg);
    } else if (binop->op == "<") {
        // Less than (signed)
        assembler->SLT(resultReg, leftReg, rightReg);
    } else if (binop->op == ">") {
        // Greater than (signed): swap operands for SLT
        assembler->SLT(resultReg, rightReg, leftReg);
    } else if (binop->op == "<=") {
        // Less than or equal: !(right < left), i.e., !(left > right)
        // Use: SLT temp, rightReg, leftReg, then XOR with 1
        biscuit::GPR tempReg = allocateRegister();
        assembler->SLT(tempReg, rightReg, leftReg);
        // Invert: resultReg = 1 - tempReg, or use XOR with 1
        assembler->XORI(resultReg, tempReg, 1);
    } else if (binop->op == ">=") {
        // Greater than or equal: !(left < right)
        // Use: SLT temp, leftReg, rightReg, then XOR with 1
        biscuit::GPR tempReg = allocateRegister();
        assembler->SLT(tempReg, leftReg, rightReg);
        // Invert: resultReg = 1 - tempReg
        assembler->XORI(resultReg, tempReg, 1);
    } else {
        // Unknown operator - default to 0
        assembler->LI(resultReg, 0);
    }
    
    // Store result register for this expression
    exprToReg[binop] = resultReg;
}

void ASTToRISCVEmitter::emitReturn(const ReturnStatement* ret) {
    if (!ret || !assembler) {
        return;
    }
    
    if (ret->value) {
        // Emit return value expression
        emitExpression(ret->value.get());
        
        // Get register for return value
        biscuit::GPR retReg = getOrAllocateRegister(ret->value.get());
        
        // Move to a0 (return value register)
        if (retReg != biscuit::GPR::A0) {
            assembler->ADD(biscuit::GPR::A0, retReg, biscuit::GPR::ZERO);
        }
    }
    
    // Function epilogue - use the maximum of initial size and actual usage
    // Calculate actual stack needed: saved regs (16) + max(stackOffset, paramStackSize)
    int paramStackSize = static_cast<int>(func->parameters.size() * 8);
    int actualStackNeeded = 16 + paramStackSize;
    if (stackOffset > paramStackSize) {
        actualStackNeeded = 16 + stackOffset;
    }
    // Ensure we use at least the allocated size (can't shrink, but can use full allocation)
    int actualStackSize = (actualStackNeeded > currentFunctionStackSize) ? actualStackNeeded : currentFunctionStackSize;
    
    assembler->LD(biscuit::GPR::RA, biscuit::GPR::SP, actualStackSize - 8);
    assembler->LD(biscuit::GPR::S0, biscuit::GPR::SP, actualStackSize - 16);
    assembler->ADDI(biscuit::GPR::SP, biscuit::GPR::SP, actualStackSize);
    assembler->RET();
}

void ASTToRISCVEmitter::emitVariableDeclaration(const VariableDeclaration* varDecl) {
    if (!varDecl || !assembler) {
        return;
    }
    
    if (varDecl->initializer) {
        // Emit initializer expression
        emitExpression(varDecl->initializer.get());
        
        // Get register for initializer value
        biscuit::GPR valueReg = getOrAllocateRegister(varDecl->initializer.get());
        
        // Allocate stack slot for variable
        int varOffset = allocateStack(varDecl->name);
        
        // Store value to variable location
        assembler->SD(valueReg, biscuit::GPR::SP, varOffset);
    } else {
        // No initializer - just allocate stack slot
        allocateStack(varDecl->name);
    }
}

biscuit::GPR ASTToRISCVEmitter::allocateRegister() {
    if (tempRegIndex < numTempRegs) {
        biscuit::GPR reg = tempRegs[tempRegIndex];
        tempRegIndex = (tempRegIndex + 1) % numTempRegs;
        return reg;
    }
    
    // Out of registers - use stack (allocate a temp slot)
    std::string tempName = "_temp_" + std::to_string(stackOffset);
    allocateStack(tempName);
    // Return a register that will be used to load from stack
    return tempRegs[0]; // Will need special handling
}

int ASTToRISCVEmitter::allocateStack(const std::string& varName) {
    // Check if variable already allocated
    if (varToStackOffset.find(varName) != varToStackOffset.end()) {
        return varToStackOffset[varName];
    }
    
    // Allocate new stack slot (8 bytes for 64-bit values)
    int offset = 16 + stackOffset; // Start after saved registers
    varToStackOffset[varName] = offset;
    stackOffset += 8;
    
    // Update function stack size if needed (track maximum)
    // Note: We can't change the prologue after it's emitted, but we track for epilogue
    int neededStackSize = 16 + stackOffset;
    if (neededStackSize > currentFunctionStackSize) {
        currentFunctionStackSize = neededStackSize;
    }
    
    return offset;
}

biscuit::GPR ASTToRISCVEmitter::getOrAllocateRegister(const ExpressionNode* expr) {
    auto it = exprToReg.find(expr);
    if (it != exprToReg.end()) {
        return it->second;
    }
    // Expression not found - allocate a new register
    // This shouldn't happen if emitExpression was called first
    return allocateRegister();
}

int ASTToRISCVEmitter::getVarStackOffset(const std::string& varName) {
    auto it = varToStackOffset.find(varName);
    if (it != varToStackOffset.end()) {
        return it->second;
    }
    return -1; // Error case
}

void ASTToRISCVEmitter::resetFunctionState() {
    exprToReg.clear();
    // Keep varToStackOffset for function scope (contains function parameters)
    // Reset stackOffset to account for parameters already stored
    // Parameters are stored starting at offset 16, so stackOffset should start after them
    stackOffset = 0; // Will be set based on parameters in emitFunction
    currentFunctionStackSize = 16; // Reset to minimum (saved regs only)
    tempRegIndex = 0;
}

} // namespace gdscript

