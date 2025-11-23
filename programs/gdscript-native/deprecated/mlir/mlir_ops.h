#pragma once

#include "mlir_ir.h"
#include <cstdint>
#include <sstream>

namespace mlir {

// Arithmetic operations
class ArithAddiOp : public MLIROperation {
public:
    ArithAddiOp(MLIRBlock* parent, MLIRValue* lhs, MLIRValue* rhs)
        : MLIROperation("arith.addi", parent) {
        operands.push_back(lhs);
        operands.push_back(rhs);
        // Result type is same as input type
        auto result = new MLIRValue(lhs->type, this, 0);
        results.push_back(result);
    }
};

class ArithSubiOp : public MLIROperation {
public:
    ArithSubiOp(MLIRBlock* parent, MLIRValue* lhs, MLIRValue* rhs)
        : MLIROperation("arith.subi", parent) {
        operands.push_back(lhs);
        operands.push_back(rhs);
        auto result = new MLIRValue(lhs->type, this, 0);
        results.push_back(result);
    }
};

class ArithMuliOp : public MLIROperation {
public:
    ArithMuliOp(MLIRBlock* parent, MLIRValue* lhs, MLIRValue* rhs)
        : MLIROperation("arith.muli", parent) {
        operands.push_back(lhs);
        operands.push_back(rhs);
        auto result = new MLIRValue(lhs->type, this, 0);
        results.push_back(result);
    }
};

class ArithConstantOp : public MLIROperation {
public:
    int64_t value;
    
    ArithConstantOp(MLIRBlock* parent, MLIRType type, int64_t val)
        : MLIROperation("arith.constant", parent), value(val) {
        auto result = new MLIRValue(type, this, 0);
        results.push_back(result);
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << opName << " " << value << " : " << results[0]->type.toString();
        return oss.str();
    }
};

// Function operations
class FuncReturnOp : public MLIROperation {
public:
    FuncReturnOp(MLIRBlock* parent, MLIRValue* value = nullptr)
        : MLIROperation("func.return", parent) {
        if (value) {
            operands.push_back(value);
        }
    }
};

class FuncCallOp : public MLIROperation {
public:
    std::string calleeName;
    
    FuncCallOp(MLIRBlock* parent, const std::string& callee, 
               const std::vector<MLIRValue*>& args, MLIRType returnType)
        : MLIROperation("func.call", parent), calleeName(callee) {
        operands = args;
        if (returnType.kind != TypeKind::VOID) {
            auto result = new MLIRValue(returnType, this, 0);
            results.push_back(result);
        }
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << opName << " @" << calleeName << "(";
        for (size_t i = 0; i < operands.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << operands[i]->toString();
        }
        oss << ")";
        if (!results.empty()) {
            oss << " : " << results[0]->type.toString();
        }
        return oss.str();
    }
};

// Control flow operations
class CondBrOp : public MLIROperation {
public:
    MLIRBlock* trueBlock;
    MLIRBlock* falseBlock;
    
    CondBrOp(MLIRBlock* parent, MLIRValue* condition,
             MLIRBlock* trueBlk, MLIRBlock* falseBlk)
        : MLIROperation("cf.cond_br", parent),
          trueBlock(trueBlk), falseBlock(falseBlk) {
        operands.push_back(condition);
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << opName << " " << operands[0]->toString() 
            << ", ^" << trueBlock->name << ", ^" << falseBlock->name;
        return oss.str();
    }
};

class BrOp : public MLIROperation {
public:
    MLIRBlock* targetBlock;
    
    BrOp(MLIRBlock* parent, MLIRBlock* target)
        : MLIROperation("cf.br", parent), targetBlock(target) {}
    
    std::string toString() const override {
        std::ostringstream oss;
        oss << opName << " ^" << targetBlock->name;
        return oss.str();
    }
};

// Comparison operations
class CmpIOp : public MLIROperation {
public:
    enum class Predicate {
        EQ,  // Equal
        NE,  // Not equal
        SLT, // Signed less than
        SLE, // Signed less than or equal
        SGT, // Signed greater than
        SGE  // Signed greater than or equal
    };
    
    Predicate predicate;
    
    CmpIOp(MLIRBlock* parent, Predicate pred, MLIRValue* lhs, MLIRValue* rhs)
        : MLIROperation("arith.cmpi", parent), predicate(pred) {
        operands.push_back(lhs);
        operands.push_back(rhs);
        auto result = new MLIRValue(MLIRType::i64(), this, 0); // Comparison returns i64 (0 or 1)
        results.push_back(result);
    }
    
    std::string toString() const override {
        std::ostringstream oss;
        const char* predStr = "";
        switch (predicate) {
            case Predicate::EQ: predStr = "eq"; break;
            case Predicate::NE: predStr = "ne"; break;
            case Predicate::SLT: predStr = "slt"; break;
            case Predicate::SLE: predStr = "sle"; break;
            case Predicate::SGT: predStr = "sgt"; break;
            case Predicate::SGE: predStr = "sge"; break;
        }
        oss << opName << " " << predStr << ", " 
            << operands[0]->toString() << ", " << operands[1]->toString();
        return oss.str();
    }
};

} // namespace mlir

