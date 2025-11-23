#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace gdscript {

// Forward declarations
class ASTNode;
class ProgramNode;
class FunctionNode;
class StatementNode;
class ExpressionNode;

// Base class for all AST nodes
class ASTNode {
public:
    enum class NodeType {
        Program,
        Function,
        ReturnStatement,
        IfStatement,
        ForStatement,
        WhileStatement,
        VariableDeclaration,
        AssignmentStatement,
        ExpressionStatement,
        BinaryOpExpr,
        UnaryOpExpr,
        CallExpr,
        IdentifierExpr,
        LiteralExpr,
        ArrayLiteralExpr,
        DictionaryLiteralExpr,
        MemberAccessExpr,
        SubscriptExpr,
        TypeCastExpr,
        TernaryExpr
    };

    virtual ~ASTNode() = default;
    virtual NodeType getType() const = 0;
    
    // Source location for error reporting
    int line = 0;
    int column = 0;
};

// Root node containing the entire program
class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<FunctionNode>> functions;
    std::vector<std::unique_ptr<StatementNode>> statements;
    
    NodeType getType() const override { return NodeType::Program; }
};

// Function definition
class FunctionNode : public ASTNode {
public:
    std::string name;
    std::vector<std::pair<std::string, std::string>> parameters; // (name, type_hint)
    std::string returnType; // Optional return type hint
    std::vector<std::unique_ptr<StatementNode>> body;
    bool isStatic = false;
    std::string rpcAnnotation; // "remote", "master", "puppet", etc.
    
    NodeType getType() const override { return NodeType::Function; }
};

// Base class for statements
class StatementNode : public ASTNode {
public:
    virtual ~StatementNode() = default;
};

// Return statement
class ReturnStatement : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> value; // nullptr if no return value
    
    NodeType getType() const override { return NodeType::ReturnStatement; }
};

// If/elif/else statement
class IfStatement : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::vector<std::unique_ptr<StatementNode>> thenBody;
    std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::vector<std::unique_ptr<StatementNode>>>> elifBranches;
    std::vector<std::unique_ptr<StatementNode>> elseBody;
    
    NodeType getType() const override { return NodeType::IfStatement; }
};

// For loop: for identifier in expression
class ForStatement : public StatementNode {
public:
    std::string variableName;
    std::unique_ptr<ExpressionNode> iterable;
    std::vector<std::unique_ptr<StatementNode>> body;
    
    NodeType getType() const override { return NodeType::ForStatement; }
};

// While loop
class WhileStatement : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::vector<std::unique_ptr<StatementNode>> body;
    
    NodeType getType() const override { return NodeType::WhileStatement; }
};

// Variable declaration: var name [= expression]
class VariableDeclaration : public StatementNode {
public:
    std::string name;
    std::string typeHint; // Optional type hint
    std::unique_ptr<ExpressionNode> initializer; // nullptr if no initializer
    
    NodeType getType() const override { return NodeType::VariableDeclaration; }
};

// Assignment statement: target = expression (or +=, -=, etc.)
class AssignmentStatement : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> target;
    std::string op; // "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^="
    std::unique_ptr<ExpressionNode> value;
    
    NodeType getType() const override { return NodeType::AssignmentStatement; }
};

// Expression statement (expression followed by newline/semicolon)
class ExpressionStatement : public StatementNode {
public:
    std::unique_ptr<ExpressionNode> expression;
    
    NodeType getType() const override { return NodeType::ExpressionStatement; }
};

// Base class for expressions
class ExpressionNode : public ASTNode {
public:
    virtual ~ExpressionNode() = default;
};

// Binary operation: left op right
class BinaryOpExpr : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> left;
    std::string op; // "+", "-", "*", "/", "%", "==", "!=", "<", ">", "<=", ">=", "and", "or", "in", etc.
    std::unique_ptr<ExpressionNode> right;
    
    NodeType getType() const override { return NodeType::BinaryOpExpr; }
};

// Unary operation: op expr
class UnaryOpExpr : public ExpressionNode {
public:
    std::string op; // "-", "+", "!", "not", "~"
    std::unique_ptr<ExpressionNode> operand;
    
    NodeType getType() const override { return NodeType::UnaryOpExpr; }
};

// Function call: func(args) or obj.method(args)
class CallExpr : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> callee; // Function name or object.method
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    
    NodeType getType() const override { return NodeType::CallExpr; }
};

// Identifier (variable or function name)
class IdentifierExpr : public ExpressionNode {
public:
    std::string name;
    
    NodeType getType() const override { return NodeType::IdentifierExpr; }
};

// Literal value
class LiteralExpr : public ExpressionNode {
public:
    // Use variant to store different literal types
    std::variant<int64_t, double, std::string, bool, std::nullptr_t> value;
    
    NodeType getType() const override { return NodeType::LiteralExpr; }
};

// Array literal: [expr, expr, ...]
class ArrayLiteralExpr : public ExpressionNode {
public:
    std::vector<std::unique_ptr<ExpressionNode>> elements;
    
    NodeType getType() const override { return NodeType::ArrayLiteralExpr; }
};

// Dictionary literal: {key: value, ...} or {key = value, ...}
class DictionaryLiteralExpr : public ExpressionNode {
public:
    std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<ExpressionNode>>> entries;
    
    NodeType getType() const override { return NodeType::DictionaryLiteralExpr; }
};

// Member access: obj.member
class MemberAccessExpr : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> object;
    std::string member;
    
    NodeType getType() const override { return NodeType::MemberAccessExpr; }
};

// Subscript access: array[index]
class SubscriptExpr : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> object;
    std::unique_ptr<ExpressionNode> index;
    
    NodeType getType() const override { return NodeType::SubscriptExpr; }
};

// Type cast: expr as Type
class TypeCastExpr : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> expression;
    std::string targetType;
    
    NodeType getType() const override { return NodeType::TypeCastExpr; }
};

// Ternary expression: condition if true_expr else false_expr
class TernaryExpr : public ExpressionNode {
public:
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<ExpressionNode> trueExpr;
    std::unique_ptr<ExpressionNode> falseExpr;
    
    NodeType getType() const override { return NodeType::TernaryExpr; }
};

} // namespace gdscript

