#include "ir_to_riscv.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "stablehlo/dialect/StablehloOps.h"
#include <sstream>
#include <iomanip>

namespace mlir {

std::string RISCVEmitter::getBlockLabel(Block* block) {
    if (blockLabels.find(block) == blockLabels.end()) {
        std::string label = "L" + std::to_string(labelCounter++);
        blockLabels[block] = label;
    }
    return blockLabels[block];
}

std::string RISCVEmitter::allocateRegister(Value value) {
    if (valueToReg.find(value) != valueToReg.end()) {
        return valueToReg[value];
    }
    
    if (tempRegIndex < numTempRegs) {
        std::string reg = tempRegs[tempRegIndex];
        valueToReg[value] = reg;
        tempRegIndex = (tempRegIndex + 1) % numTempRegs;
        return reg;
    }
    
    return allocateStack(value);
}

std::string RISCVEmitter::allocateStack(Value value) {
    if (valueToStackOffset.find(value) == valueToStackOffset.end()) {
        valueToStackOffset[value] = stackOffset;
        stackOffset += 8; // 64-bit values
    }
    int offset = valueToStackOffset[value];
    return std::to_string(offset) + "(sp)";
}

std::string RISCVEmitter::getRegister(Value value) {
    if (valueToReg.find(value) != valueToReg.end()) {
        return valueToReg[value];
    }
    if (valueToStackOffset.find(value) != valueToStackOffset.end()) {
        return allocateStack(value);
    }
    return allocateRegister(value);
}

std::string RISCVEmitter::addStringLiteral(const std::string& str) {
    std::string label = "str_" + std::to_string(stringCounter++);
    stringLiterals.push_back(label + ": .asciz \"" + str + "\"");
    return label;
}

std::string RISCVEmitter::emitModule(ModuleOp module) {
    clear();
    
    asmCode << ".text\n";
    asmCode << ".globl _start\n";
    asmCode << "\n";
    
    // Walk all functions in the module
    module.walk([this](FuncOp func) {
        asmCode << emitFunction(func);
        asmCode << "\n";
    });
    
    // Emit string literals
    if (!stringLiterals.empty()) {
        asmCode << ".section .rodata\n";
        for (const auto& str : stringLiterals) {
            asmCode << str << "\n";
        }
    }
    
    return asmCode.str();
}

std::string RISCVEmitter::emitFunction(FuncOp func) {
    // Reset state for new function
    valueToReg.clear();
    valueToStackOffset.clear();
    blockLabels.clear();
    stackOffset = 0;
    tempRegIndex = 0;
    
    std::ostringstream funcAsm;
    
    std::string funcName = func.getName().str();
    funcAsm << ".globl " << funcName << "\n";
    funcAsm << funcName << ":\n";
    
    // Function prologue
    int stackSize = 16; // Minimum frame size
    funcAsm << "    addi sp, sp, -" << stackSize << "\n";
    funcAsm << "    sd ra, " << (stackSize - 8) << "(sp)\n";
    funcAsm << "    sd s0, " << (stackSize - 16) << "(sp)\n";
    funcAsm << "    addi s0, sp, " << stackSize << "\n";
    
    // Allocate function arguments to registers a0-a7
    Block* entryBlock = &func.getBody().front();
    for (size_t i = 0; i < entryBlock->getNumArguments() && i < 8; ++i) {
        Value arg = entryBlock->getArgument(i);
        std::string reg = "a" + std::to_string(i);
        valueToReg[arg] = reg;
        // Store argument to stack if needed
        funcAsm << "    sd " << reg << ", " << (i * 8) << "(sp)\n";
    }
    
    // Emit all blocks
    for (Block& block : func.getBody()) {
        funcAsm << emitBlock(&block);
    }
    
    // Function epilogue (fallback, though return ops should handle this)
    std::string epilogueLabel = funcName + "_epilogue";
    funcAsm << epilogueLabel << ":\n";
    funcAsm << "    ld ra, " << (stackSize - 8) << "(sp)\n";
    funcAsm << "    ld s0, " << (stackSize - 16) << "(sp)\n";
    funcAsm << "    addi sp, sp, " << stackSize << "\n";
    funcAsm << "    ret\n";
    
    return funcAsm.str();
}

std::string RISCVEmitter::emitBlock(Block* block) {
    std::ostringstream blockAsm;
    std::string label = getBlockLabel(block);
    blockAsm << label << ":\n";
    
    for (Operation& op : *block) {
        blockAsm << emitOperation(&op);
    }
    
    return blockAsm.str();
}

std::string RISCVEmitter::emitOperation(Operation* op) {
    std::ostringstream opAsm;
    
    // Check operation type and emit accordingly
    if (isa<stablehlo::ConstantOp>(op)) {
        opAsm << emitConstant(op);
    } else if (isa<stablehlo::AddOp>(op)) {
        opAsm << emitAddI(op);
    } else if (isa<stablehlo::SubtractOp>(op)) {
        opAsm << emitSubI(op);
    } else if (isa<stablehlo::MultiplyOp>(op)) {
        opAsm << emitMulI(op);
    } else if (isa<func::ReturnOp>(op)) {
        opAsm << emitReturn(op);
    } else {
        opAsm << "    # Unknown operation: " << op->getName().getStringRef().str() << "\n";
    }
    
    return opAsm.str();
}

std::string RISCVEmitter::emitConstant(Operation* op) {
    auto constantOp = cast<stablehlo::ConstantOp>(op);
    Value result = op->getResult(0);
    
    // Get the tensor attribute and extract scalar value
    auto attr = constantOp.getValue().cast<DenseElementsAttr>();
    int64_t value = 0;
    
    // Extract scalar from tensor (for rank-0 or single-element tensors)
    if (attr.isSplat()) {
        // Splat tensor - all elements are the same
        auto elementType = attr.getType().getElementType();
        if (elementType.isInteger(64)) {
            value = attr.getSplatValue<IntegerAttr>().getInt();
        }
    } else {
        // Dense tensor - get first element
        auto elementType = attr.getType().getElementType();
        if (elementType.isInteger(64)) {
            value = attr.getValues<IntegerAttr>()[0].getInt();
        }
    }
    
    std::string reg = allocateRegister(result);
    std::ostringstream oss;
    
    if (reg.find("(sp)") != std::string::npos) {
        oss << "    li t0, " << value << "\n";
        oss << "    sd t0, " << reg << "\n";
    } else {
        oss << "    li " << reg << ", " << value << "\n";
    }
    
    return oss.str();
}

std::string RISCVEmitter::emitAddI(Operation* op) {
    auto addOp = cast<stablehlo::AddOp>(op);
    Value lhs = addOp.getLhs();
    Value rhs = addOp.getRhs();
    Value result = op->getResult(0);
    
    std::string lhsReg = getRegister(lhs);
    std::string rhsReg = getRegister(rhs);
    std::string resultReg = allocateRegister(result);
    
    std::ostringstream oss;
    oss << "    # addi\n";
    
    // Load operands if they're on stack
    if (lhsReg.find("(sp)") != std::string::npos) {
        oss << "    ld t0, " << lhsReg << "\n";
        lhsReg = "t0";
    }
    if (rhsReg.find("(sp)") != std::string::npos) {
        oss << "    ld t1, " << rhsReg << "\n";
        rhsReg = "t1";
    }
    
    // Emit add instruction
    if (resultReg.find("(sp)") != std::string::npos) {
        oss << "    add t2, " << lhsReg << ", " << rhsReg << "\n";
        oss << "    sd t2, " << resultReg << "\n";
    } else {
        oss << "    add " << resultReg << ", " << lhsReg << ", " << rhsReg << "\n";
    }
    
    return oss.str();
}

std::string RISCVEmitter::emitSubI(Operation* op) {
    auto subOp = cast<stablehlo::SubtractOp>(op);
    Value lhs = subOp.getLhs();
    Value rhs = subOp.getRhs();
    Value result = op->getResult(0);
    
    std::string lhsReg = getRegister(lhs);
    std::string rhsReg = getRegister(rhs);
    std::string resultReg = allocateRegister(result);
    
    std::ostringstream oss;
    oss << "    # subi\n";
    
    if (lhsReg.find("(sp)") != std::string::npos) {
        oss << "    ld t0, " << lhsReg << "\n";
        lhsReg = "t0";
    }
    if (rhsReg.find("(sp)") != std::string::npos) {
        oss << "    ld t1, " << rhsReg << "\n";
        rhsReg = "t1";
    }
    
    if (resultReg.find("(sp)") != std::string::npos) {
        oss << "    sub t2, " << lhsReg << ", " << rhsReg << "\n";
        oss << "    sd t2, " << resultReg << "\n";
    } else {
        oss << "    sub " << resultReg << ", " << lhsReg << ", " << rhsReg << "\n";
    }
    
    return oss.str();
}

std::string RISCVEmitter::emitMulI(Operation* op) {
    auto mulOp = cast<stablehlo::MultiplyOp>(op);
    Value lhs = mulOp.getLhs();
    Value rhs = mulOp.getRhs();
    Value result = op->getResult(0);
    
    std::string lhsReg = getRegister(lhs);
    std::string rhsReg = getRegister(rhs);
    std::string resultReg = allocateRegister(result);
    
    std::ostringstream oss;
    oss << "    # muli\n";
    
    if (lhsReg.find("(sp)") != std::string::npos) {
        oss << "    ld t0, " << lhsReg << "\n";
        lhsReg = "t0";
    }
    if (rhsReg.find("(sp)") != std::string::npos) {
        oss << "    ld t1, " << rhsReg << "\n";
        rhsReg = "t1";
    }
    
    if (resultReg.find("(sp)") != std::string::npos) {
        oss << "    mul t2, " << lhsReg << ", " << rhsReg << "\n";
        oss << "    sd t2, " << resultReg << "\n";
    } else {
        oss << "    mul " << resultReg << ", " << lhsReg << ", " << rhsReg << "\n";
    }
    
    return oss.str();
}

// Note: StableHLO compare is handled differently - we'll implement it if needed
// For now, this method is kept for compatibility but may not be called
std::string RISCVEmitter::emitCmpI(Operation* op) {
    std::ostringstream oss;
    oss << "    # StableHLO compare operation (not yet implemented)\n";
    return oss.str();
}

std::string RISCVEmitter::emitReturn(Operation* op) {
    auto returnOp = cast<func::ReturnOp>(op);
    std::ostringstream oss;
    
    if (returnOp.getNumOperands() > 0) {
        Value retVal = returnOp.getOperand(0);
        std::string retReg = getRegister(retVal);
        
        if (retReg.find("(sp)") != std::string::npos) {
            oss << "    ld a0, " << retReg << "\n";
        } else {
            oss << "    mv a0, " << retReg << "\n";
        }
    }
    
    // Function epilogue
    oss << "    ld ra, 8(sp)\n";
    oss << "    ld s0, 0(sp)\n";
    oss << "    addi sp, sp, 16\n";
    oss << "    ret\n";
    
    return oss.str();
}

} // namespace mlir
