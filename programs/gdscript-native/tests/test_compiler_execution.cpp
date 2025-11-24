#include "doctest.h"
#include "test_compiler_helpers.h"

TEST_SUITE("End-to-End Execution") {
    TEST_CASE("Execute simple return constant") {
        std::string source = R"(func test():
    return 42
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 42);
    }
    
    TEST_CASE("Execute simple addition") {
        std::string source = R"(func add():
    return 2 + 3
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 5);
    }
    
    TEST_CASE("Execute with variable") {
        std::string source = R"(func test():
    var x = 10
    return x
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute binary operations") {
        std::string source = R"(func calc():
    return 2 * 3 + 4
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify (2 * 3 + 4 = 6 + 4 = 10)
        // Note: Operator precedence may affect result
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute comparison operator") {
        std::string source = R"(func test():
    return 5 == 5
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify (should return 1 for true)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 1);
    }
    
    TEST_CASE("Execute function with parameters") {
        std::string source = R"(func add(a: int, b: int):
    return a + b
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Note: Parameters not yet supported in execution
        // This test verifies compilation succeeds
        // Execution test will be added once parameter passing is implemented
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Execute if/else statement - true branch") {
        std::string source = R"(func test():
    if 5 > 3:
        return 1
    else:
        return 0
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 1 since 5 > 3 is true)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 1);
    }
    
    TEST_CASE("Execute if/else statement - false branch") {
        std::string source = R"(func test():
    if 3 > 5:
        return 1
    else:
        return 0
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 0 since 3 > 5 is false)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 0);
    }
    
    TEST_CASE("Execute assignment statement") {
        std::string source = R"(func test():
    var x = 5
    x = 10
    return x
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 10 after assignment)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute if with assignment") {
        std::string source = R"(func test():
    var x = 0
    if 5 > 3:
        x = 10
    return x
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 10 after assignment in if branch)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
}
