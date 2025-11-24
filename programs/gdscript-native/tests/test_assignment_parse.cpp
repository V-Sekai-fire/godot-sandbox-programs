#include "doctest.h"
#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include <string>

using namespace gdscript;

TEST_CASE("Assignment parsing verification") {
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
    
    bool found_assignment = false;
    for (size_t i = 0; i < func->body.size(); ++i) {
        const StatementNode* stmt = func->body[i].get();
        MESSAGE("Statement ", i, " type: ", static_cast<int>(stmt->get_type()));
        
        if (stmt->get_type() == ASTNode::NodeType::AssignmentStatement) {
            found_assignment = true;
            const AssignmentStatement* assign = static_cast<const AssignmentStatement*>(stmt);
            if (assign->target && assign->target->get_type() == ASTNode::NodeType::IdentifierExpr) {
                const IdentifierExpr* target = static_cast<const IdentifierExpr*>(assign->target.get());
                MESSAGE("  Found assignment to: ", target->name);
            }
        } else if (stmt->get_type() == ASTNode::NodeType::ExpressionStatement) {
            MESSAGE("  Statement is ExpressionStatement (not AssignmentStatement)");
        }
    }
    
    CHECK(found_assignment);
}

