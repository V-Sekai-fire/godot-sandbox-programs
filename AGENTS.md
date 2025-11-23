# GDScript to RISC-V Compiler - Project Documentation

## Project Overview

This project implements a compiler that converts GDScript source code to RISC-V 64 Linux machine code using direct code generation (following BEAM JIT pattern). The compiler generates RISC-V assembly directly from the AST, eliminating the need for MLIR/StableHLO intermediate representation. The compiler is designed to work within the Godot Sandbox environment.

## Architecture

```
GDScript Source Code
    ↓
[cpp-peglib Parser] → AST (Abstract Syntax Tree)
    ↓
[Direct AST to RISC-V Emitter] → RISC-V 64 Linux Assembly Text
    ↓
[Assembler (libRiscvAsmLib)] → RISC-V 64 Machine Code
    ↓
[Execute] → Register with Sandbox API
```

**Note**: Following BEAM JIT pattern - no MLIR/StableHLO dependency. Direct code generation from AST to RISC-V 64 Linux.

## Current Implementation Status

### ✅ Completed Components

1. **Direct AST to RISC-V 64 Linux Emitter** (Replaces MLIR/StableHLO)
   - Direct code generation from AST (following BEAM JIT pattern)
   - No MLIR/StableHLO dependency - simpler and faster
   - Handwritten RISC-V 64 Linux assembly templates (`ast_to_riscv.h/cpp`)
   - Follows RISC-V 64 Linux ABI and calling conventions
   - Supports: functions, returns, literals, identifiers, binary operations, variable declarations

2. **Parser Grammar**
   - cpp-peglib integration (`parser/peglib.h`)
   - GDScript parser class (`parser/gdscript_parser.h/cpp`)
   - Complete AST node definitions (`parser/ast.h`)
   - Grammar correctly parses: functions, returns, literals, identifiers, variables, binary operations
   - **Verified**: Grammar works when tested directly with cpp-peglib

3. **Test Data Infrastructure**
   - Test data loader (`test_data_loader.h/cpp`)
   - Integration with godot-dodo dataset (60k+ GDScript samples)
   - Dataset located at `thirdparty/godot-dodo/`

4. **Testing Infrastructure**
   - doctest framework setup (`tests/doctest.h`)
   - Comprehensive test suite (`tests/test_parser.cpp`, `tests/test_ast_building.cpp`)
   - Test script for external RISC-V toolchain (`test_riscv_assembly.sh`)
   - Test CMake configuration
   - **Test Results**: Parser initialization passes, grammar parsing succeeds, AST building works

5. **Build System**
   - CMake integration (no MLIR/StableHLO dependencies)
   - RISC-V assembler library linking
   - Parser files integrated into build

### ✅ Completed Components (Updated)

1. **AST Building from Parse Results** (COMPLETED)
   - **Status**: Semantic actions fully implemented and working
   - **Implementation**: 
     - Semantic actions use `std::shared_ptr` and helper structs (BinaryOpData, FunctionData, ProgramData)
     - Conversion functions convert from `shared_ptr` (semantic actions) to `unique_ptr` (final AST)
     - Parser successfully builds AST with functions, statements, and expressions
   - **Workaround**: cpp-peglib doesn't store Program semantic value in parse result, so we use a member variable workaround
   - **Verified**: All basic AST building tests pass (simple functions, parameters, binary operations, variable declarations)
   - **Known Issues**: 
     - cpp-peglib semantic value storage issue (workaround in place)

2. **Direct AST to RISC-V 64 Linux Emitter** (COMPLETED)
   - **Status**: Fully implemented following BEAM JIT pattern
   - **Implementation**:
     - Handwritten RISC-V assembly templates (`ast_to_riscv.h/cpp`)
     - Direct code generation from AST nodes (no MLIR/StableHLO dependency)
     - Follows RISC-V 64 Linux ABI and calling conventions
     - Proper stack frame management with dynamic stack size tracking
     - Register allocation using temporary registers (t0-t6) and stack spilling
   - **Supports**: 
     - Functions with parameters (a0-a7)
     - Return statements (a0 return value)
     - Literals (integers, booleans, null)
     - Identifiers (variable references)
     - Binary operations (+, -, *, /, %)
     - Variable declarations with initializers
   - **Testing**: Test script created for external RISC-V toolchain validation
   - **Missing**: 
     - Control flow (if/else, loops)
     - Function calls
     - Float literals (converted to int)
     - String literals
     - Comparison operators

### ⚠️ Partially Implemented

1. **Testing Infrastructure**
   - Parser and AST building tests implemented and passing
   - Test script for external RISC-V toolchain created (`test_riscv_assembly.sh`)
   - Integration tests need to be run with actual RISC-V toolchain
   - End-to-end compilation tests ready but need verification

### ❌ Missing Components

1. **Full GDScript Grammar Support**
   - Control flow statements (if/elif/else, for, while, match)
   - Indentation-based block handling
   - Full type system
   - Advanced features (signals, enums, classes, etc.)

2. **Advanced AST to RISC-V Features**
   - Control flow conversion (if/else, loops)
   - Function call handling
   - Float literal support (currently converted to int)
   - String literal support
   - Comparison operators (==, !=, <, >, <=, >=)
   - Logical operators (and, or, not)
   - Type system integration

## Dependencies

### Third-Party Libraries (via git subrepo)

- **godot-dodo**: `thirdparty/godot-dodo/` - GDScript dataset with 60k+ samples

### External Libraries

- **cpp-peglib**: Single-file header-only PEG parser (integrated)
- **doctest**: Header-only testing framework (integrated)
- **libRiscvAsmLib.a**: RISC-V assembler library (already integrated)

### Testing Tools (External)

- **riscv64-linux-gnu-gcc**: RISC-V 64 Linux cross-compiler (for testing)
- **qemu-riscv64**: RISC-V 64 emulator (optional, for execution testing)

## Build Instructions

### Prerequisites

1. CMake 3.10 or higher
2. C++ compiler with C++17 support
3. (Optional) RISC-V 64 Linux toolchain for testing:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install gcc-riscv64-linux-gnu qemu-user-static
   ```

### Build Steps

```bash
cd build
cmake ..
cmake --build .
```

No MLIR/StableHLO dependencies required - much simpler build!

## API Documentation

### Sandbox API Functions

- `compile_gdscript(String gdscript_code) -> Callable`: Compile GDScript to RISC-V and return a callable function
- `test_compile() -> Variant`: Test the compilation system with a simple function
- `test_dataset(int count) -> Variant`: Test compilation with entries from the GDScript dataset
- `get_random_test() -> String`: Get a random GDScript code sample from the dataset

## Implementation Plan

### Phase 1: Parser Setup (Completed)
- [x] Add godot-dodo to thirdparty/ (already done)
- [x] Create AGENTS.md (this file)
- [x] Download and integrate cpp-peglib
- [x] Set up doctest testing framework

### Phase 2: AST Definitions (Completed)
- [x] Create complete AST node hierarchy
- [x] Define all GDScript language constructs (basic set)

### Phase 3: Parser Implementation (Completed - Basic)
- [x] Implement basic parser with cpp-peglib
- [x] Basic grammar for functions, returns, literals, identifiers
- [x] Variable declarations support
- [x] Binary operations support
- [ ] Full GDScript grammar (control flow, types, indentation, etc.) - incremental work
- [ ] Handle indentation-based blocks properly

### Phase 4: AST to RISC-V Emitter (Completed - Basic)
- [x] Implement direct AST to RISC-V 64 Linux assembly emitter
- [x] Handwritten instruction templates (following BEAM JIT pattern)
- [x] RISC-V 64 Linux ABI compliance
- [x] Stack frame management
- [x] Register allocation
- [x] Support for: functions, returns, literals, identifiers, binary operations, variable declarations
- [ ] Control flow support (if/else, loops)
- [ ] Function calls
- [ ] Full type system support

### Phase 5: Testing (In Progress)
- [x] Write doctest test infrastructure
- [x] Basic parser tests created and passing
- [x] AST building tests created and passing
- [x] Test script for external RISC-V toolchain created
- [x] Integration with main.cpp for end-to-end testing
- [ ] Run external toolchain tests
- [ ] Test with godot-dodo dataset samples

## Known Limitations

1. **cpp-peglib Semantic Value Storage**: cpp-peglib doesn't store the Program rule's semantic value in the parse result (returns void). Workaround: store ProgramData in parser instance member variable `last_program_data` and retrieve it in `parse()` method. This is a documented workaround until we find a better approach or fix the root cause.

2. **Parser Grammar**: Basic grammar implemented and verified - supports functions, returns, literals, identifiers, variables, binary operations. Missing: control flow (if/else, loops), match statements, full type system, proper indentation handling.

3. **Indentation Handling**: GDScript uses indentation for blocks - current grammar uses newlines, needs enhancement for real GDScript code.

4. **Type System**: Full GDScript type system support pending. Currently all values default to 64-bit integers.

5. **RISC-V Emitter Limitations**:
   - Float literals are converted to integers
   - String literals not yet implemented
   - Comparison and logical operators not yet implemented
   - Function calls not yet implemented
   - Control flow (if/else, loops) not yet implemented

6. **Stack Management**: Stack size tracking improved but may need further refinement for complex functions with many local variables.

## Immediate Next Steps (Priority Order)

1. **Test with External RISC-V Toolchain** (READY)
   - Run `test_riscv_assembly.sh` to validate generated assembly
   - Verify assembly compiles and executes correctly
   - Test with real GDScript samples

2. **Extend RISC-V Emitter**
   - Add comparison operators (==, !=, <, >, <=, >=)
   - Add logical operators (and, or, not)
   - Implement float literal support
   - Implement string literal support

3. **Add Control Flow**
   - Implement if/else statements
   - Implement for loops
   - Implement while loops
   - Add proper label management for branches

4. **Extend Grammar**
   - Add proper indentation handling
   - Add match statements
   - Add function call syntax
   - Add more GDScript features

5. **Improve Stack Management**
   - Refine stack size calculation for complex functions
   - Optimize register allocation
   - Add stack overflow detection

## Future Work

- Full GDScript language support
- Control flow optimizations
- Better error reporting
- Support for more complex GDScript features (classes, signals, enums, etc.)
- Test with real godot-dodo dataset samples
- Register allocation optimizations
- Stack frame optimizations

## Files Structure

```
programs/gdscript-native/
├── parser/              # Parser implementation
│   ├── peglib.h        # cpp-peglib header
│   ├── gdscript_parser.h/cpp
│   └── ast.h           # AST node definitions
├── ast_to_riscv.h/cpp  # Direct AST to RISC-V 64 Linux emitter
├── test_data_loader.h/cpp
├── test_riscv_assembly.sh  # Test script for external RISC-V toolchain
├── main.cpp
└── CMakeLists.txt

thirdparty/
├── godot-dodo/         # GDScript dataset (git subrepo)
└── asm/                # RISC-V assembler library
    └── libRiscvAsmLib.a
```

## Testing Strategy

- **Doctest Framework**: Systematic unit testing for parser and AST building
- **External Toolchain Testing**: Use `test_riscv_assembly.sh` to validate generated RISC-V assembly with `riscv64-linux-gnu-gcc`
- **Adhoc Testing**: Manual testing during development
- **Dataset Testing**: Test with real GDScript samples from godot-dodo
- **Integration Testing**: Full compilation pipeline testing (GDScript → AST → RISC-V → Machine Code)

### Running Tests

```bash
# Test with external RISC-V toolchain
cd programs/gdscript-native
./test_riscv_assembly.sh
```

