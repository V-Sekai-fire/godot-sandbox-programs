#pragma once

#include "mlir/mlir_wrapper.h"
#include "parser/ast.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace gdscript {

// AST to MLIR converter using real MLIR
class ASTToIRConverter {
private:
    std::unique_ptr<mlir::MLIRContext> context;
    mlir::ModuleOp module;
    mlir::OpBuilder builder;
    
    // Variable mapping: variable name -> MLIR Value
    std::unordered_map<std::string, mlir::Value> variables;

public:
    ASTToIRConverter();
    
    mlir::ModuleOp getModule() { return module; }
    mlir::MLIRContext* getContext() { return context.get(); }
    
    // Convert AST ProgramNode to MLIR module
    mlir::ModuleOp convertProgram(ProgramNode* program);
    
    // Legacy method name (for compatibility)
    mlir::ModuleOp convert(std::unique_ptr<ProgramNode> ast) {
        return convertProgram(ast.get());
    }
    
    // Convert AST FunctionNode to MLIR function
    mlir::FuncOp convertFunction(FunctionNode* func);
    
    // Convert AST StatementNode to MLIR operations
    void convertStatement(StatementNode* stmt, mlir::Block* block);
    
    // Convert AST ExpressionNode to MLIR Value
    mlir::Value convertExpression(ExpressionNode* expr, mlir::Block* block);
    
    // Convert literal expression to MLIR constant
    mlir::Value convertLiteral(LiteralExpr* lit);
    
    // Convert identifier expression (variable reference)
    mlir::Value convertIdentifier(IdentifierExpr* ident, mlir::Block* block);
    
    // Convert binary operation
    mlir::Value convertBinaryOp(BinaryOpExpr* binop, mlir::Block* block);
    
    // Legacy helper functions (kept for compatibility)
    mlir::FuncOp createHelloWorld();
    mlir::FuncOp createAdd();
    mlir::FuncOp createFibonacci();
};

} // namespace gdscript
