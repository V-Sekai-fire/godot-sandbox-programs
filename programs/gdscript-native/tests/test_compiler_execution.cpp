#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "test_compiler_helpers.h"

TEST_SUITE("End-to-End Execution") {
    TEST_CASE("Execute simple return constant") {
        std::string source = "func test():\n    return 42\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 42);
    }
    
    TEST_CASE("Execute simple addition") {
        std::string source = "func add():\n    return 2 + 3\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 5);
    }
    
    TEST_CASE("Execute with variable") {
        std::string source = "func test():\n    var x = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute binary operations") {
        std::string source = "func calc():\n    return 2 * 3 + 4\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify (2 * 3 + 4 = 6 + 4 = 10)
        // Note: Operator precedence may affect result
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute comparison operator") {
        std::string source = "func test():\n    return 5 == 5\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify (should return 1 for true)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 1);
    }
    
    TEST_CASE("Execute function with parameters") {
        std::string source = "func add(a: int, b: int):\n    return a + b\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Note: Parameters not yet supported in execution
        // This test verifies compilation succeeds
        // Execution test will be added once parameter passing is implemented
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Execute if/else statement - true branch") {
        std::string source = "func test():\n    if 5 > 3:\n        return 1\n    else:\n        return 0\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 1 since 5 > 3 is true)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 1);
    }
    
    TEST_CASE("Execute if/else statement - false branch") {
        std::string source = "func test():\n    if 3 > 5:\n        return 1\n    else:\n        return 0\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 0 since 3 > 5 is false)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 0);
    }
    
    TEST_CASE("Execute assignment statement") {
        std::string source = "func test():\n    var x = 5\n    x = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 10 after assignment)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute if with assignment") {
        std::string source = 
            "func test():\n"
            "    var x = 0\n"
            "    if 5 > 3:\n"
            "        x = 10\n"
            "    return x\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 10 after assignment in if branch)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
}

