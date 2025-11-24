#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

// Test parser only (doesn't require api.hpp)
#include "../../gdscript-native/src/parser/gdscript_parser.h"
#include "../../gdscript-native/src/parser/ast.h"
#include <memory>

using namespace gdscript;

// Test basic VM compilation and execution
TEST_CASE("1. Parse simple function") {
    std::string source = R"(func test():
    return 42
)";
    
    GDScriptParser parser;
    REQUIRE(parser.is_valid());
    
    std::unique_ptr<ProgramNode> ast = parser.parse(source);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->functions.size() == 1);
    CHECK(ast->functions[0]->name == "test");
}

// Note: Bytecode compilation tests require RISC-V API which doesn't compile on macOS
// These tests are disabled for now - will be enabled when testing on RISC-V platform
/*
TEST_CASE("2. Compile simple function to bytecode") {
    // This test requires api.hpp which has RISC-V specific assembly
    // Disabled for macOS testing
}
*/

TEST_CASE("3. Parse function with parameters") {
    std::string source = R"(func add(a, b):
    return a + b
)";
    
    GDScriptParser parser;
    REQUIRE(parser.is_valid());
    
    std::unique_ptr<ProgramNode> ast = parser.parse(source);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->functions.size() == 1);
    CHECK(ast->functions[0]->parameters.size() == 2);
    CHECK(ast->functions[0]->parameters[0].first == "a");
    CHECK(ast->functions[0]->parameters[1].first == "b");
}

TEST_CASE("4. Parse function with variable declaration") {
    std::string source = R"(func test():
    var x = 10
    return x
)";
    
    GDScriptParser parser;
    REQUIRE(parser.is_valid());
    
    std::unique_ptr<ProgramNode> ast = parser.parse(source);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->functions.size() == 1);
    CHECK(ast->functions[0]->name == "test");
    CHECK(ast->functions[0]->body.size() >= 1); // Should have at least var declaration
}

TEST_CASE("5. Parse function with if statement") {
    std::string source = R"(func test():
    if true:
        return 1
    else:
        return 0
)";
    
    GDScriptParser parser;
    REQUIRE(parser.is_valid());
    
    std::unique_ptr<ProgramNode> ast = parser.parse(source);
    REQUIRE(ast != nullptr);
    REQUIRE(ast->functions.size() == 1);
    CHECK(ast->functions[0]->name == "test");
}

