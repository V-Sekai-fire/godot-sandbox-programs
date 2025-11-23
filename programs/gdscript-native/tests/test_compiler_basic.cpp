#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "test_compiler_helpers.h"

TEST_SUITE("Compiler - Basic Function Compilation") {
    TEST_CASE("Compile simple function returning integer") {
        std::string source = "func test():\n    return 42\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
        CHECK(result.errorMessage.empty());
    }
    
    TEST_CASE("Compile function with parameters") {
        std::string source = "func add(a: int, b: int):\n    return a + b\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile function with variable declaration") {
        std::string source = "func test():\n    var x: int = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile function with binary operations") {
        std::string source = "func test():\n    return 2 + 3 * 4\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
}

