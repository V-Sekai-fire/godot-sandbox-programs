# RFC 0009: Structured Error Handling System

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to implement a structured error handling system with error collection, source location tracking, and formatted error messages, rather than stopping at the first error.

## Motivation

Initial implementation had basic error handling that stopped at the first error. This was limiting because:

- **Single Error Limitation**: Could only report one error per compilation
- **Poor User Experience**: Users had to fix errors one at a time
- **No Source Context**: Errors lacked line/column information
- **No Error Types**: All errors treated the same

## Detailed Design

### Error Types

**File**: `parser/errors.h`

**Error Classification**:
```cpp
enum class ErrorType {
    Parse,      // Syntax errors
    Semantic,   // Type errors, undefined variables, etc.
    Codegen     // Code generation errors
};
```

### Error Structure

**CompilationError Class**:
```cpp
class CompilationError {
    ErrorType type;
    std::string message;
    SourceLocation location;  // line, column
    std::string context;      // Source code snippet
};
```

**SourceLocation**:
```cpp
struct SourceLocation {
    int line;
    int column;
};
```

### Error Collection

**ErrorCollection Class**:
- Collects multiple errors per compilation
- Formats error messages with source context
- Provides formatted output for user display
- Continues parsing after errors to find more issues

**Usage**:
```cpp
ErrorCollection errors;
errors.add_error(ErrorType::Parse, "Unexpected token", location);
// Continue parsing...
if (errors.has_errors()) {
    std::string msg = errors.get_formatted_message();
}
```

## Benefits

1. **Multiple Errors**: Report all errors in one compilation
2. **Better UX**: Users see all issues at once
3. **Source Context**: Line/column information for debugging
4. **Error Types**: Can categorize and handle errors differently
5. **Error Recovery**: Continue parsing to find more errors

## Implementation Status

✅ **Completed**:
- `CompilationError` class
- `ErrorCollection` class
- `SourceLocation` struct
- Formatted error messages
- Integration into parser

## Comparison

| Aspect | Single Error | Error Collection |
|--------|-------------|-----------------|
| **Errors Reported** | 1 | Multiple |
| **User Experience** | Fix one at a time | See all at once |
| **Source Context** | Limited | Full (line/column) |
| **Error Types** | None | Parse/Semantic/Codegen |
| **Error Recovery** | Stop immediately | Continue parsing |

## Alternatives Considered

### Alternative 1: Exception-Based Errors
- **Rejected**: Want to collect multiple errors, exceptions stop execution

### Alternative 2: Return Error Codes
- **Rejected**: Less structured, harder to provide context

### Alternative 3: Logging System
- **Rejected**: Need structured errors with types and locations

## Unresolved Questions

- [ ] Should we add error severity levels (error/warning/info)?
- [ ] How to handle error recovery in parser?
- [ ] Should we add error codes for programmatic handling?

## References

- [RFC 0005: Recursive Descent Parser](./0005-recursive-descent-parser.md)

