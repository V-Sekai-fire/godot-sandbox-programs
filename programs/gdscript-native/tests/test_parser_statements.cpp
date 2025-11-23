#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include <string>
#include <memory>

using namespace gdscript;

TEST_SUITE("Function Parsing") {
    TEST_CASE("Parse simple function") {
        GDScriptParser parser;
        std::string source = "func hello():\nreturn 42\n";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        CHECK(ast->functions.size() >= 1);
        // TODO: Verify function name and body
    }
    
    TEST_CASE("Parse function with parameters") {
        GDScriptParser parser;
        std::string source = "func add(a: int, b: int):\nreturn a + b\n";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        // TODO: Verify parameters are parsed correctly
    }
    
    TEST_CASE("Parse function with return type") {
        GDScriptParser parser;
        std::string source = "func get_value() -> int:\nreturn 42\n";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        // TODO: Verify return type is parsed
    }
}

TEST_SUITE("Return Statement") {
    TEST_CASE("Parse return with value") {
        GDScriptParser parser;
        std::string source = "func test():\nreturn 42\n";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse return without value") {
        GDScriptParser parser;
        std::string source = "func test():\nreturn\n";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}

TEST_SUITE("Variable Declaration") {
    TEST_CASE("Parse variable declaration") {
        GDScriptParser parser;
        std::string source = "func test():\nvar x = 42\nreturn x\n";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse variable with type hint") {
        GDScriptParser parser;
        std::string source = "func test():\nvar x: int = 42\nreturn x\n";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse variable without initializer") {
        GDScriptParser parser;
        std::string source = "func test():\nvar x\nreturn x\n";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}

