#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/ast_to_riscv_biscuit.h"
#include "../src/function_registry.h"
#include "../src/code_memory_manager.h"
#include <string>
#include <memory>

using namespace gdscript;

TEST_SUITE("Compiler - Integration Tests") {
    TEST_CASE("Full compilation pipeline works") {
        std::string source = R"(func add(a: int, b: int):
    return a + b
)";
        
        // Parse
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        auto ast = parser.parse(source);
        REQUIRE(ast != nullptr);
        REQUIRE(ast->functions.size() == 1);
        
        // Emit
        ASTToRISCVEmitter emitter;
        auto [code, size] = emitter.emit(ast.get());
        REQUIRE(code != nullptr);
        REQUIRE(size > 0);
        
        // Memory management
        CodeMemoryManager memManager;
        auto* execMem = memManager.allocate(size);
        REQUIRE(execMem != nullptr);
        execMem->copy(code, size);
        
        // Function registry
        FunctionRegistry registry;
        registry.register_function("add", execMem->get(), size);
        CHECK(registry.has_function("add"));
    }
    
    TEST_CASE("Error collection works") {
        GDScriptParser parser;
        REQUIRE(parser.is_valid());
        
        std::string invalidSource = R"(func test():
    invalid syntax
)";
        auto ast = parser.parse(invalidSource);
        
        // Should have errors
        const auto& errors = parser.get_errors();
        if (!ast) {
            CHECK(errors.has_errors());
        }
    }
}
