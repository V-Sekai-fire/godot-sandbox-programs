#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../parser/gdscript_parser.h"
#include "../ast_to_ir.h"
#include "../mlir/ir_to_riscv.h"
#include <string>
#include <memory>

using namespace gdscript;

TEST_SUITE("End-to-End Compilation") {
    TEST_CASE("Compile simple function returning constant") {
        GDScriptParser parser;
        std::string source = R"(
func hello():
    return 42
)";
        
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        CHECK(ast->functions.size() == 1);
        
        ASTToIRConverter converter;
        auto module = converter.convertProgram(ast.get());
        CHECK(module != nullptr);
        
        mlir::RISCVEmitter emitter;
        std::string assembly = emitter.emitModule(module);
        CHECK(!assembly.empty());
    }
    
    TEST_CASE("Compile function with addition") {
        GDScriptParser parser;
        std::string source = R"(
func add(a: int, b: int):
    return a + b
)";
        
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        
        ASTToIRConverter converter;
        auto module = converter.convertProgram(ast.get());
        CHECK(module != nullptr);
        
        mlir::RISCVEmitter emitter;
        std::string assembly = emitter.emitModule(module);
        CHECK(!assembly.empty());
    }
    
    TEST_CASE("Compile function with variable") {
        GDScriptParser parser;
        std::string source = R"(
func test():
    var x = 10
    return x
)";
        
        auto ast = parser.parse(source);
        CHECK(ast != nullptr);
        
        ASTToIRConverter converter;
        auto module = converter.convertProgram(ast.get());
        CHECK(module != nullptr);
        
        mlir::RISCVEmitter emitter;
        std::string assembly = emitter.emitModule(module);
        CHECK(!assembly.empty());
    }
}

