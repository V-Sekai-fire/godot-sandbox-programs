#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "test_compiler_helpers.h"

TEST_SUITE("Compiler - Code Generation Quality") {
    TEST_CASE("Generated code size is reasonable") {
        std::string source = "func test():\n    return 42\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Simple function should generate at least some code
        CHECK(result.codeSize >= 16); // Minimum reasonable size
        
        // But not excessively large
        CHECK(result.codeSize < 1024); // Should be compact
    }
    
    TEST_CASE("Multiple functions generate separate code") {
        std::string source = 
            "func func1():\n    return 1\n"
            "func func2():\n    return 2\n";
        
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        CHECK(result.ast->functions.size() == 2);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Complex expression generates more code") {
        std::string simple = "func test():\n    return 1\n";
        std::string complex = "func test():\n    return 1 + 2 * 3 - 4 / 2\n";
        
        auto simpleResult = compileGDScript(simple);
        auto complexResult = compileGDScript(complex);
        
        REQUIRE(simpleResult.success);
        REQUIRE(complexResult.success);
        
        // Complex expression should generate more code
        CHECK(complexResult.codeSize >= simpleResult.codeSize);
    }
}

