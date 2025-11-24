#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include "../src/ast_interpreter.h"
#include <string>

using namespace gdscript;

TEST_SUITE("Adhoc Tests - Converted from Manual Testing") {
    
    TEST_CASE("1. Simple function (no leading newline)") {
        std::string source = R"(func hello():
    return 42
)";
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        CHECK(ast->functions.size() == 1);
        CHECK(ast->functions[0]->name == "hello");
        CHECK(ast->functions[0]->body.size() == 1);
        
        // Test interpreter execution
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
        REQUIRE(parser.is_valid());
        
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        CHECK(ast->functions.size() == 1);
        CHECK(ast->functions[0]->name == "add");
        CHECK(ast->functions[0]->parameters.size() == 2);
        CHECK(ast->functions[0]->parameters[0].first == "a");
        CHECK(ast->functions[0]->parameters[1].first == "b");
        
        // Test interpreter execution with arguments
        ASTInterpreter interpreter;
        std::vector<ASTInterpreter::Value> args;
        args.push_back(ASTInterpreter::Value{int64_t(5)});
        args.push_back(ASTInterpreter::Value{int64_t(3)});
        
        auto result = interpreter.execute_function(ast.get(), "add", args);
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 8);
    }
    
    TEST_CASE("3. Variable declaration") {
        std::string source = R"(func test():
    var x = 10
    return x
)";
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        CHECK(ast->functions.size() == 1);
        CHECK(ast->functions[0]->body.size() == 2); // var declaration + return
        
        // Test interpreter execution
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 10);
    }
    
    TEST_CASE("4. Binary operations") {
        std::string source = R"(func calc():
    return 1 + 2 * 3
)";
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        CHECK(ast->functions.size() == 1);
        
        // Test interpreter execution (operator precedence: 1 + (2 * 3) = 7)
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 7);
    }
    
    TEST_CASE("5. Different literals - boolean") {
        std::string source = R"(func test():
    return true
)";
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        CHECK(ast->functions.size() == 1);
        
        // Test interpreter execution
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        CHECK(result.success);
        // Boolean literals are stored as int64_t (1 for true, 0 for false)
        CHECK(std::holds_alternative<int64_t>(result.return_value));
        CHECK(std::get<int64_t>(result.return_value) == 1);
    }
    
    TEST_CASE("6. Different literals - null") {
        std::string source = R"(func test():
    return null
)";
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        // Test interpreter execution
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        CHECK(result.success);
        CHECK(std::holds_alternative<std::nullptr_t>(result.return_value));
    }
    
    TEST_CASE("7. Complex expression with variables") {
        std::string source = R"(func test():
    var a = 10
    var b = 5
    return a * b + 2
)";
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        // Test interpreter execution
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 52); // 10 * 5 + 2 = 52
    }
    
    TEST_CASE("8. Comparison operators") {
        std::string source = R"(func test():
    return 5 > 3
)";
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        
        // Test interpreter execution
        ASTInterpreter interpreter;
        auto result = interpreter.execute(ast.get());
        CHECK(result.success);
        CHECK(std::get<int64_t>(result.return_value) == 1); // true = 1
    }
}
