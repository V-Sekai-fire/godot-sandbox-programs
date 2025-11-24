# RFC 0007: Biscuit Library for RISC-V Code Generation

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to use the **biscuit** library for generating RISC-V machine code directly from AST, following the BEAM JIT pattern of using AsmJit for native code generation.

## Motivation

We need to generate RISC-V 64 Linux machine code from AST. Several approaches were considered:

1. **Text Assembly**: Generate assembly text, then assemble (slow, two-step process)
2. **MLIR/StableHLO**: Use intermediate representation (rejected - too complex, not needed)
3. **Direct Machine Code**: Generate machine code directly (like BEAM JIT with AsmJit)

**Decision**: Use **biscuit** library (similar to AsmJit) for direct machine code generation.

## Detailed Design

### Biscuit Library

**Location**: `ext/biscuit/` (git subrepo)

**Characteristics**:
- Header-only library (no linking needed)
- MIT license
- Generates RISC-V machine code directly
- Similar API to AsmJit (used by BEAM JIT)
- Supports RISC-V 64-bit instructions

### Usage in Codebase

**File**: `ast_to_riscv_biscuit.h/cpp`

**Architecture**:
```cpp
class ASTToRISCVEmitter {
    std::unique_ptr<biscuit::Assembler> _assembler;
    std::vector<uint8_t> _code_buffer;
    
    // Emit methods use biscuit API
    void _emit_function(...) {
        _assembler->ADDI(...);
        _assembler->LI(...);
        // etc.
    }
};
```

**Code Generation Flow**:
1. Create `biscuit::Assembler` instance
2. Emit instructions using assembler API
3. Get generated machine code from buffer
4. Use in ELF generation

## Benefits

1. **Direct Code Generation**: No assembly text step, generates machine code directly
2. **Fast**: Single-step process (AST → Machine Code)
3. **Similar to BEAM JIT**: Follows proven pattern (AsmJit in BEAM JIT)
4. **Header-Only**: No linking complexity
5. **RISC-V Native**: Designed specifically for RISC-V

## Implementation Status

✅ **Completed**:
- Biscuit integration
- Direct machine code generation
- RISC-V 64 Linux ABI compliance
- Stack frame management
- Register allocation

## Comparison

| Aspect | Text Assembly | MLIR | Biscuit (Direct) |
|--------|--------------|------|------------------|
| **Steps** | 2 (generate + assemble) | 3+ (IR passes) | 1 (direct) |
| **Speed** | Slower | Slowest | Fastest |
| **Complexity** | Medium | High | Low |
| **Dependencies** | Assembler tool | MLIR | Header-only |
| **Pattern** | Traditional | Modern | BEAM JIT |

## Alternatives Considered

### Alternative 1: Generate Assembly Text
- **Rejected**: Two-step process, slower, requires external assembler

### Alternative 2: Use MLIR/StableHLO
- **Rejected**: Too complex, unnecessary abstraction, removed from codebase

### Alternative 3: Use Different Codegen Library
- **Rejected**: Biscuit is RISC-V native, header-only, similar to AsmJit

## Unresolved Questions

- [ ] Should we support RISC-V 32-bit?
- [ ] How to handle RISC-V extensions (vector, crypto, etc.)?
- [ ] Should we add instruction-level optimizations?

## References

- [RFC 0003: BeamAsm Template Approach](./0003-beamasm-template-approach.md)
- [RFC 0001: Migration Strategy](./0001-migration-strategy.md)

