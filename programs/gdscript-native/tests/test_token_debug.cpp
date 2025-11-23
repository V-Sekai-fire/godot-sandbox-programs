#include "doctest.h"
#include "../src/parser/gdscript_tokenizer.h"
#include "../src/parser/gdscript_parser.h"
#include <string>

using namespace gdscript;

TEST_CASE("Debug tokenization of assignment") {
    std::string source = R"(func test():
    var x = 10
    x = 20
    return x
)";
    GDScriptTokenizer tokenizer;
    tokenizer.set_source(source);
    
    MESSAGE("Tokens:");
    for (int i = 0; i < 25; ++i) {
        Token t = tokenizer.scan();
        MESSAGE("  Token ", i, ": ", token_type_name(t.type));
        if (t.type == TokenType::IDENTIFIER || t.type == TokenType::LITERAL) {
            MESSAGE("    literal: ", t.literal);
        }
        if (t.type == TokenType::EOF_TOKEN) break;
        if (i >= 11 && t.type == TokenType::IDENTIFIER && t.literal == "x") {
            MESSAGE("    Found second 'x' - checking next tokens...");
            for (int j = 0; j < 5; ++j) {
                Token next = tokenizer.scan();
                MESSAGE("      Next token ", j, ": ", token_type_name(next.type));
                if (next.type == TokenType::IDENTIFIER || next.type == TokenType::LITERAL) {
                    MESSAGE("        literal: ", next.literal);
                }
                if (next.type == TokenType::EOF_TOKEN) break;
            }
            break;
        }
    }
}

TEST_CASE("Debug parser tokens during assignment parsing") {
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
    }
}

