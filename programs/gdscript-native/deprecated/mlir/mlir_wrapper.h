#pragma once

// Wrapper for StableHLO (MLIR dialect for ML compute)
// This file provides a clean interface to StableHLO operations

#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Module.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/Region.h"
#include "mlir/Support/LLVM.h"

// StableHLO includes
#include "stablehlo/dialect/StablehloOps.h"
#include "stablehlo/dialect/Base.h"

#include <memory>
#include <string>
#include <vector>

namespace mlir {

// Type aliases for convenience
using ModuleOp = mlir::ModuleOp;
using FuncOp = mlir::func::FuncOp;
using OpBuilder = mlir::OpBuilder;
using MLIRContext = mlir::MLIRContext;
using Value = mlir::Value;
using Type = mlir::Type;
using Block = mlir::Block;
using Operation = mlir::Operation;
using Location = mlir::Location;

// Helper functions for creating MLIR operations
namespace gdscript_mlir {

// Create an MLIR context
std::unique_ptr<MLIRContext> createContext();

// Create a module
ModuleOp createModule(MLIRContext* context);

// Create a function with the given signature
// Note: StableHLO works with tensor types
FuncOp createFunction(OpBuilder& builder, const std::string& name,
                      const std::vector<Type>& argTypes, Type returnType);

// Create StableHLO operations
// Note: StableHLO uses tensor types, so we work with tensors
Value createConstantI64(OpBuilder& builder, Location loc, int64_t value);
Value createAdd(OpBuilder& builder, Location loc, Value lhs, Value rhs);
Value createSubtract(OpBuilder& builder, Location loc, Value lhs, Value rhs);
Value createMultiply(OpBuilder& builder, Location loc, Value lhs, Value rhs);

// Create return operation
void createReturn(OpBuilder& builder, Location loc, Value value = Value());

// Create comparison operations (using StableHLO compare)
Value createCompare(OpBuilder& builder, Location loc, 
                   stablehlo::ComparisonDirection direction, Value lhs, Value rhs);

// Get function argument
Value getFunctionArg(FuncOp func, size_t index);

} // namespace gdscript_mlir

} // namespace mlir

