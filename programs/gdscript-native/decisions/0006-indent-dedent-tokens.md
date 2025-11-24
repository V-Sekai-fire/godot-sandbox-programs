# RFC 0006: INDENT/DEDENT Token Approach for Indentation Handling

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to use `INDENT` and `DEDENT` tokens for handling GDScript's significant whitespace, following the approach used by Godot's official GDScript parser.

## Motivation

GDScript uses indentation for block structure (like Python), not braces. This requires special handling in the parser:

- **Problem**: How to represent indentation in token stream?
- **Solution**: Use `INDENT` and `DEDENT` tokens (like Python parsers and Godot's parser)

This approach:
- Eliminates need for `NEWLINE` tokens in grammar
- Makes block structure explicit in token stream
- Aligns with Godot's official parser design
- Simplifies recursive descent parsing

## Detailed Design

### Tokenizer Implementation

**File**: `parser/gdscript_tokenizer.cpp`

**Indentation Tracking**:
```cpp
std::vector<int> _indent_stack;  // Stack of indentation levels
int _pending_indents = 0;        // Number of INDENT tokens to emit
```

**Algorithm**:
1. Track current indentation level
2. Compare with previous line's indentation
3. Emit `INDENT` tokens when indentation increases
4. Emit `DEDENT` tokens when indentation decreases
5. Maintain indentation stack for proper nesting

**Token Emission**:
- `INDENT` - Indentation increased (new block started)
- `DEDENT` - Indentation decreased (block ended)
- Multiple `DEDENT` tokens for multiple levels of dedentation

### Parser Usage

**File**: `parser/gdscript_parser.cpp`

**Grammar Simplification**:
- No need for `NEWLINE` tokens in grammar rules
- `INDENT`/`DEDENT` tokens make block structure explicit
- Direct mapping to AST block structures

**Example**:
```cpp
// Function body parsing
if (consume(TokenType::INDENT)) {
    // Parse statements in block
    while (!check(TokenType::DEDENT)) {
        parse_statement();
    }
    consume(TokenType::DEDENT);
}
```

## Benefits

1. **Explicit Block Structure**: Indentation becomes explicit tokens
2. **Simpler Grammar**: No need to handle newlines and indentation in grammar rules
3. **Alignment with Godot**: Matches official parser design
4. **Error Recovery**: Easier to detect and report indentation errors
5. **Consistency**: Same approach as Python parsers (proven pattern)

## Implementation Status

✅ **Completed**:
- Indentation tracking in tokenizer
- `INDENT`/`DEDENT` token emission
- Parser integration
- Error handling for indentation mismatches

## Comparison

| Aspect | Newline-Based | INDENT/DEDENT Tokens |
|--------|---------------|---------------------|
| **Grammar Complexity** | Complex (handle newlines) | Simple (explicit tokens) |
| **Error Messages** | Hard to pinpoint | Easy (token location) |
| **Block Detection** | Implicit | Explicit |
| **Alignment** | Custom | Matches Godot/Python |

## Alternatives Considered

### Alternative 1: Preprocess Indentation to Braces
- **Rejected**: Changes source structure, harder to map errors back

### Alternative 2: Handle in Grammar Rules
- **Rejected**: Too complex, fragile error handling

### Alternative 3: Use Newline Tokens
- **Rejected**: Doesn't capture indentation structure clearly

## Unresolved Questions

- [ ] How to handle tab vs space mixing?
- [ ] Should we normalize indentation?
- [ ] How to handle indentation errors gracefully?

## References

- [RFC 0005: Recursive Descent Parser](./0005-recursive-descent-parser.md)

