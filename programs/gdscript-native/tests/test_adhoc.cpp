// Adhoc manual testing script
// Compile with: g++ -std=c++17 -I.. -I../parser test_adhoc.cpp ../parser/gdscript_parser.cpp -o test_adhoc

#include "../parser/gdscript_parser.h"
#include "../parser/ast.h"
#include <iostream>
#include <string>

using namespace gdscript;

void test_parse(const std::string& name, const std::string& source) {
    std::cout << "\n=== Testing: " << name << " ===\n";
    std::cout << "Source:\n" << source << "\n";
    
    GDScriptParser parser;
    if (!parser.is_valid()) {
        std::cout << "ERROR: Parser initialization failed!\n";
        return;
    }
    
    auto ast = parser.parse(source);
    if (!ast) {
        std::cout << "ERROR: Parsing failed!\n";
        std::cout << "Error: " << parser.getErrorMessage() << "\n";
        return;
    }
    
    std::cout << "SUCCESS: Parsed successfully!\n";
    std::cout << "Functions: " << ast->functions.size() << "\n";
    std::cout << "Statements: " << ast->statements.size() << "\n";
    
    if (!ast->functions.empty()) {
        std::cout << "First function: " << ast->functions[0]->name << "\n";
        std::cout << "Parameters: " << ast->functions[0]->parameters.size() << "\n";
        std::cout << "Body statements: " << ast->functions[0]->body.size() << "\n";
    }
}

int main() {
    std::cout << "=== GDScript Parser Adhoc Tests ===\n";
    
    // Test 1: Simple function (no leading newline)
    test_parse("Simple function", "func hello():\nreturn 42\n");
    
    // Test 2: Function with parameters
    test_parse("Function with parameters", "func add(a: int, b: int):\nreturn a + b\n");
    
    // Test 3: Variable declaration
    test_parse("Variable declaration", "func test():\nvar x = 10\nreturn x\n");
    
    // Test 4: Binary operations
    test_parse("Binary operations", "func calc():\nreturn 1 + 2 * 3\n");
    
    // Test 5: Different literals
    test_parse("Different literals", "func test():\nreturn true\n");
    
    std::cout << "\n=== Tests Complete ===\n";
    return 0;
}

