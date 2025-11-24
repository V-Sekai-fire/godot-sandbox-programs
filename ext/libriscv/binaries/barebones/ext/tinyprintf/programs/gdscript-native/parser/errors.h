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
    std::vector<CompilationError> _errors;
    size_t _max_errors; // Maximum errors to collect (0 = unlimited)
    
public:
    ErrorCollection(size_t max = 100) : _max_errors(max) {}
    
    // Add an error
    void add_error(const CompilationError& error);
    void add_error(ErrorType type, const std::string& message, 
                 const SourceLocation& location = SourceLocation(),
                 const std::string& context = "");
    
    // Get all errors
    const std::vector<CompilationError>& get_errors() const { return _errors; }
    
    // Check if there are any errors
    bool has_errors() const { return !_errors.empty(); }
    
    // Get error count
    size_t get_error_count() const { return _errors.size(); }
    
    // Get formatted error message (all errors)
    std::string get_formatted_message() const;
    
    // Get first error message (for compatibility)
    std::string get_first_error_message() const;
    
    // Clear all errors
    void clear() { _errors.clear(); }
    
    // Get errors by type
    std::vector<CompilationError> get_errors_by_type(ErrorType type) const;
};

} // namespace gdscript

