#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include <string>
#include <memory>

using namespace gdscript;

TEST_SUITE("Parser Initialization") {
    TEST_CASE("Parser can be created") {
        GDScriptParser parser;
        CHECK(parser.is_valid());
    }
    
    TEST_CASE("Parser handles empty input") {
        GDScriptParser parser;
        std::string source = "";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        CHECK(ast->functions.empty());
        CHECK(ast->statements.empty());
    }
}

