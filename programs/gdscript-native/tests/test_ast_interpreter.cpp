#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/ast_interpreter.h"
#include <string>

using namespace gdscript;

TEST_SUITE("AST Interpreter - Core Functionality") {
    
    TEST_CASE("1. Simple function with return") {
        std::string source = R"(func test():
    return 42
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        
        CHECK(result.success);
        CHECK(std::holds_alternative<int64_t>(result.return_value));
        CHECK(std::get<int64_t>(result.return_value) == 42);
    }
    
    TEST_CASE("2. Function with parameters") {
        std::string source = R"(func add(a: int, b: int):
    return a + b
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        std::vector<ASTInterpreter::Value> args;
        args.push_back(ASTInterpreter::Value{int64_t(5)});
        args.push_back(ASTInterpreter::Value{int64_t(3)});
        
        auto result = interpreter.execute_function(ast.get(), "add", args);
        
        CHECK(result.success);
        CHECK(std::holds_alternative<int64_t>(result.return_value));
        CHECK(std::get<int64_t>(result.return_value) == 8);
    }
    
    TEST_CASE("3. Binary arithmetic operations") {
        std::string source = R"(func test():
    return 10 + 5 * 2
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 20); // 10 + (5 * 2) = 20
    }
    
    TEST_CASE("4. Comparison operators") {
        std::string source = R"(func test():
    return 5 == 5
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 1); // true = 1
    }
    
    TEST_CASE("5. Variable declaration and assignment") {
        std::string source = R"(func test():
    var x = 10
    x = 20
    return x
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 20);
    }
    
    TEST_CASE("6. If/else statement") {
        std::string source = R"(func test():
    if 5 > 3:
        return 1
    else:
        return 0
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 1);
    }
    
    TEST_CASE("7. While loop") {
        std::string source = R"(func test():
    var i = 0
    while i < 5:
        i = i + 1
    return i
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 5);
    }
    
    TEST_CASE("8. Function calls") {
        std::string source = R"(func add(a: int, b: int):
    return a + b

func test():
    return add(3, 4)
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute_function(ast.get(), "test");
        
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 7);
    }
    
    TEST_CASE("9. Complex expression with variables") {
        std::string source = R"(func test():
    var a = 10
    var b = 5
    return a * b + 2
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 52); // 10 * 5 + 2 = 52
    }
    
    TEST_CASE("10. Nested function calls") {
        std::string source = R"(func double(x: int):
    return x * 2

func add(a: int, b: int):
    return a + b

func test():
    return add(double(3), double(4))
)";
        GDScriptParser parser;
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        ASTInterpreter interpreter;
        auto result = interpreter.execute_function(ast.get(), "test");
        
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 14); // (3*2) + (4*2) = 6 + 8 = 14
    }
}

