#include "doctest.h"
#include "test_compiler_helpers.h"

TEST_SUITE("Compiler - Code Generation Quality") {
    TEST_CASE("Generated code size is reasonable") {
        std::string source = R"(func test():
    return 42
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Simple function should generate at least some code
        CHECK(result.codeSize >= 16); // Minimum reasonable size
        
        // But not excessively large
        CHECK(result.codeSize < 1024); // Should be compact
    }
    
    TEST_CASE("Multiple functions generate separate code") {
        std::string source = R"(func func1():
    return 1
func func2():
    return 2
)";
        
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        CHECK(result.ast->functions.size() == 2);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Complex expression generates more code") {
        std::string simple = R"(func test():
    return 1
)";
        std::string complex = R"(func test():
    return 1 + 2 * 3 - 4 / 2
)";
        
        auto simpleResult = compileGDScript(simple);
        auto complexResult = compileGDScript(complex);
        
        REQUIRE(simpleResult.success);
        REQUIRE(complexResult.success);
        
        // Complex expression should generate more code
        CHECK(complexResult.codeSize >= simpleResult.codeSize);
    }
}
