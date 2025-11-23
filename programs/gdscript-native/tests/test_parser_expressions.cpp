#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include <string>
#include <memory>

using namespace gdscript;

TEST_SUITE("Literal Parsing") {
    TEST_CASE("Parse integer literal") {
        GDScriptParser parser;
        std::string source = R"(func test():
return 42
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        // TODO: Verify AST structure once semantic actions are fully working
    }
    
    TEST_CASE("Parse negative integer literal") {
        GDScriptParser parser;
        std::string source = R"(func test():
return -42
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse string literal") {
        GDScriptParser parser;
        std::string source = R"(func test():
return "hello"
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse boolean literals") {
        GDScriptParser parser;
        std::string source = R"(func test():
return true
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse null literal") {
        GDScriptParser parser;
        std::string source = R"(func test():
return null
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}

TEST_SUITE("Identifier Parsing") {
    TEST_CASE("Parse identifier in return") {
        GDScriptParser parser;
        std::string source = R"(func test():
return x
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse function name") {
        GDScriptParser parser;
        std::string source = R"(func my_function():
return 0
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        // TODO: Verify function name is "my_function"
    }
}

TEST_SUITE("Binary Operations") {
    TEST_CASE("Parse addition") {
        GDScriptParser parser;
        std::string source = R"(func test():
return 1 + 2
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse subtraction") {
        GDScriptParser parser;
        std::string source = R"(func test():
return 5 - 3
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse multiplication") {
        GDScriptParser parser;
        std::string source = R"(func test():
return 2 * 3
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse comparison") {
        GDScriptParser parser;
        std::string source = R"(func test():
return 1 == 2
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}

TEST_SUITE("Complex Expressions") {
    TEST_CASE("Parse nested binary operations") {
        GDScriptParser parser;
        std::string source = R"(func test():
return 1 + 2 * 3
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
    
    TEST_CASE("Parse parenthesized expression") {
        GDScriptParser parser;
        std::string source = R"(func test():
return (1 + 2) * 3
)";
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
    }
}
