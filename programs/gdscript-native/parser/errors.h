#pragma once

#include <string>
#include <vector>
#include <memory>

namespace gdscript {

// Source location for error reporting
struct SourceLocation {
    int line;
    int column;
    
    SourceLocation() : line(0), column(0) {}
    SourceLocation(int l, int c) : line(l), column(c) {}
};

// Error types
enum class ErrorType {
    Parse,      // Syntax/parsing errors
    Semantic,   // Type checking, undefined variables, etc.
    Codegen     // Code generation errors
};

// Compilation error with source location and context
class CompilationError {
public:
    ErrorType type;
    std::string message;
    SourceLocation location;
    std::string context; // Source code snippet around error
    
    CompilationError(ErrorType t, const std::string& msg, 
                    const SourceLocation& loc = SourceLocation(),
                    const std::string& ctx = "")
        : type(t), message(msg), location(loc), context(ctx) {}
};

// Error collection - supports multiple errors per compilation
class ErrorCollection {
private:
    std::vector<CompilationError> errors;
    size_t maxErrors; // Maximum errors to collect (0 = unlimited)
    
public:
    ErrorCollection(size_t max = 100) : maxErrors(max) {}
    
    // Add an error
    void addError(const CompilationError& error);
    void addError(ErrorType type, const std::string& message, 
                 const SourceLocation& location = SourceLocation(),
                 const std::string& context = "");
    
    // Get all errors
    const std::vector<CompilationError>& getErrors() const { return errors; }
    
    // Check if there are any errors
    bool hasErrors() const { return !errors.empty(); }
    
    // Get error count
    size_t getErrorCount() const { return errors.size(); }
    
    // Get formatted error message (all errors)
    std::string getFormattedMessage() const;
    
    // Get first error message (for compatibility)
    std::string getFirstErrorMessage() const;
    
    // Clear all errors
    void clear() { errors.clear(); }
    
    // Get errors by type
    std::vector<CompilationError> getErrorsByType(ErrorType type) const;
};

} // namespace gdscript

