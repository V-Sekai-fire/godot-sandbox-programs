# RFC 0001: Migration Strategy - AST Interpreter to Native Code Templates

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC defines the strategy for migrating from AST interpretation to native code template generation. The approach prioritizes getting a fully working AST interpreter on the host machine first, then gradually migrating features to RISC-V native code templates tested via libriscv.

## Motivation

- **Fast Development**: AST interpreter runs on host machine, enabling rapid iteration without emulator overhead
- **Platform Validation**: AST interpreter also tested on godot-sandbox to ensure it works on target platform
- **Reference Implementation**: Interpreter serves as golden reference for validating native code correctness
- **Incremental Migration**: Gradual migration reduces risk and allows testing each feature independently
- **Different Execution Environments**: Interpreter (host/godot-sandbox) vs Native Code (RISC-V via libriscv) require different testing strategies

## Detailed Design

### Phase 1: Full AST Interpreter (Current)

**Goal**: Complete, working AST interpreter tested on both host machine and godot-sandbox

**Implementation**:
- Implement all AST node types in `ast_interpreter.cpp`
- Support all language features (expressions, statements, control flow)
- Test thoroughly on host machine (fast iteration)
- Test on godot-sandbox (platform validation)
- Use as reference implementation

**Status**: ✅ In Progress
- ✅ Literals, Identifiers, Binary Operations
- ✅ Function Calls (with frame management)
- ✅ Return, Variable Declaration, Assignment
- ✅ If/elif/else, While loops, For loops

### Phase 2: Gradual Migration to Templates

**Goal**: Migrate features one-by-one to native code templates

**Process**:
1. Implement feature in interpreter (host) - fast iteration
2. Create template function in `riscv_code_templates.cpp`
3. Integrate into `ast_to_riscv_biscuit.cpp`
4. Test via libriscv (RISC-V) - compare with interpreter

**Migration Order**:
1. Core features (assignment, if/else, loops)
2. Advanced features (function calls, arrays, dictionaries)
3. Optimizations (register allocation, etc.)

## Testing Strategy

### Interpreter Testing

**Host Machine** (for fast development):
```cpp
// Fast iteration on host machine
ASTInterpreter interpreter;
ExecutionResult result = interpreter.execute(ast.get());
CHECK(result.success);
CHECK(result.return_value == expected);
```

**Advantages**:
- No emulator overhead
- Fast iteration
- Easy debugging
- Immediate feedback

**Godot-Sandbox/RISC-V** (for validation):
```cpp
// Test interpreter in godot-sandbox environment
ASTInterpreter interpreter;
ExecutionResult result = interpreter.execute(ast.get());
// Validates interpreter works on target platform
```

**Advantages**:
- Validates interpreter on target platform
- Ensures consistency across environments
- Production-ready validation

### Native Code Testing (RISC-V via libriscv)

```cpp
// Test on RISC-V 64 Linux via libriscv
ASTToRISCVEmitter emitter;
auto [code, size] = emitter.emit(ast.get());

ELFGenerator elf_gen;
auto elf_data = elf_gen.generate(code, size, entry_point);

Machine<RISCV64> machine{elf_data};
machine.setup_linux();
machine.simulate();
int64_t result = machine.return_value<int64_t>();

CHECK(result == expected); // Compare with interpreter
```

**Advantages**:
- Validates actual RISC-V code generation
- Tests ELF generation
- Ensures cross-platform correctness

## Feature Completeness Matrix

| Feature | Interpreter | Templates | Status |
|---------|------------|-----------|--------|
| Literals | ✅ | ✅ | Complete |
| Identifiers | ✅ | ✅ | Complete |
| Binary Ops | ✅ | ✅ | Complete |
| Function Calls | ✅ | ⚠️ | Interpreter done, templates stubbed |
| Return | ✅ | ✅ | Complete |
| Var Decl | ✅ | ✅ | Complete |
| Assignment | ✅ | ⚠️ | Interpreter done, templates partial |
| If/Else | ✅ | ⚠️ | Interpreter done, templates partial |
| While Loop | ✅ | ❌ | Interpreter done, templates missing |
| For Loop | ✅ | ❌ | Interpreter done, templates missing |

## Default Mode

**Current**: `INTERPRET` mode (for development)

**Rationale**:
- Easier to debug and test
- Faster iteration during development
- Can validate AST correctness before codegen

**Future**: Switch to `NATIVE_CODE` mode when all features migrated

## Alternatives Considered

### Alternative 1: Direct Native Code Only
- **Rejected**: Too slow for development, hard to debug

### Alternative 2: Bytecode VM
- **Rejected**: Adds unnecessary complexity, AST interpreter is simpler

### Alternative 3: C Code Generation
- **Rejected**: We want direct RISC-V generation (like BeamAsm), not C compilation

## Unresolved Questions

- [ ] When to switch default mode from INTERPRET to NATIVE_CODE?
- [ ] Should we support hybrid mode (interpreter + native code)?
- [ ] How to handle performance-critical paths?

## References

- [RFC 0002: Testing Environments](./0002-testing-environments.md)
- [RFC 0003: BeamAsm Template Approach](./0003-beamasm-template-approach.md)
- [RFC 0004: Dual-Mode Architecture](./0004-dual-mode-architecture.md)

