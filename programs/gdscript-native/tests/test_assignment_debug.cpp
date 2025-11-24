#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include "../src/ast_interpreter.h"
#include <string>
#include <memory>

using namespace gdscript;

TEST_CASE("Debug assignment parsing") {
    std::string source = R"(func test():
    var x = 10
    x = 20
    return x
)";
    GDScriptParser parser;
    auto ast = parser.parse(source);
    
    REQUIRE(ast != nullptr);
    REQUIRE(ast->functions.size() == 1);
    
    const FunctionNode* func = ast->functions[0].get();
    MESSAGE("Function has ", func->body.size(), " statements");
    
    for (size_t i = 0; i < func->body.size(); ++i) {
        const StatementNode* stmt = func->body[i].get();
        MESSAGE("Statement ", i, " type: ", static_cast<int>(stmt->get_type()));
        
        if (stmt->get_type() == ASTNode::NodeType::AssignmentStatement) {
            const AssignmentStatement* assign = static_cast<const AssignmentStatement*>(stmt);
            MESSAGE("Found assignment statement");
            if (assign->target && assign->target->get_type() == ASTNode::NodeType::IdentifierExpr) {
                const IdentifierExpr* target = static_cast<const IdentifierExpr*>(assign->target.get());
                MESSAGE("Assignment target: ", target->name);
            }
        }
    }
    
    ASTInterpreter interpreter;
    auto result = interpreter.execute(ast.get());
    
    MESSAGE("Result success: ", result.success);
    if (std::holds_alternative<int64_t>(result.return_value)) {
        MESSAGE("Return value: ", std::get<int64_t>(result.return_value));
    }
    
    CHECK(result.success);
    CHECK(std::get<int64_t>(result.return_value) == 20);
}

