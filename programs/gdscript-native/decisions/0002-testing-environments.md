# RFC 0002: Testing Environments - Host vs RISC-V

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC defines the testing environments for the compiler: AST interpreter can be tested on both the host machine (for fast development) and on godot-sandbox/RISC-V (for validation), while native code templates are tested on RISC-V 64 Linux via libriscv emulator.

## Motivation

The compiler has two execution modes that operate in fundamentally different environments:

1. **AST Interpreter**: Runs on host machine (wherever compiler is running)
2. **Native Code Templates**: Runs on RISC-V 64 Linux (emulated via libriscv)

Understanding these environments is crucial for:
- Fast development iteration
- Proper testing strategies
- Debugging approaches
- Performance considerations

## Detailed Design

### AST Interpreter

**Environments**: 
1. **Host Machine** (macOS, Linux, Windows) - for fast development
2. **Godot-Sandbox/RISC-V** - for validation and production testing

**Host Machine Testing**:
- Direct C++ execution
- No cross-compilation needed
- Fast iteration and debugging
- Can use standard debugging tools (gdb, lldb, etc.)

```cpp
// Test on host - immediate feedback
ASTInterpreter interpreter;
ExecutionResult result = interpreter.execute(ast.get());
CHECK(result.success);
CHECK(result.return_value == expected);
```

**Godot-Sandbox/RISC-V Testing**:
- Same interpreter code, runs in godot-sandbox environment
- Validates interpreter works correctly on target platform
- Ensures consistency across environments

```cpp
// Test in godot-sandbox (RISC-V environment)
ASTInterpreter interpreter;
ExecutionResult result = interpreter.execute(ast.get());
// Same code, different environment
```

**Advantages**:
- ✅ Fast development cycle (on host)
- ✅ Easy debugging (on host)
- ✅ Platform validation (on godot-sandbox)
- ✅ Can test immediately after implementation (on host)
- ✅ Ensures interpreter works on target platform (on godot-sandbox)

**Use Cases**:
- Initial feature development (host)
- Quick validation (host)
- Debugging AST correctness (host)
- Reference implementation (both)
- Production validation (godot-sandbox)

### Native Code Templates (RISC-V 64 Linux)

**Environment**: RISC-V 64 Linux (emulated via libriscv)
- Generates RISC-V machine code
- Creates ELF files
- Executes via libriscv emulator
- Cross-platform testing (host → RISC-V)

**Testing Example**:
```cpp
// Test on RISC-V via libriscv
ASTToRISCVEmitter emitter;
auto [code, size] = emitter.emit(ast.get());

ELFGenerator elf_gen;
auto elf_data = elf_gen.generate(code, size, entry_point);

// Execute via libriscv
Machine<RISCV64> machine{elf_data};
machine.setup_linux();
machine.simulate();
int64_t result = machine.return_value<int64_t>();

CHECK(result == expected);
```

**Advantages**:
- ✅ Tests actual RISC-V code generation
- ✅ Validates ELF generation
- ✅ Ensures cross-platform correctness
- ✅ Production-ready testing

**Use Cases**:
- Final validation
- Performance testing
- Production deployment
- Cross-platform verification

## Development Workflow

```
┌─────────────────────────────────────┐
│ 1. Implement in Interpreter (Host) │
│    - Fast iteration                 │
│    - Easy debugging                 │
│    - Immediate feedback             │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ 2. Validate Interpreter (Host)     │
│    - Test correctness              │
│    - Use as golden reference        │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ 3. Test Interpreter (Godot-Sandbox) │
│    - Validate on target platform    │
│    - Ensure consistency             │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ 4. Migrate to Templates             │
│    - Add template function           │
│    - Integrate into emitter          │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│ 5. Test Native Code (RISC-V)        │
│    - Generate ELF                    │
│    - Execute via libriscv            │
│    - Compare with interpreter        │
└─────────────────────────────────────┘
```

## Comparison

| Aspect | Interpreter (Host) | Interpreter (Godot-Sandbox) | Native Code (RISC-V) |
|--------|-------------------|----------------------------|---------------------|
| **Environment** | Host machine | Godot-Sandbox/RISC-V | RISC-V 64 Linux (libriscv) |
| **Execution** | Direct C++ | Direct C++ (in sandbox) | ELF + Emulator |
| **Speed** | Fast (no emulator) | Fast (no emulator) | Slower (emulator overhead) |
| **Debugging** | Easy (standard tools) | Moderate (sandbox tools) | Harder (emulator debugging) |
| **Iteration** | Fast | Moderate | Slower |
| **Validation** | AST correctness | Platform correctness | Code generation correctness |
| **Use Case** | Development | Validation | Production |

## Best Practices

1. **Always implement in interpreter first**
   - Use as reference implementation
   - Validate AST correctness
   - Fast development cycle

2. **Test interpreter thoroughly on host**
   - Comprehensive test suite
   - Edge cases
   - Complex scenarios

3. **Test interpreter on godot-sandbox**
   - Validate interpreter works on target platform
   - Ensure consistency between host and godot-sandbox
   - Production-ready validation

4. **Migrate to native code incrementally**
   - One feature at a time
   - Test each migration
   - Compare with interpreter

5. **Use interpreter as golden reference**
   - If native code differs, fix native code
   - Interpreter is source of truth
   - Ensures correctness

6. **Test native code on RISC-V**
   - Always test via libriscv
   - Validate ELF generation
   - Ensure cross-platform correctness

## Example: Adding a New Feature

### Step 1: Interpreter (Host)
```cpp
// ast_interpreter.cpp
Value _evaluate_new_feature(const NewFeatureExpr* expr, Frame& frame) {
    // Implement on host - fast iteration
    // Test immediately
    return Value{...};
}
```

### Step 2: Test Interpreter (Host)
```cpp
// test_interpreter.cpp (runs on host)
TEST_CASE("New feature") {
    ASTInterpreter interpreter;
    auto result = interpreter.execute(ast.get());
    CHECK(result.return_value == expected);
}
```

### Step 3: Test Interpreter (Godot-Sandbox)
```cpp
// test_interpreter_sandbox.cpp (runs in godot-sandbox)
TEST_CASE("New feature in sandbox") {
    ASTInterpreter interpreter;
    auto result = interpreter.execute(ast.get());
    CHECK(result.return_value == expected);
    // Validates interpreter works on target platform
}
```

### Step 4: Native Code Template (RISC-V)
```cpp
// riscv_code_templates.cpp
void emit_new_feature(biscuit::Assembler* a, ...) {
    // Generate RISC-V code
}
```

### Step 5: Test Native Code (RISC-V)
```cpp
// test_native_code.cpp (runs on host, tests RISC-V)
TEST_CASE("New feature native code") {
    ASTToRISCVEmitter emitter;
    auto [code, size] = emitter.emit(ast.get());
    
    // Generate ELF and test via libriscv
    Machine<RISCV64> machine{elf_data};
    machine.simulate();
    CHECK(machine.return_value<int64_t>() == expected);
}
```

## Alternatives Considered

### Alternative 1: Test Native Code on Host Only
- **Rejected**: Doesn't validate actual RISC-V code generation

### Alternative 2: Use QEMU Instead of libriscv
- **Rejected**: libriscv is already integrated, simpler to use

### Alternative 3: Skip Interpreter, Go Direct to Native Code
- **Rejected**: Too slow for development, hard to debug

## Unresolved Questions

- [ ] Should we support testing native code on actual RISC-V hardware?
- [ ] How to handle performance profiling across environments?
- [ ] Should we add host-native code generation for faster testing?

## References

- [RFC 0001: Migration Strategy](./0001-migration-strategy.md)
- [RFC 0003: BeamAsm Template Approach](./0003-beamasm-template-approach.md)
- [RFC 0004: Dual-Mode Architecture](./0004-dual-mode-architecture.md)

