#include "errors.h"
#include <sstream>

namespace gdscript {

void ErrorCollection::addError(const CompilationError& error) {
    if (maxErrors == 0 || errors.size() < maxErrors) {
        errors.push_back(error);
    }
}

void ErrorCollection::addError(ErrorType type, const std::string& message, 
                               const SourceLocation& location,
                               const std::string& context) {
    addError(CompilationError(type, message, location, context));
}

std::string ErrorCollection::getFormattedMessage() const {
    if (errors.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    
    for (size_t i = 0; i < errors.size(); ++i) {
        const auto& error = errors[i];
        
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
        
        if (i < errors.size() - 1) {
            oss << "\n";
        }
    }
    
    return oss.str();
}

std::string ErrorCollection::getFirstErrorMessage() const {
    if (errors.empty()) {
        return "";
    }
    
    return errors[0].message;
}

std::vector<CompilationError> ErrorCollection::getErrorsByType(ErrorType type) const {
    std::vector<CompilationError> result;
    for (const auto& error : errors) {
        if (error.type == type) {
            result.push_back(error);
        }
    }
    return result;
}

} // namespace gdscript

