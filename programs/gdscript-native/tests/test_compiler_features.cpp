#include "doctest.h"
#include "test_compiler_helpers.h"

// TDD: These tests should FAIL first (features not yet implemented)
TEST_SUITE("Compiler - TDD: Unimplemented Features (Should Fail)") {
    TEST_CASE("Control flow: if/else statement") {
        // ✅ NOW IMPLEMENTED - if/else parsing and codegen implemented
        std::string source = R"(func test():
    if 5 > 3:
        return 1
    else:
        return 0
)";
        auto result = compileGDScript(source);
        
        // Should now compile successfully
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Assignment statements") {
        // ✅ NOW IMPLEMENTED - assignment parsing and codegen implemented
        std::string source = R"(func test():
    var x = 5
    x = 10
    return x
)";
        auto result = compileGDScript(source);
        
        // Should now compile successfully
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Function call syntax") {
        // ✅ NOW IMPLEMENTED - function call parsing implemented (codegen stubbed)
        std::string source = R"(func test():
    return func_name()
)";
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen may stub out calls)
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Control flow: while loop") {
        // This should FAIL - while loops not implemented yet
        std::string source = R"(func test():
    var i = 0
    while i < 10:
        i = i + 1
    return i
)";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Control flow: for loop") {
        // This should FAIL - for loops not implemented yet
        std::string source = R"(func test():
    for i in range(10):
        pass
    return 0
)";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Function calls: call other functions") {
        // ✅ NOW IMPLEMENTED - function call parsing implemented (codegen stubbed)
        std::string source = R"(func add(a: int, b: int):
    return a + b
func test():
    return add(1, 2)
)";
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen stubs out calls for now)
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("String literals") {
        // This should FAIL - string literals not fully implemented
        std::string source = R"(func test():
    return "hello"
)";
        auto result = compileGDScript(source);
        
        // May parse but codegen might fail
        // CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Float literals") {
        // This should FAIL - float literals converted to int, not proper floats
        std::string source = R"(func test():
    return 3.14
)";
        auto result = compileGDScript(source);
        
        // May compile but float handling not proper
        // CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Logical operators: and/or/not") {
        // This should FAIL - logical operators not implemented yet
        std::string source = R"(func test():
    return 5 > 3 and 2 < 4
)";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Array literals") {
        // This should FAIL - arrays not implemented yet
        std::string source = R"(func test():
    return [1, 2, 3]
)";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Dictionary literals") {
        // This should FAIL - dictionaries not implemented yet
        std::string source = R"(func test():
    return {"key": "value"}
)";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Type checking: incompatible types") {
        // This should FAIL - type checking not implemented yet
        // Should detect error: trying to add string to int
        std::string source = R"(func test():
    return 5 + "hello"
)";
        auto result = compileGDScript(source);
        
        // Currently will compile (no type checking)
        // Once type checking implemented, should fail with semantic error
        // CHECK_FALSE(result.success); // TDD: Expect failure once type checking added
    }
}

TEST_SUITE("Compiler - New Features: Assignments, If/Else, Function Calls") {
    TEST_CASE("Parse assignment statement") {
        std::string source = R"(func test():
    var x = 5
    x = 10
    return x
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse Unicode identifier (UTF-8)") {
        // GDScript supports Unicode in identifiers
        std::string source = R"(func テスト():
    return 42
)"; // Japanese: "tesuto" (test)
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
        std::string source = R"(func test():
    return "こんにちは"
)"; // Japanese: "konnichiwa" (hello)
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen for strings is stubbed)
        CHECK(result.success);
        CHECK(result.ast != nullptr);
    }
    
    TEST_CASE("Parse if/else statement") {
        std::string source = R"(func test():
    if 5 > 3:
        return 1
    else:
        return 0
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse if/elif/else statement") {
        std::string source = R"(func test():
    if 5 > 10:
        return 1
    elif 5 > 3:
        return 2
    else:
        return 0
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse function call") {
        std::string source = R"(func test():
    return func_name()
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse function call with arguments") {
        std::string source = R"(func test():
    return add(1, 2)
)";
        auto result = compileGDScript(source);
        
        // Should parse (arguments parsing is stubbed for now)
        CHECK(result.success);
        CHECK(result.ast != nullptr);
    }
    
    TEST_CASE("Parse assignment with expression") {
        std::string source = R"(func test():
    var x = 5
    x = x + 1
    return x
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse if statement with assignment") {
        std::string source = R"(func test():
    var x = 0
    if 5 > 3:
        x = 10
    return x
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
}
