# RFC 0003: BeamAsm-Style Template Approach for RISC-V Code Generation

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC defines the approach for RISC-V code generation using native code templates, following the BeamAsm pattern. Instead of BEAM instructions, we interpret AST nodes and use native code templates similar to how BeamAsm has templates for each BEAM instruction.

## Motivation

BeamAsm (BEAM JIT) uses **AsmJit** to generate native code directly at load time, translating BEAM instructions to native code templates. We want to follow the same pattern:

- **BeamAsm**: `BEAM Instruction → Code Template Function → Native Code (AsmJit)`
- **Our Approach**: `AST Node → Code Template Function → RISC-V Machine Code (biscuit)`

This provides:
- Consistency in code generation
- Maintainability (easy to modify instruction patterns)
- Testability (templates can be tested independently)
- Proven architecture pattern (BeamAsm is production-ready)

## Detailed Design

### Code Templates (Like BeamAsm's `instr_*.cpp`)

Each AST node type has a corresponding template function in `riscv_code_templates.h/cpp`:

**Function Templates**:
- `emit_function_prologue()` - Function entry (like BeamAsm's function setup)
- `emit_function_epilogue()` - Function return (like BeamAsm's return instruction)

**Expression Templates**:
- `emit_load_immediate()` - Load constant (like BeamAsm's literal handling)
- `emit_load_from_stack()` - Load variable (like BeamAsm's register access)
- `emit_store_to_stack()` - Store variable

**Operation Templates**:
- `emit_add()`, `emit_sub()`, `emit_mul()`, `emit_div()`, `emit_mod()` - Binary operations
- `emit_eq()`, `emit_ne()`, `emit_lt()`, `emit_gt()`, `emit_le()`, `emit_ge()` - Comparisons

**Control Flow Templates**:
- `emit_branch_if_zero()`, `emit_jump()` - Control flow

### Usage in Emitter

The `ASTToRISCVEmitter` uses templates instead of direct `_assembler->` calls:

**Before (direct)**:
```cpp
_assembler->LI(reg, 42);
_assembler->ADD(result, left, right);
```

**After (template-based, like BeamAsm)**:
```cpp
RISCVCodeTemplates::emit_load_immediate(_assembler.get(), reg, 42);
RISCVCodeTemplates::emit_add(_assembler.get(), result, left, right);
```

### Template Structure

Each template function:
- Takes `biscuit::Assembler*` as first parameter
- Takes operation-specific parameters
- Emits RISC-V instructions directly
- No return value (side effect: emits code)

**Example: Function Prologue Template**
```cpp
void RISCVCodeTemplates::emit_function_prologue(
    biscuit::Assembler* a, 
    int stack_size, 
    size_t num_params
) {
    // Allocate stack space
    a->ADDI(biscuit::sp, biscuit::sp, -stack_size);
    
    // Save registers
    a->SD(biscuit::ra, stack_size - 8, biscuit::sp);
    a->SD(biscuit::s0, stack_size - 16, biscuit::sp);
    
    // Set frame pointer
    a->ADDI(biscuit::s0, biscuit::sp, stack_size);
    
    // Store parameters...
}
```

## Implementation Status

✅ **Completed**:
- Created `riscv_code_templates.h/cpp`
- Migrated function prologue/epilogue to templates
- Migrated literal loading to templates
- Migrated variable load/store to templates
- Migrated binary operations to templates
- Migrated comparisons to templates
- Migrated control flow to templates

## Comparison with BeamAsm

| BeamAsm | Our Approach |
|---------|-------------|
| `instr_*.cpp` files | `riscv_code_templates.cpp` |
| BEAM instructions | AST node types |
| AsmJit | biscuit |
| Load-time codegen | Compile-time codegen |
| Shared code fragments | Template functions |

## Benefits

1. **Consistency** - All code generation goes through templates
2. **Maintainability** - Easy to modify instruction patterns
3. **Testability** - Templates can be tested independently
4. **Similarity to BeamAsm** - Proven architecture pattern

## Notes

- Templates are stateless (like BeamAsm's instruction functions)
- All state (registers, stack, labels) managed by emitter
- Templates are pure functions (no side effects except code emission)

## Alternatives Considered

### Alternative 1: Direct Assembler Calls
- **Rejected**: Less maintainable, harder to test, inconsistent

### Alternative 2: Macro-Based Templates
- **Rejected**: Less flexible, harder to debug

### Alternative 3: String-Based Code Generation
- **Rejected**: Type-unsafe, requires separate assembler pass

## Unresolved Questions

- [ ] Should templates support optimization hints?
- [ ] How to handle architecture-specific templates (RV32 vs RV64)?
- [ ] Should we add template composition (templates calling templates)?

## References

- [RFC 0001: Migration Strategy](./0001-migration-strategy.md)
- [RFC 0002: Testing Environments](./0002-testing-environments.md)
- [RFC 0004: Dual-Mode Architecture](./0004-dual-mode-architecture.md)

