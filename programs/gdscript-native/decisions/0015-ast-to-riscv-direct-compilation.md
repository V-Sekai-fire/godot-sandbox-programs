# RFC 0015: Direct AST to RISC-V Compilation (Not Godot Bytecode)

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to use direct AST → RISC-V compilation, rather than using Godot's internal bytecode format as an intermediate representation.

## Motivation

We need to compile GDScript to RISC-V machine code. Two approaches were considered:

1. **GDScript → Godot Bytecode → RISC-V**: Use Godot's bytecode format
2. **GDScript → AST → RISC-V**: Direct compilation from AST

**Decision**: Use direct AST → RISC-V compilation.

## Detailed Design

### Architecture

```
GDScript Source Code
    ↓
[Parser] → AST (Abstract Syntax Tree)
    ↓
[Direct AST to RISC-V Emitter (biscuit)] → RISC-V 64 Machine Code
    ↓
[ELF Generator] → RISC-V 64 Linux ELF File
```

**Single Translation Step**: AST → RISC-V (no intermediate bytecode format)

## Rationale

1. **BEAM JIT Pattern**: BEAM JIT generates machine code directly from BEAM instructions. Our approach generates directly from AST, which is even simpler.

2. **Performance**: Native code generation eliminates VM overhead. No interpretation layer needed.

3. **Simplicity**: Single translation step (AST → RISC-V) is simpler than two steps (GDScript → Bytecode → RISC-V).

4. **Independence**: No dependency on Godot's internal bytecode format, which:
   - Contains non-serializable pointers
   - May change between Godot versions
   - Is designed for VM interpretation, not direct code generation
   - Is not publicly documented/stable

5. **Control**: Full control over code generation allows for optimizations and RISC-V-specific improvements.

## Alternatives Considered

### Alternative 1: Use Godot Bytecode Format

**Rejected** because:
- Requires reverse-engineering or depending on Godot internals
- Translating stack-based bytecode to register-based RISC-V is complex
- Version compatibility issues between Godot versions
- More complex architecture (two translation steps)
- Bytecode format not publicly documented/stable

## Benefits

1. **Simplicity**: Single translation step
2. **Independence**: No dependency on Godot internals
3. **Performance**: Direct native code generation
4. **Control**: Full control over code generation
5. **Maintainability**: Easier to understand and modify

## Implementation Status

✅ **Completed**:
- Direct AST to RISC-V emitter using biscuit
- No bytecode intermediate format
- Single-pass compilation

## References

- [RFC 0007: Biscuit RISC-V Codegen](./0007-biscuit-riscv-codegen.md)
- [RFC 0003: BeamAsm Template Approach](./0003-beamasm-template-approach.md)

