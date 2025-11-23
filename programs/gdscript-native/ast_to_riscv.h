#pragma once

#include "parser/ast.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

namespace gdscript {

// Direct AST to RISC-V assembly emitter (following BEAM JIT pattern)
// No MLIR/StableHLO dependency - direct code generation
class ASTToRISCVEmitter {
private:
    std::ostringstream asmCode;
    
    // Register allocation
    std::unordered_map<const ExpressionNode*, std::string> exprToReg;
    std::unordered_map<std::string, int> varToStackOffset; // Variable name -> stack offset
    std::unordered_map<const FunctionNode*, std::string> funcLabels;
    
    // Stack management
    int stackOffset;
    int currentFunctionStackSize; // Track stack size for current function
    size_t labelCounter;
    size_t tempRegIndex;
    
    // RISC-V registers: a0-a7 (arguments/returns), t0-t6 (temporaries), s0-s11 (saved)
    static constexpr const char* tempRegs[] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6"};
    static constexpr size_t numTempRegs = 7;
    
    // Helper methods
    std::string allocateRegister();
    std::string allocateStack(const std::string& varName);
    std::string getOrAllocateRegister(const ExpressionNode* expr);
    std::string getVarLocation(const std::string& varName);
    
    // Emit methods for different AST node types
    std::string emitProgram(const ProgramNode* program);
    std::string emitFunction(const FunctionNode* func);
    std::string emitStatement(const StatementNode* stmt);
    std::string emitExpression(const ExpressionNode* expr);
    
    // Expression emitters
    std::string emitLiteral(const LiteralExpr* lit);
    std::string emitIdentifier(const IdentifierExpr* ident);
    std::string emitBinaryOp(const BinaryOpExpr* binop);
    std::string emitReturn(const ReturnStatement* ret);
    std::string emitVariableDeclaration(const VariableDeclaration* varDecl);
    
    // Reset state for new function
    void resetFunctionState();
    
public:
    ASTToRISCVEmitter() : stackOffset(0), currentFunctionStackSize(16), labelCounter(0), tempRegIndex(0) {}
    
    // Main entry point: emit RISC-V assembly from AST
    std::string emit(const ProgramNode* program);
    
    // Get the generated assembly
    std::string getAssembly() const { return asmCode.str(); }
    
    // Clear state
    void clear() {
        asmCode.str("");
        asmCode.clear();
        exprToReg.clear();
        varToStackOffset.clear();
        funcLabels.clear();
        stackOffset = 0;
        currentFunctionStackSize = 16;
        labelCounter = 0;
        tempRegIndex = 0;
    }
};

} // namespace gdscript

