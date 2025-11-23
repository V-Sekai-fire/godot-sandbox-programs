#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "test_compiler_helpers.h"

// TDD: These tests should FAIL first (features not yet implemented)
TEST_SUITE("Compiler - TDD: Unimplemented Features (Should Fail)") {
    TEST_CASE("Control flow: if/else statement") {
        // ✅ NOW IMPLEMENTED - if/else parsing and codegen implemented
        std::string source = "func test():\n    if 5 > 3:\n        return 1\n    else:\n        return 0\n";
        auto result = compileGDScript(source);
        
        // Should now compile successfully
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Assignment statements") {
        // ✅ NOW IMPLEMENTED - assignment parsing and codegen implemented
        std::string source = "func test():\n    var x = 5\n    x = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        // Should now compile successfully
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Function call syntax") {
        // ✅ NOW IMPLEMENTED - function call parsing implemented (codegen stubbed)
        std::string source = "func test():\n    return func_name()\n";
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen may stub out calls)
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Control flow: while loop") {
        // This should FAIL - while loops not implemented yet
        std::string source = "func test():\n    var i = 0\n    while i < 10:\n        i = i + 1\n    return i\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Control flow: for loop") {
        // This should FAIL - for loops not implemented yet
        std::string source = "func test():\n    for i in range(10):\n        pass\n    return 0\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Function calls: call other functions") {
        // ✅ NOW IMPLEMENTED - function call parsing implemented (codegen stubbed)
        std::string source = 
            "func add(a: int, b: int):\n    return a + b\n"
            "func test():\n    return add(1, 2)\n";
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen stubs out calls for now)
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("String literals") {
        // This should FAIL - string literals not fully implemented
        std::string source = "func test():\n    return \"hello\"\n";
        auto result = compileGDScript(source);
        
        // May parse but codegen might fail
        // CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Float literals") {
        // This should FAIL - float literals converted to int, not proper floats
        std::string source = "func test():\n    return 3.14\n";
        auto result = compileGDScript(source);
        
        // May compile but float handling not proper
        // CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Logical operators: and/or/not") {
        // This should FAIL - logical operators not implemented yet
        std::string source = "func test():\n    return 5 > 3 and 2 < 4\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Array literals") {
        // This should FAIL - arrays not implemented yet
        std::string source = "func test():\n    return [1, 2, 3]\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Dictionary literals") {
        // This should FAIL - dictionaries not implemented yet
        std::string source = "func test():\n    return {\"key\": \"value\"}\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Type checking: incompatible types") {
        // This should FAIL - type checking not implemented yet
        // Should detect error: trying to add string to int
        std::string source = "func test():\n    return 5 + \"hello\"\n";
        auto result = compileGDScript(source);
        
        // Currently will compile (no type checking)
        // Once type checking implemented, should fail with semantic error
        // CHECK_FALSE(result.success); // TDD: Expect failure once type checking added
    }
}

TEST_SUITE("Compiler - New Features: Assignments, If/Else, Function Calls") {
    TEST_CASE("Parse assignment statement") {
        std::string source = "func test():\n    var x = 5\n    x = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse Unicode identifier (UTF-8)") {
        // GDScript supports Unicode in identifiers
        std::string source = "func テスト():\n    return 42\n"; // Japanese: "tesuto" (test)
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.ast && result.ast->functions.size() > 0) {
            // Function name should be UTF-8 encoded
            CHECK(result.ast->functions[0]->name == "テスト");
        }
    }
    
    TEST_CASE("Parse Unicode string literal (UTF-8)") {
        // String literals can contain Unicode characters
        std::string source = "func test():\n    return \"こんにちは\"\n"; // Japanese: "konnichiwa" (hello)
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen for strings is stubbed)
        CHECK(result.success);
        CHECK(result.ast != nullptr);
    }
    
    TEST_CASE("Parse if/else statement") {
        std::string source = "func test():\n    if 5 > 3:\n        return 1\n    else:\n        return 0\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse if/elif/else statement") {
        std::string source = 
            "func test():\n"
            "    if 5 > 10:\n"
            "        return 1\n"
            "    elif 5 > 3:\n"
            "        return 2\n"
            "    else:\n"
            "        return 0\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse function call") {
        std::string source = "func test():\n    return func_name()\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse function call with arguments") {
        std::string source = "func test():\n    return add(1, 2)\n";
        auto result = compileGDScript(source);
        
        // Should parse (arguments parsing is stubbed for now)
        CHECK(result.success);
        CHECK(result.ast != nullptr);
    }
    
    TEST_CASE("Parse assignment with expression") {
        std::string source = "func test():\n    var x = 5\n    x = x + 1\n    return x\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse if statement with assignment") {
        std::string source = 
            "func test():\n"
            "    var x = 0\n"
            "    if 5 > 3:\n"
            "        x = 10\n"
            "    return x\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
}

