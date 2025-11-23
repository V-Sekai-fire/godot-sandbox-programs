#include "ast_to_riscv.h"
#include <sstream>
#include <iomanip>

namespace gdscript {

std::string ASTToRISCVEmitter::emit(const ProgramNode* program) {
    clear();
    
    if (!program) {
        return "";
    }
    
    // RISC-V 64 Linux assembly header
    asmCode << ".option pic\n";  // Position-independent code
    asmCode << ".text\n";
    asmCode << ".align 2\n";     // Align to 4-byte boundary (RISC-V requirement)
    asmCode << "\n";
    
    // Emit all functions
    for (const auto& func : program->functions) {
        asmCode << emitFunction(func.get());
        asmCode << "\n";
    }
    
    return asmCode.str();
}

std::string ASTToRISCVEmitter::emitProgram(const ProgramNode* program) {
    // This is handled by emit() - kept for consistency
    return "";
}

std::string ASTToRISCVEmitter::emitFunction(const FunctionNode* func) {
    if (!func) {
        return "";
    }
    
    resetFunctionState();
    
    std::ostringstream funcAsm;
    
    std::string funcName = func->name;
    funcAsm << ".globl " << funcName << "\n";
    funcAsm << ".type " << funcName << ", @function\n";
    funcAsm << funcName << ":\n";
    
    // RISC-V 64 Linux function prologue
    // Stack frame layout (RISC-V 64 ABI):
    // - sp+0: saved frame pointer (s0/fp)
    // - sp+8: saved return address (ra)
    // - sp+16+: local variables and spilled registers
    // Calculate total stack size needed:
    // - 16 bytes for saved registers (ra, s0)
    // - 8 bytes per function parameter (if we need to store them)
    int paramStackSize = static_cast<int>(func->parameters.size() * 8);
    int stackSize = 16 + paramStackSize; // Saved regs + parameters
    currentFunctionStackSize = stackSize; // Track for epilogue
    
    funcAsm << "    addi sp, sp, -" << stackSize << "\n";
    funcAsm << "    sd ra, " << (stackSize - 8) << "(sp)\n";
    funcAsm << "    sd s0, " << (stackSize - 16) << "(sp)\n";
    funcAsm << "    addi s0, sp, " << stackSize << "\n";
    
    // RISC-V 64 Linux calling convention:
    // - Arguments: a0-a7 (x10-x17) for first 8 arguments
    // - Return value: a0 (x10)
    // - Stack arguments: sp+0, sp+8, ... (if more than 8 args)
    // Store function arguments from registers to stack slots
    for (size_t i = 0; i < func->parameters.size() && i < 8; ++i) {
        const auto& param = func->parameters[i];
        std::string varName = param.first;
        // Arguments arrive in a0-a7, store them to stack for variable access
        int offset = 16 + static_cast<int>(i * 8); // Start after saved registers
        funcAsm << "    sd a" << i << ", " << offset << "(sp)\n";
        varToStackOffset[varName] = offset;
    }
    
    // Emit function body
    for (const auto& stmt : func->body) {
        funcAsm << emitStatement(stmt.get());
        // Update stack size if we allocated more stack space
        if (stackOffset > currentFunctionStackSize - 16) {
            int neededSize = 16 + (stackOffset - 16);
            if (neededSize > currentFunctionStackSize) {
                // Would need to adjust stack, but for now we'll handle in epilogue
                currentFunctionStackSize = std::max(currentFunctionStackSize, neededSize);
            }
        }
    }
    
    // Function epilogue (fallback if no explicit return)
    // RISC-V 64 Linux: return value should be in a0
    std::string epilogueLabel = funcName + "_epilogue";
    funcAsm << epilogueLabel << ":\n";
    funcAsm << "    li a0, 0\n";  // Default return value 0
    // Use the actual stack size we calculated
    int finalStackSize = std::max(currentFunctionStackSize, 16 + (stackOffset - 16));
    funcAsm << "    ld ra, " << (finalStackSize - 8) << "(sp)\n";
    funcAsm << "    ld s0, " << (finalStackSize - 16) << "(sp)\n";
    funcAsm << "    addi sp, sp, " << finalStackSize << "\n";
    funcAsm << "    ret\n";
    funcAsm << ".size " << funcName << ", .-" << funcName << "\n";
    
    return funcAsm.str();
}

std::string ASTToRISCVEmitter::emitStatement(const StatementNode* stmt) {
    if (!stmt) {
        return "";
    }
    
    switch (stmt->getType()) {
        case ASTNode::NodeType::ReturnStatement:
            return emitReturn(static_cast<const ReturnStatement*>(stmt));
        
        case ASTNode::NodeType::VariableDeclaration:
            return emitVariableDeclaration(static_cast<const VariableDeclaration*>(stmt));
        
        // TODO: Add other statement types
        default:
            return "    # Unsupported statement type\n";
    }
}

std::string ASTToRISCVEmitter::emitExpression(const ExpressionNode* expr) {
    if (!expr) {
        return "";
    }
    
    switch (expr->getType()) {
        case ASTNode::NodeType::LiteralExpr:
            return emitLiteral(static_cast<const LiteralExpr*>(expr));
        
        case ASTNode::NodeType::IdentifierExpr:
            return emitIdentifier(static_cast<const IdentifierExpr*>(expr));
        
        case ASTNode::NodeType::BinaryOpExpr:
            return emitBinaryOp(static_cast<const BinaryOpExpr*>(expr));
        
        // TODO: Add other expression types
        default:
            return "    # Unsupported expression type\n";
    }
}

std::string ASTToRISCVEmitter::emitLiteral(const LiteralExpr* lit) {
    if (!lit) {
        return "";
    }
    
    std::string reg = allocateRegister();
    std::ostringstream oss;
    
    // Extract value from variant
    if (std::holds_alternative<int64_t>(lit->value)) {
        int64_t val = std::get<int64_t>(lit->value);
        oss << "    li " << reg << ", " << val << "\n";
    } else if (std::holds_alternative<double>(lit->value)) {
        // TODO: Handle float literals (for now, convert to int)
        double val = std::get<double>(lit->value);
        oss << "    li " << reg << ", " << static_cast<int64_t>(val) << "\n";
    } else if (std::holds_alternative<bool>(lit->value)) {
        bool val = std::get<bool>(lit->value);
        oss << "    li " << reg << ", " << (val ? 1 : 0) << "\n";
    } else if (std::holds_alternative<std::string>(lit->value)) {
        // TODO: Handle string literals
        oss << "    # String literal not yet implemented\n";
    } else {
        oss << "    # Null literal\n";
        oss << "    li " << reg << ", 0\n";
    }
    
    // Track register for this expression
    exprToReg[lit] = reg;
    
    return oss.str();
}

std::string ASTToRISCVEmitter::emitIdentifier(const IdentifierExpr* ident) {
    if (!ident) {
        return "";
    }
    
    std::string varName = ident->name;
    std::string location = getVarLocation(varName);
    std::string reg = allocateRegister();
    std::ostringstream oss;
    
    // Load variable from stack
    if (location.find("(sp)") != std::string::npos) {
        oss << "    ld " << reg << ", " << location << "\n";
    } else {
        // Variable not found - this is an error case
        oss << "    # Error: Variable '" << varName << "' not found\n";
        oss << "    li " << reg << ", 0\n";
    }
    
    // Track register for this expression
    exprToReg[ident] = reg;
    
    return oss.str();
}

std::string ASTToRISCVEmitter::emitBinaryOp(const BinaryOpExpr* binop) {
    if (!binop || !binop->left || !binop->right) {
        return "";
    }
    
    std::ostringstream oss;
    
    // Emit left and right operands
    std::string leftCode = emitExpression(binop->left.get());
    std::string rightCode = emitExpression(binop->right.get());
    
    oss << leftCode;
    oss << rightCode;
    
    // Get registers for operands (they should have been allocated)
    std::string leftReg = getOrAllocateRegister(binop->left.get());
    std::string rightReg = getOrAllocateRegister(binop->right.get());
    std::string resultReg = allocateRegister();
    
    // Handle stack-based operands
    if (leftReg.find("(sp)") != std::string::npos) {
        oss << "    ld t0, " << leftReg << "\n";
        leftReg = "t0";
    }
    if (rightReg.find("(sp)") != std::string::npos) {
        oss << "    ld t1, " << rightReg << "\n";
        rightReg = "t1";
    }
    
    // Emit operation based on operator
    if (binop->op == "+") {
        oss << "    add " << resultReg << ", " << leftReg << ", " << rightReg << "\n";
    } else if (binop->op == "-") {
        oss << "    sub " << resultReg << ", " << leftReg << ", " << rightReg << "\n";
    } else if (binop->op == "*") {
        oss << "    mul " << resultReg << ", " << leftReg << ", " << rightReg << "\n";
    } else if (binop->op == "/") {
        oss << "    div " << resultReg << ", " << leftReg << ", " << rightReg << "\n";
    } else if (binop->op == "%") {
        oss << "    rem " << resultReg << ", " << leftReg << ", " << rightReg << "\n";
    } else {
        // TODO: Handle comparison and logical operators
        oss << "    # Unsupported operator: " << binop->op << "\n";
        oss << "    li " << resultReg << ", 0\n";
    }
    
    // Store result register for this expression
    exprToReg[binop] = resultReg;
    
    return oss.str();
}

std::string ASTToRISCVEmitter::emitReturn(const ReturnStatement* ret) {
    if (!ret) {
        return "";
    }
    
    std::ostringstream oss;
    
    if (ret->value) {
        // Emit return value expression
        oss << emitExpression(ret->value.get());
        
        // Get register for return value
        std::string retReg = getOrAllocateRegister(ret->value.get());
        
        // Load to a0 if it's on stack
        if (retReg.find("(sp)") != std::string::npos) {
            oss << "    ld a0, " << retReg << "\n";
        } else {
            oss << "    mv a0, " << retReg << "\n";
        }
    }
    
    // Function epilogue - use the tracked stack size
    int actualStackSize = currentFunctionStackSize;
    // Account for any additional stack allocations
    if (stackOffset > currentFunctionStackSize - 16) {
        actualStackSize = 16 + (stackOffset - 16);
    }
    
    oss << "    ld ra, " << (actualStackSize - 8) << "(sp)\n";
    oss << "    ld s0, " << (actualStackSize - 16) << "(sp)\n";
    oss << "    addi sp, sp, " << actualStackSize << "\n";
    oss << "    ret\n";
    
    return oss.str();
}

std::string ASTToRISCVEmitter::emitVariableDeclaration(const VariableDeclaration* varDecl) {
    if (!varDecl) {
        return "";
    }
    
    std::ostringstream oss;
    
    if (varDecl->initializer) {
        // Emit initializer expression
        oss << emitExpression(varDecl->initializer.get());
        
        // Get register for initializer value
        std::string valueReg = getOrAllocateRegister(varDecl->initializer.get());
        
        // Allocate stack slot for variable
        std::string varLocation = allocateStack(varDecl->name);
        
        // Store value to variable location
        if (valueReg.find("(sp)") != std::string::npos) {
            oss << "    ld t0, " << valueReg << "\n";
            oss << "    sd t0, " << varLocation << "\n";
        } else {
            oss << "    sd " << valueReg << ", " << varLocation << "\n";
        }
    } else {
        // No initializer - just allocate stack slot
        allocateStack(varDecl->name);
    }
    
    return oss.str();
}

std::string ASTToRISCVEmitter::allocateRegister() {
    if (tempRegIndex < numTempRegs) {
        std::string reg = tempRegs[tempRegIndex];
        tempRegIndex = (tempRegIndex + 1) % numTempRegs;
        return reg;
    }
    
    // Out of registers - use stack
    return allocateStack("_temp_" + std::to_string(stackOffset));
}

std::string ASTToRISCVEmitter::allocateStack(const std::string& varName) {
    if (varToStackOffset.find(varName) == varToStackOffset.end()) {
        varToStackOffset[varName] = stackOffset;
        stackOffset += 8; // 64-bit values
    }
    int offset = varToStackOffset[varName];
    return std::to_string(offset) + "(sp)";
}

std::string ASTToRISCVEmitter::getOrAllocateRegister(const ExpressionNode* expr) {
    auto it = exprToReg.find(expr);
    if (it != exprToReg.end()) {
        return it->second;
    }
    // Expression not found - allocate a new register
    // This shouldn't happen if emitExpression was called first
    return allocateRegister();
}

std::string ASTToRISCVEmitter::getVarLocation(const std::string& varName) {
    auto it = varToStackOffset.find(varName);
    if (it != varToStackOffset.end()) {
        int offset = it->second;
        return std::to_string(offset) + "(sp)";
    }
    return "0(sp)"; // Error case
}

void ASTToRISCVEmitter::resetFunctionState() {
    exprToReg.clear();
    // Keep varToStackOffset for function scope (contains function parameters)
    stackOffset = 16; // Start after saved registers (ra, s0)
    currentFunctionStackSize = 16; // Reset to minimum
    tempRegIndex = 0;
}

} // namespace gdscript

