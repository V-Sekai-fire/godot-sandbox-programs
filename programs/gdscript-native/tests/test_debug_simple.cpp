#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "test_compiler_helpers.h"
#include <iostream>

TEST_SUITE("Debug Simple Test") {
    TEST_CASE("Debug return 42") {
        std::string source = R"(func test():
    return 42
)";
        
        std::cout << "Source:\n" << source << "\n";
        
        CompilationResult result = compileGDScript(source);
        
        std::cout << "Success: " << result.success << "\n";
        std::cout << "Code size: " << result.codeSize << "\n";
        std::cout << "AST functions: " << (result.ast ? result.ast->functions.size() : 0) << "\n";
        
        if (result.ast && !result.ast->functions.empty()) {
            const FunctionNode* func = result.ast->functions[0].get();
            std::cout << "Function name: " << func->name << "\n";
            std::cout << "Function body statements: " << func->body.size() << "\n";
            
            if (!func->body.empty()) {
                const StatementNode* stmt = func->body[0].get();
                std::cout << "First statement type: " << static_cast<int>(stmt->get_type()) << "\n";
                
                if (stmt->get_type() == ASTNode::NodeType::ReturnStatement) {
                    const ReturnStatement* ret = static_cast<const ReturnStatement*>(stmt);
                    std::cout << "Return has value: " << (ret->value != nullptr) << "\n";
                    if (ret->value) {
                        std::cout << "Return value type: " << static_cast<int>(ret->value->get_type()) << "\n";
                        if (ret->value->get_type() == ASTNode::NodeType::LiteralExpr) {
                            const LiteralExpr* lit = static_cast<const LiteralExpr*>(ret->value.get());
                            if (std::holds_alternative<int64_t>(lit->value)) {
                                std::cout << "Literal value: " << std::get<int64_t>(lit->value) << "\n";
                            }
                        }
                    }
                }
            }
        }
        
        if (result.success && result.codeSize > 0) {
            int64_t actual = execute_generated_code(result.get_code_ptr(), result.codeSize);
            std::cout << "Execution result: " << actual << "\n";
        }
        
        CHECK(result.success);
    }
}

