# GDScript to RISC-V Compiler - Quick Reference

## Project Overview

This project implements a compiler that converts GDScript source code to RISC-V 64 Linux machine code using direct code generation (following BEAM JIT pattern). The compiler generates RISC-V machine code directly from the AST using the **biscuit** library.

## Architecture

```
GDScript Source Code
    ↓
[Parser] → AST (Abstract Syntax Tree)
    ↓
[Direct AST to RISC-V Emitter (biscuit)] → RISC-V 64 Machine Code
    ↓
[ELF Generator] → RISC-V 64 Linux ELF File
    ↓
[libriscv Execution] → Register with Sandbox API
```

**Note**: Following BEAM JIT pattern - no MLIR/StableHLO dependency. Direct code generation from AST to RISC-V 64 Linux machine code using **biscuit**.

## Current Status

✅ **Core Features Implemented**
- Recursive descent parser (hand-written, like Godot's parser)
- INDENT/DEDENT token-based indentation handling
- Direct AST to RISC-V machine code generation (biscuit)
- ELF generation with libriscv compatibility
- Function registry and calling convention
- Memory management (RAII)
- Structured error handling
- Comparison operators (==, !=, <, >, <=, >=)
- Dynamic stack size tracking
- Buffer growth strategy
- Godot C++ naming conventions throughout

## Architectural Decisions

All architectural decisions are documented as RFCs in `programs/gdscript-native/decisions/`:

- [RFC 0001: Migration Strategy](./programs/gdscript-native/decisions/0001-migration-strategy.md)
- [RFC 0002: Testing Environments](./programs/gdscript-native/decisions/0002-testing-environments.md)
- [RFC 0003: BeamAsm Template Approach](./programs/gdscript-native/decisions/0003-beamasm-template-approach.md)
- [RFC 0004: Dual-Mode Architecture](./programs/gdscript-native/decisions/0004-dual-mode-architecture.md)
- [RFC 0005: Recursive Descent Parser](./programs/gdscript-native/decisions/0005-recursive-descent-parser.md)
- [RFC 0006: INDENT/DEDENT Tokens](./programs/gdscript-native/decisions/0006-indent-dedent-tokens.md)
- [RFC 0007: Biscuit RISC-V Codegen](./programs/gdscript-native/decisions/0007-biscuit-riscv-codegen.md)
- [RFC 0008: ELF Generation for libriscv](./programs/gdscript-native/decisions/0008-elf-generation-libriscv.md)
- [RFC 0009: Structured Error Handling](./programs/gdscript-native/decisions/0009-structured-error-handling.md)
- [RFC 0010: Memory Management RAII](./programs/gdscript-native/decisions/0010-memory-management-raii.md)
- [RFC 0011: Constants Centralization](./programs/gdscript-native/decisions/0011-constants-centralization.md)
- [RFC 0012: Godot C++ Naming Conventions](./programs/gdscript-native/decisions/0012-godot-cpp-naming-conventions.md)
- [RFC 0013: Function Registry](./programs/gdscript-native/decisions/0013-function-registry-calling-convention.md)
- [RFC 0014: Dynamic Buffer Growth](./programs/gdscript-native/decisions/0014-dynamic-buffer-growth.md)
- [RFC 0015: AST to RISC-V Direct Compilation](./programs/gdscript-native/decisions/0015-ast-to-riscv-direct-compilation.md)

## Supported Features

### ✅ Fully Supported
- Simple functions
- Functions with parameters
- Return statements
- Integer, boolean, and null literals
- Variable declarations and references
- Binary arithmetic operations (+, -, *, /, %)
- Comparison operators (==, !=, <, >, <=, >=)
- Complex expressions with operator precedence

### ⚠️ Partially Supported
- String literals (parsed, not yet in codegen)
- Float literals (parsed, converted to int in codegen)

### ❌ Not Yet Supported
- Control flow (if/elif/else, for, while, match)
- Logical operators (and, or, not)
- Function calls
- Complex types (arrays, dictionaries)
- Type system (type checking, inference)
- Advanced features (classes, signals, enums)

## Dependencies

- **biscuit**: RISC-V code generator (header-only, similar to AsmJit)
- **libriscv**: RISC-V emulator (used by Godot sandbox)
- **godot-dodo**: GDScript dataset (60k+ samples)
- **doctest**: Testing framework (header-only)

## Build Instructions

```bash
cd build
cmake ..
cmake --build .
```

No MLIR/StableHLO dependencies required.

## API Functions

- `compile_gdscript(String gdscript_code) -> Variant`: Compile GDScript and return result
- `test_compile() -> Variant`: Test the compilation system
- `set_compiler_mode(int mode)`: Set compiler mode (0: INTERPRET, 1: NATIVE_CODE)
- `test_dataset(int count) -> Variant`: Test with dataset samples
- `get_random_test() -> String`: Get random GDScript sample

## Immediate Next Steps

1. **Extend RISC-V Emitter**
   - Add logical operators (and, or, not)
   - Implement float literal support
   - Implement string literal support

2. **Add Control Flow**
   - Implement if/else statements
   - Implement for loops
   - Implement while loops

3. **Function Call Support**
   - Generate RISC-V code to call other functions
   - Support inter-function calls

4. **Improve Stack Management**
   - Refine stack size calculation
   - Optimize register allocation

## Files Structure

```
programs/gdscript-native/
├── src/
│   ├── parser/              # Recursive descent parser
│   ├── ast_to_riscv_biscuit.h/cpp  # RISC-V emitter
│   ├── elf_generator.h/cpp         # ELF generator
│   ├── function_registry.h/cpp     # Function registry
│   ├── code_memory_manager.h/cpp   # Memory management
│   ├── ast_interpreter.h/cpp       # AST interpreter
│   └── main.cpp                    # Sandbox integration
└── tests/                      # Test suite
```

## Testing

- **Doctest Framework**: Unit testing for parser and AST building
- **libriscv Integration**: ELF validation and execution testing
- **External Toolchain**: RISC-V cross-compiler for assembly validation
