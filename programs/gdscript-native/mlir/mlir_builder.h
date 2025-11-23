#pragma once

#include "mlir_ir.h"
#include "mlir_ops.h"
#include <memory>

namespace mlir {

// Builder for constructing MLIR IR
class MLIRBuilder {
private:
    MLIRModule* module;
    MLIRFunction* currentFunction;
    MLIRBlock* currentBlock;
    size_t valueCounter;

public:
    MLIRBuilder(MLIRModule* mod) 
        : module(mod), currentFunction(nullptr), currentBlock(nullptr), valueCounter(0) {}
    
    // Module and function creation
    MLIRFunction* createFunction(const std::string& name,
                                 const std::vector<MLIRType>& argTypes,
                                 MLIRType returnType) {
        currentFunction = module->addFunction(name, argTypes, returnType);
        currentBlock = currentFunction->getEntryBlock();
        return currentFunction;
    }
    
    void setInsertionPoint(MLIRBlock* block) {
        currentBlock = block;
    }
    
    MLIRBlock* createBlock(const std::string& name) {
        if (!currentFunction) return nullptr;
        auto block = std::make_unique<MLIRBlock>(name, currentFunction);
        MLIRBlock* ptr = block.get();
        currentFunction->blocks.push_back(std::move(block));
        return ptr;
    }
    
    // Arithmetic operations
    MLIRValue* createAddi(MLIRValue* lhs, MLIRValue* rhs) {
        if (!currentBlock) return nullptr;
        auto op = std::make_unique<ArithAddiOp>(currentBlock, lhs, rhs);
        MLIRValue* result = op->results[0];
        currentBlock->addOperation(std::move(op));
        return result;
    }
    
    MLIRValue* createSubi(MLIRValue* lhs, MLIRValue* rhs) {
        if (!currentBlock) return nullptr;
        auto op = std::make_unique<ArithSubiOp>(currentBlock, lhs, rhs);
        MLIRValue* result = op->results[0];
        currentBlock->addOperation(std::move(op));
        return result;
    }
    
    MLIRValue* createMuli(MLIRValue* lhs, MLIRValue* rhs) {
        if (!currentBlock) return nullptr;
        auto op = std::make_unique<ArithMuliOp>(currentBlock, lhs, rhs);
        MLIRValue* result = op->results[0];
        currentBlock->addOperation(std::move(op));
        return result;
    }
    
    MLIRValue* createConstant(MLIRType type, int64_t value) {
        if (!currentBlock) return nullptr;
        auto op = std::make_unique<ArithConstantOp>(currentBlock, type, value);
        MLIRValue* result = op->results[0];
        currentBlock->addOperation(std::move(op));
        return result;
    }
    
    // Function operations
    void createReturn(MLIRValue* value = nullptr) {
        if (!currentBlock) return;
        auto op = std::make_unique<FuncReturnOp>(currentBlock, value);
        currentBlock->addOperation(std::move(op));
    }
    
    MLIRValue* createCall(const std::string& callee,
                         const std::vector<MLIRValue*>& args,
                         MLIRType returnType) {
        if (!currentBlock) return nullptr;
        auto op = std::make_unique<FuncCallOp>(currentBlock, callee, args, returnType);
        MLIRValue* result = op->results.empty() ? nullptr : op->results[0];
        currentBlock->addOperation(std::move(op));
        return result;
    }
    
    // Control flow operations
    void createCondBr(MLIRValue* condition, MLIRBlock* trueBlock, MLIRBlock* falseBlock) {
        if (!currentBlock) return;
        auto op = std::make_unique<CondBrOp>(currentBlock, condition, trueBlock, falseBlock);
        currentBlock->addOperation(std::move(op));
    }
    
    void createBr(MLIRBlock* targetBlock) {
        if (!currentBlock) return;
        auto op = std::make_unique<BrOp>(currentBlock, targetBlock);
        currentBlock->addOperation(std::move(op));
    }
    
    // Comparison operations
    MLIRValue* createCmpi(CmpIOp::Predicate pred, MLIRValue* lhs, MLIRValue* rhs) {
        if (!currentBlock) return nullptr;
        auto op = std::make_unique<CmpIOp>(currentBlock, pred, lhs, rhs);
        MLIRValue* result = op->results[0];
        currentBlock->addOperation(std::move(op));
        return result;
    }
    
    // Get function arguments (placeholder - needs proper implementation)
    MLIRValue* getFunctionArg(size_t index) {
        // TODO: Proper argument value tracking
        // For now, arguments are accessed via stack offsets in the emitter
        return nullptr;
    }
    
    MLIRBlock* getCurrentBlock() const { return currentBlock; }
    MLIRFunction* getCurrentFunction() const { return currentFunction; }
};

} // namespace mlir

