#ifndef TEST_COMPILER_HELPERS_H
#define TEST_COMPILER_HELPERS_H

#include "../src/parser/gdscript_parser.h"
#include "../src/parser/ast.h"
#include "../src/ast_to_riscv_biscuit.h"
#include <string>
#include <memory>
#include <vector>
#include <cstring>
#include <sys/mman.h>
#include <variant>
#include <iostream>

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

static inline CompilationResult compileGDScript(const std::string& source) {
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
        // Add more context to error message
        if (result.errorMessage.empty()) {
            result.errorMessage = "Parse failed: AST is null";
        }
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
static inline int64_t execute_generated_code(const uint8_t* code, size_t size) {
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

// Helper to print variant value for debugging
static inline void print_variant(const std::variant<int64_t, double, std::string, bool, std::nullptr_t>& v, std::ostream& os) {
    if (std::holds_alternative<int64_t>(v)) {
        os << std::get<int64_t>(v);
    } else if (std::holds_alternative<double>(v)) {
        os << std::get<double>(v);
    } else if (std::holds_alternative<std::string>(v)) {
        os << "\"" << std::get<std::string>(v) << "\"";
    } else if (std::holds_alternative<bool>(v)) {
        os << (std::get<bool>(v) ? "true" : "false");
    } else if (std::holds_alternative<std::nullptr_t>(v)) {
        os << "null";
    }
}

// Helper to print AST for debugging (returns string for doctest MESSAGE)
static inline void print_ast(const ASTNode* node, int indent, std::ostream& os) {
    std::string indent_str(indent * 2, ' ');
    
    if (!node) {
        os << indent_str << "null\n";
        return;
    }
    
    switch (node->get_type()) {
        case ASTNode::NodeType::Program: {
            auto* prog = static_cast<const ProgramNode*>(node);
            os << indent_str << "Program\n";
            os << indent_str << "  Functions: " << prog->functions.size() << "\n";
            for (size_t i = 0; i < prog->functions.size(); ++i) {
                os << indent_str << "  Function[" << i << "]:\n";
                print_ast(prog->functions[i].get(), indent + 2, os);
            }
            os << indent_str << "  Statements: " << prog->statements.size() << "\n";
            for (size_t i = 0; i < prog->statements.size(); ++i) {
                os << indent_str << "  Statement[" << i << "]:\n";
                print_ast(prog->statements[i].get(), indent + 2, os);
            }
            break;
        }
        case ASTNode::NodeType::Function: {
            auto* func = static_cast<const FunctionNode*>(node);
            os << indent_str << "Function: " << func->name << "\n";
            os << indent_str << "  Return type: " << func->return_type << "\n";
            os << indent_str << "  Parameters: " << func->parameters.size() << "\n";
            for (size_t i = 0; i < func->parameters.size(); ++i) {
                os << indent_str << "    [" << i << "] " << func->parameters[i].first 
                   << " : " << func->parameters[i].second << "\n";
            }
            os << indent_str << "  Body statements: " << func->body.size() << "\n";
            for (size_t i = 0; i < func->body.size(); ++i) {
                os << indent_str << "    [" << i << "]:\n";
                print_ast(func->body[i].get(), indent + 3, os);
            }
            break;
        }
        case ASTNode::NodeType::ReturnStatement: {
            auto* ret = static_cast<const ReturnStatement*>(node);
            os << indent_str << "ReturnStatement\n";
            if (ret->value) {
                os << indent_str << "  Value:\n";
                print_ast(ret->value.get(), indent + 2, os);
            } else {
                os << indent_str << "  Value: null\n";
            }
            break;
        }
        case ASTNode::NodeType::LiteralExpr: {
            auto* lit = static_cast<const LiteralExpr*>(node);
            os << indent_str << "LiteralExpr: ";
            print_variant(lit->value, os);
            os << "\n";
            break;
        }
        case ASTNode::NodeType::IdentifierExpr: {
            auto* ident = static_cast<const IdentifierExpr*>(node);
            os << indent_str << "IdentifierExpr: " << ident->name << "\n";
            break;
        }
        case ASTNode::NodeType::BinaryOpExpr: {
            auto* binop = static_cast<const BinaryOpExpr*>(node);
            os << indent_str << "BinaryOpExpr: " << binop->op << "\n";
            os << indent_str << "  Left:\n";
            print_ast(binop->left.get(), indent + 2, os);
            os << indent_str << "  Right:\n";
            print_ast(binop->right.get(), indent + 2, os);
            break;
        }
        case ASTNode::NodeType::VariableDeclaration: {
            auto* var = static_cast<const VariableDeclaration*>(node);
            os << indent_str << "VariableDeclaration: " << var->name;
            if (!var->type_hint.empty()) {
                os << " : " << var->type_hint;
            }
            os << "\n";
            if (var->initializer) {
                os << indent_str << "  Initializer:\n";
                print_ast(var->initializer.get(), indent + 2, os);
            }
            break;
        }
        default:
            os << indent_str << "ASTNodeType: " << static_cast<int>(node->get_type()) << "\n";
            break;
    }
}

// Helper to print AST as string for doctest MESSAGE
static inline std::string ast_to_string(const ASTNode* node) {
    std::ostringstream oss;
    print_ast(node, 0, oss);
    return oss.str();
}

// Helper to print AST using doctest MESSAGE macro
static inline void print_ast_message(const ASTNode* node, const char* label = "AST") {
    if (node) {
        MESSAGE(label << ":\n" << ast_to_string(node));
    } else {
        MESSAGE(label << ": null");
    }
}

#endif // TEST_COMPILER_HELPERS_H

