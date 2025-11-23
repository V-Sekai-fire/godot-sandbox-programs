# RFC 0012: Godot C++ Naming Conventions

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to follow Godot engine's C++ naming conventions throughout the codebase, ensuring consistency with the Godot codebase and improving code readability.

## Motivation

The codebase is part of the Godot sandbox ecosystem and should follow Godot's coding standards:

- **Consistency**: Matches Godot engine code style
- **Readability**: Godot developers familiar with conventions
- **Integration**: Easier to integrate with Godot codebase
- **Maintainability**: Standard conventions reduce cognitive load

## Detailed Design

### Naming Conventions

**Methods**: `snake_case`
- ✅ `get_type()`, `emit_function()`, `parse_statement()`
- ❌ `getType()`, `emitFunction()`, `parseStatement()`

**Private Members**: `_` prefix
- ✅ `_code_buffer`, `_stack_offset`, `_current_function`
- ❌ `codeBuffer`, `m_codeBuffer`, `code_buffer_`

**Private Methods**: `_` prefix
- ✅ `_allocate_register()`, `_emit_return()`, `_get_var_stack_offset()`
- ❌ `allocateRegister()`, `emitReturn()`, `getVarStackOffset()`

**Public Methods**: No prefix
- ✅ `emit()`, `clear()`, `get_code()`
- ❌ `_emit()`, `Emit()`, `emit_()`

**Classes**: `PascalCase`
- ✅ `ASTToRISCVEmitter`, `GDScriptParser`, `ELFGenerator`
- ❌ `ast_to_riscv_emitter`, `gdscript_parser`

**Namespaces**: `snake_case`
- ✅ `namespace gdscript { ... }`
- ❌ `namespace GDScript { ... }`

### Implementation

**Refactoring Applied**:
- All methods converted to `snake_case`
- All private members prefixed with `_`
- All private methods prefixed with `_`
- Consistent throughout entire codebase

## Benefits

1. **Consistency**: Matches Godot engine code style
2. **Readability**: Clear distinction between public/private
3. **Integration**: Easier to integrate with Godot codebase
4. **Familiarity**: Godot developers recognize conventions
5. **Maintainability**: Standard conventions reduce confusion

## Implementation Status

✅ **Completed**:
- Entire codebase refactored
- All methods use `snake_case`
- All private members use `_` prefix
- All private methods use `_` prefix
- Consistent with Godot engine style

## Comparison

| Aspect | camelCase | snake_case (Godot) |
|--------|-----------|-------------------|
| **Consistency** | Mixed | Consistent |
| **Godot Alignment** | No | Yes |
| **Private Visibility** | No prefix | `_` prefix |
| **Readability** | Mixed | Clear |

## Alternatives Considered

### Alternative 1: Keep camelCase
- **Rejected**: Doesn't match Godot conventions

### Alternative 2: Use Different Convention
- **Rejected**: Want consistency with Godot ecosystem

### Alternative 3: Mixed Conventions
- **Rejected**: Inconsistent, confusing

## Unresolved Questions

- [ ] Should we add clang-format config matching Godot?
- [ ] Should we add naming convention linter?
- [ ] How to handle new code to ensure compliance?

## References

- Godot Engine Coding Standards

