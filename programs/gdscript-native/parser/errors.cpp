#include "errors.h"
#include <sstream>

namespace gdscript {

void ErrorCollection::add_error(const CompilationError& error) {
    if (_max_errors == 0 || _errors.size() < _max_errors) {
        _errors.push_back(error);
    }
}

void ErrorCollection::add_error(ErrorType type, const std::string& message, 
                               const SourceLocation& location,
                               const std::string& context) {
    add_error(CompilationError(type, message, location, context));
}

std::string ErrorCollection::get_formatted_message() const {
    if (_errors.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    
    for (size_t i = 0; i < _errors.size(); ++i) {
        const auto& error = _errors[i];
        
        // Error type prefix
        const char* typeStr = "";
        switch (error.type) {
            case ErrorType::Parse:
                typeStr = "Parse";
                break;
            case ErrorType::Semantic:
                typeStr = "Semantic";
                break;
            case ErrorType::Codegen:
                typeStr = "Codegen";
                break;
        }
        
        oss << "[" << typeStr << " Error]";
        
        // Location
        if (error.location.line > 0) {
            oss << " at line " << error.location.line;
            if (error.location.column > 0) {
                oss << ", column " << error.location.column;
            }
        }
        
        oss << ": " << error.message;
        
        // Context
        if (!error.context.empty()) {
            oss << "\n  Context: " << error.context;
        }
        
        if (i < _errors.size() - 1) {
            oss << "\n";
        }
    }
    
    return oss.str();
}

std::string ErrorCollection::get_first_error_message() const {
    if (_errors.empty()) {
        return "";
    }
    
    return _errors[0].message;
}

std::vector<CompilationError> ErrorCollection::get_errors_by_type(ErrorType type) const {
    std::vector<CompilationError> result;
    for (const auto& error : _errors) {
        if (error.type == type) {
            result.push_back(error);
        }
    }
    return result;
}

} // namespace gdscript

