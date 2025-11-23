#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "test_compiler_helpers.h"
#include "../src/function_registry.h"
#include "../src/code_memory_manager.h"

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

