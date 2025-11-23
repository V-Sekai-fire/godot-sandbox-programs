#include "doctest.h"
#include "test_compiler_helpers.h"

TEST_SUITE("Compiler - Basic Function Compilation") {
    TEST_CASE("Compile simple function returning integer") {
        std::string source = R"(func test():
    return 42
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
        CHECK(result.errorMessage.empty());
    }
    
    TEST_CASE("Compile function with parameters") {
        std::string source = R"(func add(a: int, b: int):
    return a + b
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile function with variable declaration") {
        std::string source = R"(func test():
    var x: int = 10
    return x
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile function with binary operations") {
        std::string source = R"(func test():
    return 2 + 3 * 4
)";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
}
