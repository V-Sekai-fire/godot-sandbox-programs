// Test AST building with various GDScript constructs
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include <iostream>
#include <string>
#include <cassert>

using namespace gdscript;

void test_simple_function() {
    std::cout << "=== Test: Simple Function ===\n";
    GDScriptParser parser;
    std::string source = "func hello():\nreturn 42\n";
    
    auto ast = parser.parse(source);
    assert(ast != nullptr);
    assert(ast->functions.size() == 1);
    assert(ast->functions[0]->name == "hello");
    assert(ast->functions[0]->body.size() == 1);
    
    auto ret_stmt = dynamic_cast<ReturnStatement*>(ast->functions[0]->body[0].get());
    assert(ret_stmt != nullptr);
    assert(ret_stmt->value != nullptr);
    
    auto lit = dynamic_cast<LiteralExpr*>(ret_stmt->value.get());
    assert(lit != nullptr);
    assert(std::get<int64_t>(lit->value) == 42);
    
    std::cout << "✓ Simple function test passed\n\n";
}

void test_function_with_parameters() {
    std::cout << "=== Test: Function with Parameters ===\n";
    GDScriptParser parser;
    std::string source = "func add(a, b):\nreturn a\n";
    
    auto ast = parser.parse(source);
    assert(ast != nullptr);
    assert(ast->functions.size() == 1);
    assert(ast->functions[0]->name == "add");
    assert(ast->functions[0]->parameters.size() == 2);
    assert(ast->functions[0]->parameters[0].first == "a");
    assert(ast->functions[0]->parameters[1].first == "b");
    
    std::cout << "✓ Function with parameters test passed\n\n";
}

void test_binary_operation() {
    std::cout << "=== Test: Binary Operation ===\n";
    GDScriptParser parser;
    std::string source = "func add():\nreturn 1 + 2\n";
    
    auto ast = parser.parse(source);
    assert(ast != nullptr);
    assert(ast->functions.size() == 1);
    assert(ast->functions[0]->body.size() == 1);
    
    auto ret_stmt = dynamic_cast<ReturnStatement*>(ast->functions[0]->body[0].get());
    assert(ret_stmt != nullptr);
    assert(ret_stmt->value != nullptr);
    
    auto binop = dynamic_cast<BinaryOpExpr*>(ret_stmt->value.get());
    assert(binop != nullptr);
    assert(binop->op == "+");
    assert(binop->left != nullptr);
    assert(binop->right != nullptr);
    
    std::cout << "✓ Binary operation test passed\n\n";
}

void test_variable_declaration() {
    std::cout << "=== Test: Variable Declaration ===\n";
    GDScriptParser parser;
    std::string source = "func test():\nvar x = 10\nreturn x\n";
    
    auto ast = parser.parse(source);
    assert(ast != nullptr);
    assert(ast->functions.size() == 1);
    assert(ast->functions[0]->body.size() == 2);
    
    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->functions[0]->body[0].get());
    assert(var_decl != nullptr);
    assert(var_decl->name == "x");
    assert(var_decl->initializer != nullptr);
    
    std::cout << "✓ Variable declaration test passed\n\n";
}

int main() {
    std::cout << "Running AST building tests...\n\n";
    
    test_simple_function();
    test_function_with_parameters();
    test_binary_operation();
    test_variable_declaration();
    
    std::cout << "All AST building tests passed!\n";
    return 0;
}

