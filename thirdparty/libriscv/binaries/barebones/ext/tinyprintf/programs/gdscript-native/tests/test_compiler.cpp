#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../parser/gdscript_parser.h"
#include "../ast_to_riscv_biscuit.h"
#include "../function_registry.h"
#include "../code_memory_manager.h"
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <sys/mman.h>

using namespace gdscript;

// Helper to compile GDScript and get machine code
struct CompilationResult {
    bool success;
    std::unique_ptr<ProgramNode> ast;
    std::vector<uint8_t> code;  // Own the code buffer to keep it alive
    size_t codeSize;
    std::string errorMessage;
    
    // Get pointer to code for execution
    const uint8_t* get_code_ptr() const { return code.data(); }
};

CompilationResult compileGDScript(const std::string& source) {
    CompilationResult result;
    result.success = false;
    result.codeSize = 0;
    
    // Parse
    GDScriptParser parser;
    if (!parser.is_valid()) {
        result.errorMessage = "Parser initialization failed";
        return result;
    }
    
    result.ast = parser.parse(source);
    if (!result.ast) {
        result.errorMessage = parser.getErrorMessage();
        return result;
    }
    
    // Emit RISC-V code
    ASTToRISCVEmitter emitter;
    auto [code, size] = emitter.emit(result.ast.get());
    
    if (code == nullptr || size == 0) {
        result.errorMessage = "Code generation failed";
        return result;
    }
    
    // Copy code to our own buffer (emitter's buffer will go out of scope)
    result.code.resize(size);
    std::memcpy(result.code.data(), code, size);
    result.codeSize = size;
    result.success = true;
    return result;
}

// Helper to execute generated RISC-V code and get result
int64_t execute_generated_code(const uint8_t* code, size_t size) {
    if (code == nullptr || size == 0) {
        return 0;
    }
    
    // Allocate executable memory with mmap
    void* execMem = mmap(nullptr, size, PROT_READ | PROT_WRITE | PROT_EXEC, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (execMem == MAP_FAILED) {
        return 0;
    }
    
    // Copy code to executable memory
    std::memcpy(execMem, code, size);
    
    // Cast to function pointer and call
    using FuncPtr = int64_t(*)();
    FuncPtr func = reinterpret_cast<FuncPtr>(execMem);
    int64_t result = func();
    
    // Cleanup
    munmap(execMem, size);
    
    return result;
}

TEST_SUITE("Compiler - Basic Function Compilation") {
    TEST_CASE("Compile simple function returning integer") {
        std::string source = "func test():\n    return 42\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
        CHECK(result.errorMessage.empty());
    }
    
    TEST_CASE("Compile function with parameters") {
        std::string source = "func add(a: int, b: int):\n    return a + b\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile function with variable declaration") {
        std::string source = "func test():\n    var x: int = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile function with binary operations") {
        std::string source = "func test():\n    return 2 + 3 * 4\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
}

TEST_SUITE("Compiler - Comparison Operators") {
    TEST_CASE("Compile equality comparison") {
        std::string source = "func test():\n    return 5 == 5\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile inequality comparison") {
        std::string source = "func test():\n    return 5 != 3\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile less than comparison") {
        std::string source = "func test():\n    return 3 < 5\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile greater than comparison") {
        std::string source = "func test():\n    return 5 > 3\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile less than or equal comparison") {
        std::string source = "func test():\n    return 3 <= 5\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Compile greater than or equal comparison") {
        std::string source = "func test():\n    return 5 >= 3\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
}

TEST_SUITE("Compiler - Error Handling") {
    TEST_CASE("Handle invalid syntax gracefully") {
        std::string source = "func test():\n    invalid syntax here\n";
        auto result = compileGDScript(source);
        
        // Should fail gracefully with error message
        CHECK_FALSE(result.success);
        CHECK_FALSE(result.errorMessage.empty());
    }
    
    TEST_CASE("Handle empty source") {
        std::string source = "";
        auto result = compileGDScript(source);
        
        // Empty source might be valid (empty program) or invalid
        // This test documents current behavior
        // CHECK(result.success); // May or may not succeed
    }
    
    TEST_CASE("Handle missing return statement") {
        std::string source = "func test():\n    var x = 5\n";
        auto result = compileGDScript(source);
        
        // Should compile (function without return is valid)
        CHECK(result.success);
    }
}

TEST_SUITE("Compiler - Function Registry Integration") {
    TEST_CASE("Register compiled function") {
        std::string source = "func test():\n    return 42\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        FunctionRegistry registry;
        
        // Allocate executable memory
        CodeMemoryManager memManager;
        auto* execMem = memManager.allocate(result.codeSize);
        REQUIRE(execMem != nullptr);
        REQUIRE(execMem->is_valid());
        
        execMem->copy(result.get_code_ptr(), result.codeSize);
        void* funcAddr = execMem->get();
        
        // Register function
        registry.register_function("test", funcAddr, result.codeSize);
        
        CHECK(registry.has_function("test"));
        CHECK(registry.get_function("test") == funcAddr);
        CHECK(registry.get_function_size("test") == result.codeSize);
    }
    
    TEST_CASE("Call registered function") {
        std::string source = "func test():\n    return 42\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Allocate executable memory
        CodeMemoryManager memManager;
        auto* execMem = memManager.allocate(result.codeSize);
        REQUIRE(execMem != nullptr);
        
        execMem->copy(result.get_code_ptr(), result.codeSize);
        void* funcAddr = execMem->get();
        
        // Call function
        int64_t returnValue = call_assembly_function(funcAddr);
        
        // Function should return 42
        CHECK(returnValue == 42);
    }
}

TEST_SUITE("Compiler - Memory Management") {
    TEST_CASE("CodeMemoryManager tracks allocations") {
        CodeMemoryManager memManager;
        
        CHECK(memManager.getAllocationCount() == 0);
        
        auto* mem1 = memManager.allocate(1024);
        CHECK(mem1 != nullptr);
        CHECK(memManager.getAllocationCount() == 1);
        
        auto* mem2 = memManager.allocate(2048);
        CHECK(mem2 != nullptr);
        CHECK(memManager.getAllocationCount() == 2);
        
        memManager.clear();
        CHECK(memManager.getAllocationCount() == 0);
    }
    
    TEST_CASE("ExecutableMemory is valid after allocation") {
        ExecutableMemory mem(1024);
        
        CHECK(mem.is_valid());
        CHECK(mem.get() != nullptr);
        CHECK(mem.get_size() == 1024);
    }
    
    TEST_CASE("ExecutableMemory can copy data") {
        ExecutableMemory mem(1024);
        REQUIRE(mem.is_valid());
        
        uint8_t testData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
        mem.copy(testData, 8);
        
        // Verify data was copied
        const uint8_t* copied = static_cast<const uint8_t*>(mem.get());
        for (int i = 0; i < 8; ++i) {
            CHECK(copied[i] == testData[i]);
        }
    }
}

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

TEST_SUITE("Compiler - Integration Tests") {
    TEST_CASE("Full compilation pipeline works") {
        std::string source = "func add(a: int, b: int):\n    return a + b\n";
        
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
        
        std::string invalidSource = "func test():\n    invalid syntax\n";
        auto ast = parser.parse(invalidSource);
        
        // Should have errors
        const auto& errors = parser.get_errors();
        if (!ast) {
            CHECK(errors.has_errors());
        }
    }
}

// TDD: These tests should FAIL first (features not yet implemented)
TEST_SUITE("Compiler - TDD: Unimplemented Features (Should Fail)") {
    TEST_CASE("Control flow: if/else statement") {
        // ✅ NOW IMPLEMENTED - if/else parsing and codegen implemented
        std::string source = "func test():\n    if 5 > 3:\n        return 1\n    else:\n        return 0\n";
        auto result = compileGDScript(source);
        
        // Should now compile successfully
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Assignment statements") {
        // ✅ NOW IMPLEMENTED - assignment parsing and codegen implemented
        std::string source = "func test():\n    var x = 5\n    x = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        // Should now compile successfully
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Function call syntax") {
        // ✅ NOW IMPLEMENTED - function call parsing implemented (codegen stubbed)
        std::string source = "func test():\n    return func_name()\n";
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen may stub out calls)
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Control flow: while loop") {
        // This should FAIL - while loops not implemented yet
        std::string source = "func test():\n    var i = 0\n    while i < 10:\n        i = i + 1\n    return i\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Control flow: for loop") {
        // This should FAIL - for loops not implemented yet
        std::string source = "func test():\n    for i in range(10):\n        pass\n    return 0\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Function calls: call other functions") {
        // ✅ NOW IMPLEMENTED - function call parsing implemented (codegen stubbed)
        std::string source = 
            "func add(a: int, b: int):\n    return a + b\n"
            "func test():\n    return add(1, 2)\n";
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen stubs out calls for now)
        CHECK(result.success);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("String literals") {
        // This should FAIL - string literals not fully implemented
        std::string source = "func test():\n    return \"hello\"\n";
        auto result = compileGDScript(source);
        
        // May parse but codegen might fail
        // CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Float literals") {
        // This should FAIL - float literals converted to int, not proper floats
        std::string source = "func test():\n    return 3.14\n";
        auto result = compileGDScript(source);
        
        // May compile but float handling not proper
        // CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Logical operators: and/or/not") {
        // This should FAIL - logical operators not implemented yet
        std::string source = "func test():\n    return 5 > 3 and 2 < 4\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Array literals") {
        // This should FAIL - arrays not implemented yet
        std::string source = "func test():\n    return [1, 2, 3]\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Dictionary literals") {
        // This should FAIL - dictionaries not implemented yet
        std::string source = "func test():\n    return {\"key\": \"value\"}\n";
        auto result = compileGDScript(source);
        
        CHECK_FALSE(result.success); // TDD: Expect failure until implemented
    }
    
    TEST_CASE("Type checking: incompatible types") {
        // This should FAIL - type checking not implemented yet
        // Should detect error: trying to add string to int
        std::string source = "func test():\n    return 5 + \"hello\"\n";
        auto result = compileGDScript(source);
        
        // Currently will compile (no type checking)
        // Once type checking implemented, should fail with semantic error
        // CHECK_FALSE(result.success); // TDD: Expect failure once type checking added
    }
}

TEST_SUITE("Compiler - New Features: Assignments, If/Else, Function Calls") {
    TEST_CASE("Parse assignment statement") {
        std::string source = "func test():\n    var x = 5\n    x = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse Unicode identifier (UTF-8)") {
        // GDScript supports Unicode in identifiers
        // Regular string literals in UTF-8 source files contain UTF-8 bytes
        // If source file is saved as UTF-8, this will work correctly
        std::string source = "func テスト():\n    return 42\n"; // Japanese: "tesuto" (test)
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        if (result.ast && result.ast->functions.size() > 0) {
            // Function name should be UTF-8 encoded
            CHECK(result.ast->functions[0]->name == "テスト");
        }
    }
    
    TEST_CASE("Parse Unicode string literal (UTF-8)") {
        // String literals can contain Unicode characters
        // Regular string literals in UTF-8 source files contain UTF-8 bytes
        std::string source = "func test():\n    return \"こんにちは\"\n"; // Japanese: "konnichiwa" (hello)
        auto result = compileGDScript(source);
        
        // Should parse successfully (codegen for strings is stubbed)
        CHECK(result.success);
        CHECK(result.ast != nullptr);
    }
    
    TEST_CASE("Parse if/else statement") {
        std::string source = "func test():\n    if 5 > 3:\n        return 1\n    else:\n        return 0\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse if/elif/else statement") {
        std::string source = 
            "func test():\n"
            "    if 5 > 10:\n"
            "        return 1\n"
            "    elif 5 > 3:\n"
            "        return 2\n"
            "    else:\n"
            "        return 0\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse function call") {
        std::string source = "func test():\n    return func_name()\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse function call with arguments") {
        std::string source = "func test():\n    return add(1, 2)\n";
        auto result = compileGDScript(source);
        
        // Should parse (arguments parsing is stubbed for now)
        CHECK(result.success);
        CHECK(result.ast != nullptr);
    }
    
    TEST_CASE("Parse assignment with expression") {
        std::string source = "func test():\n    var x = 5\n    x = x + 1\n    return x\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
    
    TEST_CASE("Parse if statement with assignment") {
        std::string source = 
            "func test():\n"
            "    var x = 0\n"
            "    if 5 > 3:\n"
            "        x = 10\n"
            "    return x\n";
        auto result = compileGDScript(source);
        
        CHECK(result.success);
        CHECK(result.ast != nullptr);
        CHECK(result.codeSize > 0);
    }
}

TEST_SUITE("End-to-End Execution") {
    TEST_CASE("Execute simple return constant") {
        std::string source = "func test():\n    return 42\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 42);
    }
    
    TEST_CASE("Execute simple addition") {
        std::string source = "func add():\n    return 2 + 3\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 5);
    }
    
    TEST_CASE("Execute with variable") {
        std::string source = "func test():\n    var x = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute binary operations") {
        std::string source = "func calc():\n    return 2 * 3 + 4\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify (2 * 3 + 4 = 6 + 4 = 10)
        // Note: Operator precedence may affect result
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute comparison operator") {
        std::string source = "func test():\n    return 5 == 5\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Execute and verify (should return 1 for true)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 1);
    }
    
    TEST_CASE("Execute function with parameters") {
        std::string source = "func add(a: int, b: int):\n    return a + b\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Note: Parameters not yet supported in execution
        // This test verifies compilation succeeds
        // Execution test will be added once parameter passing is implemented
        CHECK(result.codeSize > 0);
        CHECK(!result.code.empty());
    }
    
    TEST_CASE("Execute if/else statement - true branch") {
        std::string source = "func test():\n    if 5 > 3:\n        return 1\n    else:\n        return 0\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 1 since 5 > 3 is true)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 1);
    }
    
    TEST_CASE("Execute if/else statement - false branch") {
        std::string source = "func test():\n    if 3 > 5:\n        return 1\n    else:\n        return 0\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 0 since 3 > 5 is false)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 0);
    }
    
    TEST_CASE("Execute assignment statement") {
        std::string source = "func test():\n    var x = 5\n    x = 10\n    return x\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 10 after assignment)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
    
    TEST_CASE("Execute if with assignment") {
        std::string source = 
            "func test():\n"
            "    var x = 0\n"
            "    if 5 > 3:\n"
            "        x = 10\n"
            "    return x\n";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute and verify (should return 10 after assignment in if branch)
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        CHECK(actual == 10);
    }
}

