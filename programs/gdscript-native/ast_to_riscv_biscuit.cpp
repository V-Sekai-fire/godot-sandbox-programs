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
    int stackSize = 16 + paramStackSize; // Saved regs + parameters
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
    
    // Emit function body
    for (const auto& stmt : func->body) {
        emitStatement(stmt.get());
    }
    
    // Function epilogue (fallback if no explicit return)
    // RISC-V 64 Linux: return value should be in a0
    assembler->LI(biscuit::GPR::A0, 0);  // Default return value 0
    assembler->LD(biscuit::GPR::RA, biscuit::GPR::SP, currentFunctionStackSize - 8);
    assembler->LD(biscuit::GPR::S0, biscuit::GPR::SP, currentFunctionStackSize - 16);
    assembler->ADDI(biscuit::GPR::SP, biscuit::GPR::SP, currentFunctionStackSize);
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
    } else {
        // TODO: Handle comparison and logical operators
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
    
    // Function epilogue - use the tracked stack size
    int actualStackSize = currentFunctionStackSize;
    if (stackOffset > currentFunctionStackSize - 16) {
        actualStackSize = 16 + (stackOffset - 16);
    }
    
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
    if (varToStackOffset.find(varName) == varToStackOffset.end()) {
        varToStackOffset[varName] = stackOffset;
        stackOffset += 8; // 64-bit values
        // Update function stack size if needed
        if (stackOffset > currentFunctionStackSize - 16) {
            currentFunctionStackSize = 16 + stackOffset;
        }
    }
    return varToStackOffset[varName];
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
    stackOffset = 16; // Start after saved registers (ra, s0)
    currentFunctionStackSize = 16; // Reset to minimum
    tempRegIndex = 0;
}

} // namespace gdscript

