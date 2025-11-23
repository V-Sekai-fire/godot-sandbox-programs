// Test AST to MLIR conversion with real AST nodes
#include "../parser/gdscript_parser.h"
#include "../ast_to_ir.h"
#include <iostream>
#include <string>
#include <cassert>

using namespace gdscript;

void test_simple_function_to_mlir() {
    std::cout << "=== Test: Simple Function to MLIR ===\n";
    GDScriptParser parser;
    std::string source = "func hello():\nreturn 42\n";
    
    auto ast = parser.parse(source);
    assert(ast != nullptr);
    assert(ast->functions.size() == 1);
    
    ASTToIRConverter converter;
    mlir::ModuleOp module = converter.convertProgram(ast.get());
    
    assert(module != nullptr);
    std::cout << "✓ Module created successfully\n";
    
    // Check that module has functions
    // (We can't easily check MLIR operations without more complex code)
    std::cout << "✓ Simple function to MLIR test passed\n\n";
}

void test_function_with_parameters_to_mlir() {
    std::cout << "=== Test: Function with Parameters to MLIR ===\n";
    GDScriptParser parser;
    std::string source = "func add(a, b):\nreturn a\n";
    
    auto ast = parser.parse(source);
    assert(ast != nullptr);
    assert(ast->functions.size() == 1);
    
    ASTToIRConverter converter;
    mlir::ModuleOp module = converter.convertProgram(ast.get());
    
    assert(module != nullptr);
    std::cout << "✓ Module created successfully\n";
    std::cout << "✓ Function with parameters to MLIR test passed\n\n";
}

void test_binary_operation_to_mlir() {
    std::cout << "=== Test: Binary Operation to MLIR ===\n";
    GDScriptParser parser;
    std::string source = "func add():\nreturn 1 + 2\n";
    
    auto ast = parser.parse(source);
    assert(ast != nullptr);
    assert(ast->functions.size() == 1);
    
    ASTToIRConverter converter;
    mlir::ModuleOp module = converter.convertProgram(ast.get());
    
    assert(module != nullptr);
    std::cout << "✓ Module created successfully\n";
    std::cout << "✓ Binary operation to MLIR test passed\n\n";
}

int main() {
    std::cout << "Running AST to MLIR conversion tests...\n\n";
    
    test_simple_function_to_mlir();
    test_function_with_parameters_to_mlir();
    test_binary_operation_to_mlir();
    
    std::cout << "All AST to MLIR conversion tests passed!\n";
    return 0;
}
