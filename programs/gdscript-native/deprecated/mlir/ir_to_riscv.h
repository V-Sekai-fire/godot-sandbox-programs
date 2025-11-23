#pragma once

#include "mlir_wrapper.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>

namespace mlir {

// RISC-V assembly emitter for real MLIR operations
class RISCVEmitter {
private:
    std::ostringstream asmCode;
    std::unordered_map<Value, std::string> valueToReg;
    std::unordered_map<Value, int> valueToStackOffset;
    std::unordered_map<Block*, std::string> blockLabels;
    std::vector<std::string> stringLiterals;
    int stackOffset;
    size_t labelCounter;
    size_t stringCounter;
    size_t tempRegIndex;
    
    // RISC-V registers: a0-a7 (arguments/returns), t0-t6 (temporaries), s0-s11 (saved)
    static constexpr const char* tempRegs[] = {"t0", "t1", "t2", "t3", "t4", "t5", "t6"};
    static constexpr size_t numTempRegs = 7;
    
    std::string getBlockLabel(Block* block);
    std::string allocateRegister(Value value);
    std::string allocateStack(Value value);
    std::string getRegister(Value value);
    std::string addStringLiteral(const std::string& str);
    
public:
    RISCVEmitter() : stackOffset(0), labelCounter(0), stringCounter(0), tempRegIndex(0) {}
    
    // Emit RISC-V assembly from MLIR module
    std::string emitModule(ModuleOp module);
    
    // Emit a function
    std::string emitFunction(FuncOp func);
    
    // Emit a block
    std::string emitBlock(Block* block);
    
    // Emit an operation
    std::string emitOperation(Operation* op);
    
    // Operation-specific emitters (StableHLO operations)
    std::string emitConstant(Operation* op);
    std::string emitAddI(Operation* op);
    std::string emitSubI(Operation* op);
    std::string emitMulI(Operation* op);
    std::string emitReturn(Operation* op);
    std::string emitCmpI(Operation* op); // Placeholder for future implementation
    
    std::string getAssembly() const { return asmCode.str(); }
    void clear() {
        asmCode.str("");
        asmCode.clear();
        valueToReg.clear();
        valueToStackOffset.clear();
        blockLabels.clear();
        stringLiterals.clear();
        stackOffset = 0;
        labelCounter = 0;
        stringCounter = 0;
        tempRegIndex = 0;
    }
};

} // namespace mlir
