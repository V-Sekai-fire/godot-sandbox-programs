# RFC 0005: Recursive Descent Parser Implementation

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to use a hand-written recursive descent parser instead of a PEG parser generator (cpp-peglib) for parsing GDScript source code. This decision was made after encountering limitations with the PEG parser approach.

## Motivation

The initial implementation used cpp-peglib (PEG parser generator), but several issues were encountered:

1. **Semantic Value Storage Issues**: cpp-peglib doesn't store the Program rule's semantic value in the parse result, requiring workarounds
2. **Fragile Type Conversions**: Complex `std::any` conversions between `shared_ptr` (semantic actions) and `unique_ptr` (final AST)
3. **Limited Error Reporting**: Difficult to provide good error messages with source locations
4. **Complex Grammar Rules**: Workarounds needed for Program rule and indentation handling

## Detailed Design

### Implementation

**Files**: `parser/gdscript_tokenizer.h/cpp`, `parser/gdscript_parser.h/cpp`

**Architecture**:
1. **Tokenizer**: Converts source code into tokens with source location tracking
2. **Parser**: Recursive descent parser that directly constructs AST nodes
3. **Error Handling**: Structured error system with `ErrorCollection` and `SourceLocation`

### Tokenizer Design

**Indentation Handling**:
- Uses `INDENT` and `DEDENT` tokens (similar to Godot's official parser)
- Tracks indentation stack to emit proper `INDENT`/`DEDENT` tokens
- Eliminates need for `NEWLINE` tokens in grammar

**Token Types**:
- `TokenType` enum class for type safety
- `token_type_name()` helper for debugging/display
- Source location tracking (line, column)

### Parser Design

**Recursive Descent Methods**:
- `parse_program()` - Entry point
- `parse_function()` - Function definitions
- `parse_statement()` - Statements
- `parse_expression()` - Expressions
- Each method directly constructs AST nodes (no `std::any` conversions)

**Error Handling**:
- `ErrorCollection` class for multiple errors per compilation
- `SourceLocation` struct for line/column tracking
- Formatted error messages with source context
- Continues parsing after errors to find more issues

## Benefits

1. **Better Error Messages**: Direct control over error reporting with source locations
2. **Direct AST Construction**: No `std::any` conversions, build AST nodes directly
3. **Easier Debugging**: Clear parsing logic, easy to step through in debugger
4. **Full Control**: Complete control over parsing behavior and error recovery
5. **Simpler**: No external parser generator, just C++ code
6. **Alignment**: Matches Godot's official parser design (uses `INDENT`/`DEDENT`)

## Implementation Status

✅ **Completed**:
- Tokenizer with `INDENT`/`DEDENT` support
- Recursive descent parser
- Error collection and reporting
- Source location tracking
- Direct AST construction

## Comparison

| Aspect | PEG Parser (cpp-peglib) | Recursive Descent |
|--------|------------------------|-------------------|
| **Error Messages** | Limited | Full control |
| **Type Safety** | `std::any` conversions | Direct AST construction |
| **Debugging** | Hard (generated code) | Easy (hand-written) |
| **Indentation** | Workarounds needed | Native `INDENT`/`DEDENT` |
| **Dependencies** | External library | Pure C++ |
| **Complexity** | Grammar + workarounds | Straightforward code |

## Alternatives Considered

### Alternative 1: Fix PEG Parser Issues
- **Rejected**: Too many workarounds, fundamental limitations

### Alternative 2: Use Different PEG Library
- **Rejected**: Same fundamental issues, prefer hand-written control

### Alternative 3: Use Godot's Parser Directly
- **Rejected**: Want independence, different target (RISC-V vs GDScript VM)

## Unresolved Questions

- [ ] Should we add more sophisticated error recovery?
- [ ] How to handle incremental parsing for IDE support?
- [ ] Should we add parse tree caching?

## References

- [RFC 0001: Migration Strategy](./0001-migration-strategy.md)
- [RFC 0002: Testing Environments](./0002-testing-environments.md)

