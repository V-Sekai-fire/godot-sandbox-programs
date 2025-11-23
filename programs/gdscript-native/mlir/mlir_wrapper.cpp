#include "mlir_wrapper.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Types.h"
#include "stablehlo/dialect/StablehloOps.h"
#include "stablehlo/dialect/Base.h"

namespace mlir {
namespace gdscript_mlir {

std::unique_ptr<MLIRContext> createContext() {
    auto context = std::make_unique<MLIRContext>();
    // Register dialects
    context->loadDialect<stablehlo::StablehloDialect>();
    context->loadDialect<func::FuncDialect>();
    return context;
}

ModuleOp createModule(MLIRContext* context) {
    OpBuilder builder(context);
    Location loc = builder.getUnknownLoc();
    return builder.create<ModuleOp>(loc);
}

FuncOp createFunction(OpBuilder& builder, const std::string& name,
                      const std::vector<Type>& argTypes, Type returnType) {
    Location loc = builder.getUnknownLoc();
    
    // Create function type
    FunctionType funcType = builder.getFunctionType(argTypes, {returnType});
    
    // Create function
    auto func = builder.create<func::FuncOp>(loc, name, funcType);
    
    // Create entry block
    Block* entryBlock = func.addEntryBlock();
    builder.setInsertionPointToStart(entryBlock);
    
    return func;
}

Value createConstantI64(OpBuilder& builder, Location loc, int64_t value) {
    // StableHLO uses tensor types, so create a tensor<i64>
    auto i64Type = builder.getI64Type();
    auto tensorType = RankedTensorType::get({}, i64Type);
    auto attr = DenseElementsAttr::get(tensorType, {value});
    return builder.create<stablehlo::ConstantOp>(loc, tensorType, attr).getResult();
}

Value createAdd(OpBuilder& builder, Location loc, Value lhs, Value rhs) {
    return builder.create<stablehlo::AddOp>(loc, lhs, rhs).getResult();
}

Value createSubtract(OpBuilder& builder, Location loc, Value lhs, Value rhs) {
    return builder.create<stablehlo::SubtractOp>(loc, lhs, rhs).getResult();
}

Value createMultiply(OpBuilder& builder, Location loc, Value lhs, Value rhs) {
    return builder.create<stablehlo::MultiplyOp>(loc, lhs, rhs).getResult();
}

void createReturn(OpBuilder& builder, Location loc, Value value) {
    if (value) {
        builder.create<func::ReturnOp>(loc, value);
    } else {
        builder.create<func::ReturnOp>(loc);
    }
}

Value createCompare(OpBuilder& builder, Location loc,
                   stablehlo::ComparisonDirection direction, Value lhs, Value rhs) {
    // StableHLO compare returns tensor<i1>
    auto i1Type = builder.getI1Type();
    auto tensorType = RankedTensorType::get({}, i1Type);
    auto compareTypeAttr = builder.getAttr<stablehlo::ComparisonTypeAttr>(
        stablehlo::ComparisonType::SIGNED);
    auto directionAttr = builder.getAttr<stablehlo::ComparisonDirectionAttr>(direction);
    return builder.create<stablehlo::CompareOp>(loc, tensorType, lhs, rhs, 
                                                directionAttr, compareTypeAttr).getResult();
}

Value getFunctionArg(FuncOp func, size_t index) {
    Block* entryBlock = &func.getBody().front();
    if (index < entryBlock->getNumArguments()) {
        return entryBlock->getArgument(index);
    }
    return Value();
}

} // namespace gdscript_mlir
} // namespace mlir

