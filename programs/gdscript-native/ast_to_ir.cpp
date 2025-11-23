#include "ast_to_ir.h"
#include "mlir/mlir_wrapper.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Types.h"
#include "parser/ast.h"
#include <variant>

namespace gdscript {

ASTToIRConverter::ASTToIRConverter() 
    : context(mlir::gdscript_mlir::createContext()),
      module(mlir::gdscript_mlir::createModule(context.get())),
      builder(context.get()) {
    builder.setInsertionPointToStart(module.getBody());
}

mlir::FuncOp ASTToIRConverter::convertSimpleFunction(
    const std::string& name,
    const std::vector<mlir::Type>& argTypes,
    mlir::Type returnType) {
    
    return mlir::gdscript_mlir::createFunction(builder, name, argTypes, returnType);
}

mlir::FuncOp ASTToIRConverter::createHelloWorld() {
    // Create a simple function that returns 42
    // StableHLO uses tensor types, so we use tensor<i64>
    auto i64Type = builder.getI64Type();
    auto tensorType = RankedTensorType::get({}, i64Type);
    mlir::Location loc = builder.getUnknownLoc();
    
    // Create function: func hello_world() -> tensor<i64>
    mlir::FuncOp func = mlir::gdscript_mlir::createFunction(
        builder, "hello_world", {}, tensorType);
    
    // Set insertion point to function body
    Block* entryBlock = &func.getBody().front();
    builder.setInsertionPointToStart(entryBlock);
    
    // Create constant 42
    mlir::Value constant = mlir::gdscript_mlir::createConstantI64(builder, loc, 42);
    
    // Return the constant
    mlir::gdscript_mlir::createReturn(builder, loc, constant);
    
    return func;
}

mlir::FuncOp ASTToIRConverter::createAdd() {
    // Create function: func add(a: tensor<i64>, b: tensor<i64>) -> tensor<i64>
    auto i64Type = builder.getI64Type();
    auto tensorType = RankedTensorType::get({}, i64Type);
    mlir::Location loc = builder.getUnknownLoc();
    
    std::vector<mlir::Type> argTypes = {tensorType, tensorType};
    mlir::FuncOp func = mlir::gdscript_mlir::createFunction(
        builder, "add", argTypes, tensorType);
    
    // Set insertion point to function body
    Block* entryBlock = &func.getBody().front();
    builder.setInsertionPointToStart(entryBlock);
    
    // Get function arguments
    mlir::Value a = mlir::gdscript_mlir::getFunctionArg(func, 0);
    mlir::Value b = mlir::gdscript_mlir::getFunctionArg(func, 1);
    
    // Add them using StableHLO
    mlir::Value sum = mlir::gdscript_mlir::createAdd(builder, loc, a, b);
    
    // Return the sum
    mlir::gdscript_mlir::createReturn(builder, loc, sum);
    
    return func;
}

mlir::FuncOp ASTToIRConverter::createFibonacci() {
    // Create function: func fibonacci(n: tensor<i64>) -> tensor<i64>
    // For now, simplified version - full implementation would need control flow
    auto i64Type = builder.getI64Type();
    auto tensorType = RankedTensorType::get({}, i64Type);
    mlir::Location loc = builder.getUnknownLoc();
    
    std::vector<mlir::Type> argTypes = {tensorType};
    mlir::FuncOp func = mlir::gdscript_mlir::createFunction(
        builder, "fibonacci", argTypes, tensorType);
    
    // Set insertion point to function body
    Block* entryBlock = &func.getBody().front();
    builder.setInsertionPointToStart(entryBlock);
    
    // Simplified: just return n (full implementation would need branches)
    mlir::Value n = mlir::gdscript_mlir::getFunctionArg(func, 0);
    mlir::gdscript_mlir::createReturn(builder, loc, n);
    
    return func;
}

// Convert AST ProgramNode to MLIR module
mlir::ModuleOp ASTToIRConverter::convertProgram(ProgramNode* program) {
    if (!program) {
        return module;
    }
    
    // Convert all functions
    for (auto& func : program->functions) {
        convertFunction(func.get());
    }
    
    return module;
}

// Convert AST FunctionNode to MLIR function
mlir::FuncOp ASTToIRConverter::convertFunction(FunctionNode* func) {
    if (!func) {
        return mlir::FuncOp();
    }
    
    // Clear variable mapping for new function
    variables.clear();
    
    auto i64Type = builder.getI64Type();
    auto tensorType = RankedTensorType::get({}, i64Type);
    mlir::Location loc = builder.getUnknownLoc();
    
    // Build argument types (for now, assume all are i64)
    std::vector<mlir::Type> argTypes;
    for (const auto& param : func->parameters) {
        argTypes.push_back(tensorType); // TODO: Use actual type from param.second
    }
    
    // Build return type
    mlir::Type returnType = tensorType; // TODO: Use func->returnType if present
    
    // Create MLIR function
    mlir::FuncOp mlirFunc = mlir::gdscript_mlir::createFunction(
        builder, func->name, argTypes, returnType);
    
    // Map function arguments to variables
    Block* entryBlock = &mlirFunc.getBody().front();
    for (size_t i = 0; i < func->parameters.size() && i < mlirFunc.getNumArguments(); ++i) {
        variables[func->parameters[i].first] = mlir::gdscript_mlir::getFunctionArg(mlirFunc, i);
    }
    
    // Set insertion point to function body
    builder.setInsertionPointToStart(entryBlock);
    
    // Convert function body statements
    for (auto& stmt : func->body) {
        convertStatement(stmt.get(), entryBlock);
    }
    
    return mlirFunc;
}

// Convert AST StatementNode to MLIR operations
void ASTToIRConverter::convertStatement(StatementNode* stmt, mlir::Block* block) {
    if (!stmt) return;
    
    builder.setInsertionPointToEnd(block);
    mlir::Location loc = builder.getUnknownLoc();
    
    switch (stmt->getType()) {
        case ASTNode::NodeType::ReturnStatement: {
            auto* retStmt = static_cast<ReturnStatement*>(stmt);
            mlir::Value retValue;
            if (retStmt->value) {
                retValue = convertExpression(retStmt->value.get(), block);
            }
            mlir::gdscript_mlir::createReturn(builder, loc, retValue);
            break;
        }
        case ASTNode::NodeType::VariableDeclaration: {
            auto* varDecl = static_cast<VariableDeclaration*>(stmt);
            mlir::Value initValue;
            if (varDecl->initializer) {
                initValue = convertExpression(varDecl->initializer.get(), block);
            } else {
                // Default to 0 if no initializer
                initValue = mlir::gdscript_mlir::createConstantI64(builder, loc, 0);
            }
            variables[varDecl->name] = initValue;
            break;
        }
        case ASTNode::NodeType::ExpressionStatement: {
            auto* exprStmt = static_cast<ExpressionStatement*>(stmt);
            if (exprStmt->expression) {
                convertExpression(exprStmt->expression.get(), block);
                // Expression result is computed but not stored (side effects only)
            }
            break;
        }
        default:
            // TODO: Handle other statement types
            break;
    }
}

// Convert AST ExpressionNode to MLIR Value
mlir::Value ASTToIRConverter::convertExpression(ExpressionNode* expr, mlir::Block* block) {
    if (!expr) {
        return mlir::Value();
    }
    
    builder.setInsertionPointToEnd(block);
    mlir::Location loc = builder.getUnknownLoc();
    
    switch (expr->getType()) {
        case ASTNode::NodeType::LiteralExpr:
            return convertLiteral(static_cast<LiteralExpr*>(expr));
        
        case ASTNode::NodeType::IdentifierExpr:
            return convertIdentifier(static_cast<IdentifierExpr*>(expr), block);
        
        case ASTNode::NodeType::BinaryOpExpr:
            return convertBinaryOp(static_cast<BinaryOpExpr*>(expr), block);
        
        default:
            // TODO: Handle other expression types
            return mlir::Value();
    }
}

// Convert literal expression to MLIR constant
mlir::Value ASTToIRConverter::convertLiteral(LiteralExpr* lit) {
    if (!lit) {
        return mlir::Value();
    }
    
    mlir::Location loc = builder.getUnknownLoc();
    
    // Extract value from variant
    if (std::holds_alternative<int64_t>(lit->value)) {
        int64_t val = std::get<int64_t>(lit->value);
        return mlir::gdscript_mlir::createConstantI64(builder, loc, val);
    } else if (std::holds_alternative<double>(lit->value)) {
        // TODO: Handle float literals
        double val = std::get<double>(lit->value);
        // For now, convert to int64
        return mlir::gdscript_mlir::createConstantI64(builder, loc, static_cast<int64_t>(val));
    } else if (std::holds_alternative<bool>(lit->value)) {
        bool val = std::get<bool>(lit->value);
        return mlir::gdscript_mlir::createConstantI64(builder, loc, val ? 1 : 0);
    } else if (std::holds_alternative<std::string>(lit->value)) {
        // TODO: Handle string literals
        return mlir::Value();
    }
    
    return mlir::Value();
}

// Convert identifier expression (variable reference)
mlir::Value ASTToIRConverter::convertIdentifier(IdentifierExpr* ident, mlir::Block* block) {
    if (!ident) {
        return mlir::Value();
    }
    
    // Look up variable in current scope
    auto it = variables.find(ident->name);
    if (it != variables.end()) {
        return it->second;
    }
    
    // Variable not found - return empty value (error case)
    return mlir::Value();
}

// Convert binary operation
mlir::Value ASTToIRConverter::convertBinaryOp(BinaryOpExpr* binop, mlir::Block* block) {
    if (!binop || !binop->left || !binop->right) {
        return mlir::Value();
    }
    
    mlir::Location loc = builder.getUnknownLoc();
    
    mlir::Value left = convertExpression(binop->left.get(), block);
    mlir::Value right = convertExpression(binop->right.get(), block);
    
    if (!left || !right) {
        return mlir::Value();
    }
    
    // Handle different operators
    if (binop->op == "+") {
        return mlir::gdscript_mlir::createAdd(builder, loc, left, right);
    } else if (binop->op == "-") {
        return mlir::gdscript_mlir::createSubtract(builder, loc, left, right);
    } else if (binop->op == "*") {
        return mlir::gdscript_mlir::createMultiply(builder, loc, left, right);
    } else if (binop->op == "/") {
        // TODO: Implement divide operation in mlir_wrapper
        return mlir::Value();
    } else {
        // TODO: Handle other operators
        return mlir::Value();
    }
}

} // namespace gdscript
