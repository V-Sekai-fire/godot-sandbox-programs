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
    std::vector<uint8_t> codeBuffer;
    std::unique_ptr<biscuit::Assembler> assembler;
    
    // Register allocation
    std::unordered_map<const ExpressionNode*, biscuit::GPR> exprToReg;
    std::unordered_map<std::string, int> varToStackOffset; // Variable name -> stack offset
    
    // Stack management
    int stackOffset;
    int currentFunctionStackSize; // Track stack size for current function
    size_t labelCounter;
    size_t tempRegIndex;
    
    // RISC-V registers: a0-a7 (arguments/returns), t0-t6 (temporaries), s0-s11 (saved)
    static constexpr biscuit::GPR tempRegs[] = {
        biscuit::GPR::T0, biscuit::GPR::T1, biscuit::GPR::T2,
        biscuit::GPR::T3, biscuit::GPR::T4, biscuit::GPR::T5, biscuit::GPR::T6
    };
    static constexpr size_t numTempRegs = 7;
    
    // Helper methods
    biscuit::GPR allocateRegister();
    int allocateStack(const std::string& varName);
    biscuit::GPR getOrAllocateRegister(const ExpressionNode* expr);
    int getVarStackOffset(const std::string& varName);
    
    // Emit methods for different AST node types
    void emitProgram(const ProgramNode* program);
    void emitFunction(const FunctionNode* func);
    void emitStatement(const StatementNode* stmt);
    void emitExpression(const ExpressionNode* expr);
    
    // Expression emitters
    void emitLiteral(const LiteralExpr* lit);
    void emitIdentifier(const IdentifierExpr* ident);
    void emitBinaryOp(const BinaryOpExpr* binop);
    void emitReturn(const ReturnStatement* ret);
    void emitVariableDeclaration(const VariableDeclaration* varDecl);
    
    // Reset state for new function
    void resetFunctionState();
    
public:
    ASTToRISCVEmitter() : stackOffset(0), currentFunctionStackSize(16), 
                          labelCounter(0), tempRegIndex(0) {
        codeBuffer.resize(8192); // Initial buffer size (will grow if needed)
    }
    
    // Main entry point: emit RISC-V machine code from AST
    // Returns pointer to machine code and size
    std::pair<const uint8_t*, size_t> emit(const ProgramNode* program);
    
    // Get the generated machine code
    const std::vector<uint8_t>& getCode() const { return codeBuffer; }
    
    // Clear state
    void clear() {
        codeBuffer.clear();
        codeBuffer.resize(8192);
        exprToReg.clear();
        varToStackOffset.clear();
        stackOffset = 0;
        currentFunctionStackSize = 16;
        labelCounter = 0;
        tempRegIndex = 0;
    }
};

} // namespace gdscript

