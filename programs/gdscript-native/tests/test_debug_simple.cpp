#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "test_compiler_helpers.h"
#include <iomanip>
#include <sstream>

TEST_SUITE("Debug Simple Test") {
    TEST_CASE("Debug return 42") {
        std::string source = R"(func test():
    return 42
)";
        
        MESSAGE("Source:\n" << source);
        
        CompilationResult result = compileGDScript(source);
        
        MESSAGE("Success: " << result.success);
        MESSAGE("Code size: " << result.codeSize);
        MESSAGE("AST functions: " << (result.ast ? result.ast->functions.size() : 0));
        
        if (result.ast && !result.ast->functions.empty()) {
            const FunctionNode* func = result.ast->functions[0].get();
            MESSAGE("Function name: " << func->name);
            MESSAGE("Function body statements: " << func->body.size());
            
            if (!func->body.empty()) {
                const StatementNode* stmt = func->body[0].get();
                MESSAGE("First statement type: " << static_cast<int>(stmt->get_type()));
                
                if (stmt->get_type() == ASTNode::NodeType::ReturnStatement) {
                    const ReturnStatement* ret = static_cast<const ReturnStatement*>(stmt);
                    MESSAGE("Return has value: " << (ret->value != nullptr));
                    if (ret->value) {
                        MESSAGE("Return value type: " << static_cast<int>(ret->value->get_type()));
                        if (ret->value->get_type() == ASTNode::NodeType::LiteralExpr) {
                            const LiteralExpr* lit = static_cast<const LiteralExpr*>(ret->value.get());
                            if (std::holds_alternative<int64_t>(lit->value)) {
                                MESSAGE("Literal value: " << std::get<int64_t>(lit->value));
                            }
                        }
                    }
                }
            }
        }
        
        REQUIRE(result.success);
        REQUIRE(result.codeSize > 0);
        
        // Generate ELF to get entry point for diagnostics
        gdscript::ELFGenerator elf_gen;
        elf_gen.add_code_section(result.get_code_ptr(), result.codeSize, ".text");
        std::vector<uint8_t> elf_data = elf_gen.generate();
        
        if (!elf_data.empty()) {
            // Create machine temporarily to get entry point and PC diagnostics
            riscv::Machine<riscv::RISCV64> machine{elf_data, {.memory_max = 64ULL << 20}};
            riscv::address_type<riscv::RISCV64> entry_point = machine.memory.start_address();
            riscv::address_type<riscv::RISCV64> pc_after_construction = machine.cpu.pc();
            
            {
                std::ostringstream oss;
                oss << "Entry point: 0x" << std::hex << entry_point << std::dec;
                MESSAGE(oss.str());
            }
            {
                std::ostringstream oss;
                oss << "PC after construction: 0x" << std::hex << pc_after_construction << std::dec;
                MESSAGE(oss.str());
            }
            
            machine.setup_linux({"./program"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
            machine.setup_linux_syscalls();
            
            riscv::address_type<riscv::RISCV64> pc_after_setup = machine.cpu.pc();
            riscv::address_type<riscv::RISCV64> pc_before_simulate = machine.cpu.pc();
            
            {
                std::ostringstream oss;
                oss << "PC after setup_linux(): 0x" << std::hex << pc_after_setup << std::dec;
                MESSAGE(oss.str());
            }
            {
                std::ostringstream oss;
                oss << "PC before simulate(): 0x" << std::hex << pc_before_simulate << std::dec;
                MESSAGE(oss.str());
            }
        }
        
        // Execute and verify
        int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
        MESSAGE("Execution result: " << actual);
        CHECK(actual == 42);
    }
}

