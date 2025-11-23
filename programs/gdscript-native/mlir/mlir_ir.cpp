#include "mlir_ir.h"
#include <sstream>

namespace mlir {

std::string MLIRType::toString() const {
    switch (kind) {
        case TypeKind::I64: return "i64";
        case TypeKind::I32: return "i32";
        case TypeKind::F64: return "f64";
        case TypeKind::F32: return "f32";
        case TypeKind::VOID: return "void";
        case TypeKind::POINTER: return "ptr";
        default: return "unknown";
    }
}

std::string MLIRValue::toString() const {
    if (!name.empty()) {
        return name;
    }
    if (definingOp) {
        std::ostringstream oss;
        oss << "%" << definingOp->opName << "_" << resultIndex;
        return oss.str();
    }
    return "%unknown";
}

std::string MLIROperation::toString() const {
    std::ostringstream oss;
    oss << opName;
    if (!operands.empty()) {
        oss << "(";
        for (size_t i = 0; i < operands.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << operands[i]->toString();
        }
        oss << ")";
    }
    return oss.str();
}

MLIROperation* MLIRBlock::addOperation(std::unique_ptr<MLIROperation> op) {
    MLIROperation* ptr = op.get();
    operations.push_back(std::move(op));
    return ptr;
}

std::string MLIRBlock::toString() const {
    std::ostringstream oss;
    oss << "^" << name << ":\n";
    for (const auto& op : operations) {
        oss << "  " << op->toString() << "\n";
    }
    return oss.str();
}

std::string MLIRFunction::toString() const {
    std::ostringstream oss;
    oss << "func @" << name << "(";
    for (size_t i = 0; i < argTypes.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << "%arg" << i << ": " << argTypes[i].toString();
    }
    oss << ") -> " << returnType.toString() << " {\n";
    for (const auto& block : blocks) {
        oss << block->toString();
    }
    oss << "}\n";
    return oss.str();
}

MLIRFunction* MLIRModule::addFunction(const std::string& name,
                                      const std::vector<MLIRType>& argTypes,
                                      MLIRType returnType) {
    auto func = std::make_unique<MLIRFunction>(name, argTypes, returnType, this);
    MLIRFunction* ptr = func.get();
    functions.push_back(std::move(func));
    return ptr;
}

std::string MLIRModule::toString() const {
    std::ostringstream oss;
    oss << "module @" << name << " {\n";
    for (const auto& func : functions) {
        oss << func->toString() << "\n";
    }
    oss << "}\n";
    return oss.str();
}

} // namespace mlir

