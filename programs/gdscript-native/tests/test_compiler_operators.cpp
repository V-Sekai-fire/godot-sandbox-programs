#include "doctest.h"
#include "test_compiler_helpers.h"

TEST_SUITE("Compiler - Comparison Operators") {
    TEST_CASE("Compile equality comparison") {
        std::string source = R"(func test():
    return 5 == 5
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile inequality comparison") {
        std::string source = R"(func test():
    return 5 != 3
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile less than comparison") {
        std::string source = R"(func test():
    return 3 < 5
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile greater than comparison") {
        std::string source = R"(func test():
    return 5 > 3
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile less than or equal comparison") {
        std::string source = R"(func test():
    return 3 <= 5
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile greater than or equal comparison") {
        std::string source = R"(func test():
    return 5 >= 3
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
}
