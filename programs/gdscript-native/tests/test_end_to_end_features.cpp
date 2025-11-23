#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../parser/gdscript_parser.h"
#include "../ast_to_riscv_biscuit.h"
#include "../elf_generator.h"
#include "../function_registry.h"
#include "../code_memory_manager.h"
#include <libriscv/machine.hpp>
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <sys/mman.h>
#include <fstream>
#include <filesystem>

using namespace riscv;

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

// Helper to execute generated RISC-V code directly (mmap)
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

// Helper to generate ELF and execute via libriscv
int64_t execute_via_elf(const std::vector<uint8_t>& code) {
    // Generate ELF
    ELFGenerator elf_gen;
    elf_gen.add_code_section(code.data(), code.size(), ".text");
    auto elf = elf_gen.generate();
    
    if (elf.empty()) {
        return -1;
    }
    
    try {
        // Configure machine options
        MachineOptions<RISCV64> options;
        options.memory_max = 64 << 20; // 64 MiB
        options.allow_write_exec_segment = true;
        options.protect_segments = false;
        options.verbose_loader = false;
        
        // Create RISC-V machine with ELF binary
        std::string_view binary_view{reinterpret_cast<const char*>(elf.data()), elf.size()};
        Machine<RISCV64> machine{binary_view, options};
        
        // Setup Linux environment
        machine.setup_linux({"test_program"});
        machine.setup_linux_syscalls();
        
        // Set instruction limit
        machine.set_max_instructions(1'000'000);
        
        // Execute
        machine.simulate();
        
        // Get exit code (from a0 register, which is register 10)
        int64_t exit_code = machine.cpu.reg(10);
        return exit_code;
        
    } catch (const std::exception& e) {
        // Return error code on exception
        return -1;
    }
}

// Ensure test output directory exists
void ensure_test_output_dir() {
    std::filesystem::path dir = "test_output";
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
}

TEST_SUITE("End-to-End: Fully Supported Features") {
    
    TEST_CASE("1. Simple Functions - Return Integer Constant") {
        std::string source = R"(func test():
    return 42
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Execute via libriscv (ELF)
        int64_t actual = execute_via_elf(result.code);
        CHECK(actual == 42);
    }
    
    TEST_CASE("2. Functions with Parameters - Parse and Compile") {
        std::string source = R"(func add(a: int, b: int):
    return a + b
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.ast != nullptr);
        CHECK(result.ast->functions.size() == 1);
        CHECK(result.ast->functions[0]->parameters.size() == 2);
        CHECK(result.codeSize > 0);
        // Note: Parameter passing execution not yet tested
    }
    
    TEST_CASE("3. Return Statements - Multiple Returns") {
        std::string source1 = R"(func test():
    return 42
)";
        std::string source2 = R"(func test():
    var x = 5
)";
        
        auto result1 = compileGDScript(source1);
        auto result2 = compileGDScript(source2);
        
        REQUIRE(result1.success);
        REQUIRE(result2.success);
        
        int64_t val1 = execute_via_elf(result1.code);
        int64_t val2 = execute_via_elf(result2.code);
        
        CHECK(val1 == 42);
        CHECK(val2 == 0); // Functions without return default to 0
    }
    
    TEST_CASE("4. Integer Literals - Positive and Negative") {
        std::string source1 = R"(func test():
    return 42
)";
        std::string source2 = R"(func test():
    return -42
)";
        
        auto result1 = compileGDScript(source1);
        auto result2 = compileGDScript(source2);
        
        REQUIRE(result1.success);
        REQUIRE(result2.success);
        
        int64_t val1 = execute_via_elf(result1.code);
        int64_t val2 = execute_via_elf(result2.code);
        
        CHECK(val1 == 42);
        CHECK(val2 == -42);
    }
    
    TEST_CASE("5. Boolean Literals - True and False") {
        std::string source1 = R"(func test():
    return true
)";
        std::string source2 = R"(func test():
    return false
)";
        
        auto result1 = compileGDScript(source1);
        auto result2 = compileGDScript(source2);
        
        REQUIRE(result1.success);
        REQUIRE(result2.success);
        
        int64_t val1 = execute_via_elf(result1.code);
        int64_t val2 = execute_via_elf(result2.code);
        
        CHECK(val1 == 1); // true is 1
        CHECK(val2 == 0); // false is 0
    }
    
    TEST_CASE("6. Null Literals") {
        std::string source = R"(func test():
    return null
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 0); // null is 0
    }
    
    TEST_CASE("7. Variable Declarations - With Type Hints and Initializers") {
        std::string source = R"(func test():
    var x: int = 10
    return x
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 10);
    }
    
    TEST_CASE("8. Variable References - Read Variables") {
        std::string source = R"(func test():
    var x = 5
    return x
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 5);
    }
    
    TEST_CASE("9. Binary Arithmetic Operations - All Operators") {
        // Addition
        std::string add = R"(
func test():
    return 2 + 3
)";
        auto result_add = compileGDScript(add);
        REQUIRE(result_add.success);
        CHECK(execute_via_elf(result_add.code) == 5);
        
        // Subtraction
        std::string sub = R"(
func test():
    return 10 - 3
)";
        auto result_sub = compileGDScript(sub);
        REQUIRE(result_sub.success);
        CHECK(execute_via_elf(result_sub.code) == 7);
        
        // Multiplication
        std::string mul = R"(
func test():
    return 2 * 3
)";
        auto result_mul = compileGDScript(mul);
        REQUIRE(result_mul.success);
        CHECK(execute_via_elf(result_mul.code) == 6);
        
        // Division
        std::string div = R"(
func test():
    return 10 / 2
)";
        auto result_div = compileGDScript(div);
        REQUIRE(result_div.success);
        CHECK(execute_via_elf(result_div.code) == 5);
        
        // Modulo
        std::string mod = R"(
func test():
    return 10 % 3
)";
        auto result_mod = compileGDScript(mod);
        REQUIRE(result_mod.success);
        CHECK(execute_via_elf(result_mod.code) == 1);
    }
    
    TEST_CASE("10. Comparison Operators - All Operators") {
        // Equality
        std::string eq = R"(
func test():
    return 5 == 5
)";
        auto result_eq = compileGDScript(eq);
        REQUIRE(result_eq.success);
        CHECK(execute_via_elf(result_eq.code) == 1);
        
        // Inequality
        std::string ne = R"(
func test():
    return 5 != 3
)";
        auto result_ne = compileGDScript(ne);
        REQUIRE(result_ne.success);
        CHECK(execute_via_elf(result_ne.code) == 1);
        
        // Less than
        std::string lt = R"(
func test():
    return 3 < 5
)";
        auto result_lt = compileGDScript(lt);
        REQUIRE(result_lt.success);
        CHECK(execute_via_elf(result_lt.code) == 1);
        
        // Greater than
        std::string gt = R"(
func test():
    return 5 > 3
)";
        auto result_gt = compileGDScript(gt);
        REQUIRE(result_gt.success);
        CHECK(execute_via_elf(result_gt.code) == 1);
        
        // Less than or equal
        std::string le = R"(
func test():
    return 3 <= 5
)";
        auto result_le = compileGDScript(le);
        REQUIRE(result_le.success);
        CHECK(execute_via_elf(result_le.code) == 1);
        
        // Greater than or equal
        std::string ge = R"(
func test():
    return 5 >= 3
)";
        auto result_ge = compileGDScript(ge);
        REQUIRE(result_ge.success);
        CHECK(execute_via_elf(result_ge.code) == 1);
        
        // False cases
        std::string eq_false = R"(
func test():
    return 5 == 3
)";
        auto result_eq_false = compileGDScript(eq_false);
        REQUIRE(result_eq_false.success);
        CHECK(execute_via_elf(result_eq_false.code) == 0);
    }
    
    TEST_CASE("11. Complex Expressions - Operator Precedence") {
        std::string source = R"(func calc():
    return 1 + 2 * 3 - 4 / 2
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Expected: 1 + (2 * 3) - (4 / 2) = 1 + 6 - 2 = 5
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 5);
    }
    
    TEST_CASE("12. Multiple Functions - Compile Multiple Functions") {
        std::string source = R"(func func1():
    return 1
func func2():
    return 2
)";
        
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        CHECK(result.ast->functions.size() == 2);
        CHECK(result.codeSize > 0);
        // Note: Currently only first function is executed
    }
    
    TEST_CASE("13. Functions Without Return - Default Return Value") {
        std::string source = R"(func test():
    var x = 5
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 0); // Default return value
    }
}

TEST_SUITE("End-to-End: New Features (Assignments, If/Else)") {
    TEST_CASE("Assignment Statement") {
        std::string source = R"(func test():
    var x = 5
    x = 10
    return x
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        int64_t actual = execute_via_elf(result.code);
        CHECK(actual == 10);
    }
    
    TEST_CASE("If/Else Statement - True Branch") {
        std::string source = R"(func test():
    if 5 > 3:
        return 1
    else:
        return 0
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        int64_t actual = execute_via_elf(result.code);
        CHECK(actual == 1);
    }
    
    TEST_CASE("If/Else Statement - False Branch") {
        std::string source = R"(func test():
    if 3 > 5:
        return 1
    else:
        return 0
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        int64_t actual = execute_via_elf(result.code);
        CHECK(actual == 0);
    }
    
    TEST_CASE("If with Assignment") {
        std::string source = R"(func test():
    var x = 0
    if 5 > 3:
        x = 10
    return x
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        int64_t actual = execute_via_elf(result.code);
        CHECK(actual == 10);
    }
    
    TEST_CASE("If/Elif/Else Statement") {
        std::string source = R"(func test():
    if 5 > 10:
        return 1
    elif 5 > 3:
        return 2
    else:
        return 0
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        int64_t actual = execute_via_elf(result.code);
        CHECK(actual == 2);
    }
}

TEST_SUITE("End-to-End: Complex Scenarios") {
    
    TEST_CASE("Multiple Variables and Operations") {
        std::string source = R"(func test():
    var a = 10
    var b = 20
    return a + b
)";
        
        auto result = compileGDScript(source);
        REQUIRE(result.success);
        
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 30);
    }
    
    TEST_CASE("Nested Expressions") {
        std::string source = R"(func test():
    return (2 + 3) * 4
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 20); // (2 + 3) * 4 = 5 * 4 = 20
    }
    
    TEST_CASE("Comparison in Expression") {
        std::string source = R"(func test():
    return 5 > 3
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 1);
    }
    
    TEST_CASE("Variable Used in Binary Operation") {
        std::string source = R"(func test():
    var x = 5
    var y = 3
    return x * y
)";
        
        auto result = compileGDScript(source);
        REQUIRE(result.success);
        
        int64_t val = execute_via_elf(result.code);
        CHECK(val == 15);
    }
}

TEST_SUITE("End-to-End: ELF Generation") {
    
    TEST_CASE("Generate ELF for Simple Function") {
        ensure_test_output_dir();
        
        std::string source = R"(func test():
    return 42
)";
        auto result = compileGDScript(source);
        
        REQUIRE(result.success);
        
        // Generate ELF
        ELFGenerator elf_gen;
        elf_gen.add_code_section(result.get_code_ptr(), result.codeSize, ".text");
        auto elf = elf_gen.generate();
        
        REQUIRE(!elf.empty());
        
        // Write to file for inspection
        std::ofstream file("test_output/test_feature.elf", std::ios::binary);
        REQUIRE(file.is_open());
        file.write(reinterpret_cast<const char*>(elf.data()), elf.size());
        file.close();
        
        // Verify ELF size is reasonable
        CHECK(elf.size() > 200); // ELF header + program header + code + section headers
    }
}

