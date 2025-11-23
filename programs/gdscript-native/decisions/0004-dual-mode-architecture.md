# RFC 0004: Dual-Mode Architecture - AST Interpreter + Native Code Templates

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC defines the dual-mode architecture supporting both AST interpretation and native code template generation. The compiler can switch between interpreter mode (for development) and native code mode (for production), similar to how BEAM JIT has both interpreter and native code paths.

## Motivation

Following BEAM JIT's approach, we want both:
1. **AST Interpreter** - Executes AST directly (like a VM)
2. **Native Code Templates** - Generates RISC-V machine code using templates (like BeamAsm)

This provides:
- Fast development iteration (interpreter on host)
- Production performance (native code on RISC-V)
- Reference implementation (interpreter validates native code)
- Flexibility (can switch modes at runtime)

## Detailed Design

### Architecture

```
GDScript Source
    ↓
[Parser] → AST
    ↓
    ├─→ [AST Interpreter] → Direct Execution (Value) [Host]
    │
    └─→ [ASTToRISCVEmitter + Templates] → RISC-V Machine Code → ELF → Execution [RISC-V]
```

### Mode 1: AST Interpreter

**File**: `ast_interpreter.h/cpp`

**How it works**:
- Executes AST nodes directly without code generation
- Maintains execution frames (like a VM stack)
- Evaluates expressions and executes statements
- Returns `Value` (variant type) as result

**Benefits**:
- Simple and easy to debug
- No code generation complexity
- Good for testing and development
- Can step through execution easily

**Limitations**:
- Slower than native code
- No optimization opportunities
- Memory overhead for frame management

**Usage**:
```cpp
ASTInterpreter interpreter;
ExecutionResult result = interpreter.execute(ast.get());
// result.return_value contains the result
```

### Mode 2: Native Code Templates

**Files**: `ast_to_riscv_biscuit.h/cpp`, `riscv_code_templates.h/cpp`

**How it works**:
- Uses code templates (like BeamAsm's instruction templates)
- Generates RISC-V machine code directly
- Creates ELF files for execution
- Fast execution via native code

**Benefits**:
- Fast execution (native code)
- Optimization opportunities
- Similar to BeamAsm architecture
- Production-ready performance

**Limitations**:
- More complex implementation
- Harder to debug
- Requires ELF generation

**Usage**:
```cpp
ASTToRISCVEmitter emitter;
auto [code, size] = emitter.emit(ast.get());
// Generate ELF and execute
```

### Mode Switching

**API**: `set_compiler_mode(int mode)`
- `0` = INTERPRET mode
- `1` = NATIVE_CODE mode

**Default**: INTERPRET mode (for development)

**Example**:
```gdscript
# Switch to interpreter mode
set_compiler_mode(0)
var result = compile_gdscript("func test(): return 42")

# Switch to native code mode
set_compiler_mode(1)
var result = compile_gdscript("func test(): return 42")
```

## Comparison with BEAM JIT

| Feature | BEAM JIT | Our Approach |
|---------|----------|--------------|
| Interpreter | BEAM VM (bytecode) | AST Interpreter (AST nodes) |
| Native Code | BeamAsm (AsmJit) | RISC-V Templates (biscuit) |
| Mode Switch | Runtime decision | API-controlled |
| Code Templates | `instr_*.cpp` | `riscv_code_templates.cpp` |

## Implementation Details

### AST Interpreter

**Frame Structure**:
```cpp
struct Frame {
    std::unordered_map<std::string, Value> variables; // Local variables
    const FunctionNode* function; // Current function
    const ProgramNode* program; // Program context (for function calls)
    size_t pc; // Program counter (statement index)
};
```

**Value Type**:
```cpp
using Value = std::variant<int64_t, double, bool, std::nullptr_t, std::string>;
```

**Execution Flow**:
1. Create frame for function
2. Initialize parameters as local variables
3. Execute statements sequentially
4. Return value when `return` statement encountered
5. Handle control flow (if/else, loops)

### Native Code Templates

**Template Functions**:
- `emit_function_prologue()` - Function entry
- `emit_function_epilogue()` - Function return
- `emit_load_immediate()` - Load constant
- `emit_binary_op()` - Arithmetic/comparison operations
- `emit_branch_if_zero()` - Control flow

**Usage in Emitter**:
```cpp
// Instead of direct assembler calls:
_assembler->LI(reg, 42);

// Use templates:
RISCVCodeTemplates::emit_load_immediate(_assembler.get(), reg, 42);
```

## When to Use Each Mode

### Use Interpreter Mode When:
- Debugging parser/AST issues
- Testing new language features
- Need simple execution without ELF generation
- Development and prototyping

### Use Native Code Mode When:
- Production execution
- Performance is critical
- Need optimized code generation
- Final compilation pipeline

## Future Enhancements

1. **Hybrid Mode**: Start with interpreter, JIT compile hot paths
2. **Profile-Guided Optimization**: Use interpreter to profile, then optimize native code
3. **Debug Mode**: Native code with interpreter fallback for debugging
4. **Mode Auto-Switch**: Automatically choose mode based on code complexity

## Alternatives Considered

### Alternative 1: Interpreter Only
- **Rejected**: Too slow for production, no optimization opportunities

### Alternative 2: Native Code Only
- **Rejected**: Too slow for development, hard to debug

### Alternative 3: Bytecode VM
- **Rejected**: Adds unnecessary complexity, AST interpreter is simpler

## Unresolved Questions

- [ ] Should we support hybrid mode (interpreter + native code)?
- [ ] When to automatically switch modes?
- [ ] How to handle performance-critical paths?
- [ ] Should we add debug mode with interpreter fallback?

## References

- [RFC 0001: Migration Strategy](./0001-migration-strategy.md)
- [RFC 0002: Testing Environments](./0002-testing-environments.md)
- [RFC 0003: BeamAsm Template Approach](./0003-beamasm-template-approach.md)

