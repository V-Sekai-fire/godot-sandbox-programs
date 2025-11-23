#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Custom MLIR-like IR implementation (no official MLIR library)

namespace mlir {

// Forward declarations
class MLIRType;
class MLIRValue;
class MLIROperation;
class MLIRBlock;
class MLIRFunction;
class MLIRModule;

// Type system
enum class TypeKind {
    I64,    // 64-bit integer
    I32,    // 32-bit integer
    F64,    // 64-bit float
    F32,    // 32-bit float
    VOID,   // Void type
    POINTER // Pointer type
};

class MLIRType {
public:
    TypeKind kind;
    
    MLIRType(TypeKind k) : kind(k) {}
    
    static MLIRType i64() { return MLIRType(TypeKind::I64); }
    static MLIRType i32() { return MLIRType(TypeKind::I32); }
    static MLIRType f64() { return MLIRType(TypeKind::F64); }
    static MLIRType f32() { return MLIRType(TypeKind::F32); }
    static MLIRType voidType() { return MLIRType(TypeKind::VOID); }
    static MLIRType pointer(MLIRType base) { return MLIRType(TypeKind::POINTER); }
    
    std::string toString() const;
    bool operator==(const MLIRType& other) const { return kind == other.kind; }
};

// SSA Value (result of an operation)
class MLIRValue {
public:
    MLIRType type;
    MLIROperation* definingOp;
    size_t resultIndex;
    std::string name; // Optional name for debugging
    
    MLIRValue(MLIRType t, MLIROperation* op, size_t idx)
        : type(t), definingOp(op), resultIndex(idx), name("") {}
    
    std::string toString() const;
};

// Operation base class
class MLIROperation {
public:
    std::string opName; // e.g., "arith.addi", "func.return"
    std::vector<MLIRValue*> operands;
    std::vector<MLIRValue*> results;
    MLIRBlock* parentBlock;
    
    MLIROperation(const std::string& name, MLIRBlock* parent)
        : opName(name), parentBlock(parent) {}
    
    virtual ~MLIROperation() = default;
    virtual std::string toString() const;
};

// Basic block (for control flow)
class MLIRBlock {
public:
    std::string name;
    std::vector<std::unique_ptr<MLIROperation>> operations;
    MLIRFunction* parentFunction;
    
    MLIRBlock(const std::string& n, MLIRFunction* parent)
        : name(n), parentFunction(parent) {}
    
    MLIROperation* addOperation(std::unique_ptr<MLIROperation> op);
    std::string toString() const;
};

// Function
class MLIRFunction {
public:
    std::string name;
    std::vector<MLIRType> argTypes;
    MLIRType returnType;
    std::unique_ptr<MLIRBlock> entryBlock;
    std::vector<std::unique_ptr<MLIRBlock>> blocks;
    MLIRModule* parentModule;
    
    MLIRFunction(const std::string& n, 
                 const std::vector<MLIRType>& args,
                 MLIRType ret,
                 MLIRModule* parent)
        : name(n), argTypes(args), returnType(ret), parentModule(parent) {
        entryBlock = std::make_unique<MLIRBlock>("entry", this);
    }
    
    MLIRBlock* getEntryBlock() { return entryBlock.get(); }
    std::string toString() const;
};

// Module (top-level container)
class MLIRModule {
public:
    std::vector<std::unique_ptr<MLIRFunction>> functions;
    std::string name;
    
    MLIRModule(const std::string& n = "module") : name(n) {}
    
    MLIRFunction* addFunction(const std::string& name,
                             const std::vector<MLIRType>& argTypes,
                             MLIRType returnType);
    
    std::string toString() const;
};

} // namespace mlir

