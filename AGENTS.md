# GDScript to RISC-V Compiler - Project Documentation

## Project Overview

This project implements a compiler that converts GDScript source code to RISC-V machine code using StableHLO (MLIR dialect) as an intermediate representation. The compiler is designed to work within the Godot Sandbox environment.

## Architecture

```
GDScript Source Code
    ↓
[cpp-peglib Parser] → AST (Abstract Syntax Tree)
    ↓
[AST to StableHLO Converter] → StableHLO IR
    ↓
[RISC-V Emitter] → Assembly Text
    ↓
[Assembler (libRiscvAsmLib)] → Machine Code
    ↓
[Execute] → Register with Sandbox API
```

## Current Implementation Status

### ✅ Completed Components

1. **MLIR/StableHLO Integration**
   - Real MLIR C++ API integration (no LLVM backend)
   - StableHLO dialect support
   - MLIR wrapper functions (`mlir_wrapper.h/cpp`)
   - RISC-V assembly emitter (`ir_to_riscv.h/cpp`)

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
   - Comprehensive test suite (`tests/test_parser.cpp`, `tests/test_adhoc.cpp`)
   - Test CMake configuration
   - **Test Results**: Parser initialization passes, grammar parsing succeeds, but AST building not implemented

5. **Build System**
   - CMake integration
   - MLIR and StableHLO dependency management
   - RISC-V assembler library linking
   - Parser files integrated into build

### ✅ Completed Components (Updated)

1. **AST Building from Parse Results** (COMPLETED)
   - **Status**: Semantic actions fully implemented and working
   - **Implementation**: 
     - Semantic actions use `std::shared_ptr` and helper structs (BinaryOpData, FunctionData, ProgramData)
     - Conversion functions convert from `shared_ptr` (semantic actions) to `unique_ptr` (final AST)
     - Parser successfully builds AST with functions, statements, and expressions
   - **Workaround**: cpp-peglib doesn't store Program semantic value in parse result, so we use a global variable workaround
   - **Verified**: All basic AST building tests pass (simple functions, parameters, binary operations, variable declarations)
   - **Known Issues**: 
     - cpp-peglib semantic value storage issue (workaround in place)
     - Debug output still present (should be removed for production)

2. **AST to MLIR Conversion** (READY FOR TESTING)
   - Code fully implemented and ready to test
   - Supports: functions, return statements, literals, identifiers, binary operations, variable declarations
   - Missing: control flow, function calls, complex expressions, type system

### ⚠️ Partially Implemented

1. **AST to MLIR Testing**
   - Code exists and should work, but requires MLIR build to test
   - Test infrastructure in place (`test_ast_to_mlir.cpp`)
   - Need to verify with actual MLIR build

### ❌ Missing Components

1. **AST Building Implementation**
   - Semantic actions not implemented
   - Need to convert parse results to AST nodes
   - Critical blocker for all downstream functionality

### ❌ Missing Components

1. **Full GDScript Grammar Support**
   - Control flow statements (if/elif/else, for, while, match)
   - Indentation-based block handling
   - Full type system
   - Advanced features (signals, enums, classes, etc.)

2. **Advanced AST to MLIR Features**
   - Control flow conversion
   - Function call handling
   - SSA conversion for complex code
   - Type system integration

## Dependencies

### Third-Party Libraries (via git subrepo)

- **StableHLO**: `thirdparty/stablehlo/` - MLIR dialect for ML compute operations
- **LLVM Project**: `thirdparty/llvm-project/` - Required by StableHLO for MLIR
- **godot-dodo**: `thirdparty/godot-dodo/` - GDScript dataset with 60k+ samples

### External Libraries

- **cpp-peglib**: Single-file header-only PEG parser (to be added)
- **doctest**: Header-only testing framework (to be added)
- **libRiscvAsmLib.a**: RISC-V assembler library (already integrated)

## Build Instructions

### Prerequisites

1. Build MLIR and StableHLO (see `thirdparty/README.md`)
2. Ensure CMake can find MLIR and StableHLO

### Build Steps

```bash
cd build
cmake .. \
  -DMLIR_DIR=${PWD}/../thirdparty/llvm-build/lib/cmake/mlir \
  -DSTABLEHLO_DIR=${PWD}/../thirdparty/stablehlo/build/lib/cmake/stablehlo
cmake --build .
```

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

### Phase 4: AST to MLIR Conversion (Completed - Basic)
- [x] Implement AST traversal
- [x] Convert AST nodes to StableHLO operations (basic: literals, identifiers, binary ops)
- [x] Handle variable mapping
- [x] Replace placeholder functions in ASTToIRConverter
- [ ] Full type system support
- [ ] SSA conversion for complex control flow

### Phase 5: Testing (Completed - Infrastructure)
- [x] Write doctest test infrastructure
- [x] Basic parser tests created
- [x] Test infrastructure ready for dataset testing
- [x] Integration with main.cpp for end-to-end testing
- [ ] Run and verify tests compile (requires build)
- [ ] Test with godot-dodo dataset samples (requires working parser)

## Known Limitations

1. **cpp-peglib Semantic Value Storage**: cpp-peglib doesn't store the Program rule's semantic value in the parse result (returns void). Workaround: store ProgramData in parser instance member variable `last_program_data` and retrieve it in `parse()` method. This is a documented workaround until we find a better approach or fix the root cause.

2. **Parser Grammar**: Basic grammar implemented and verified - supports functions, returns, literals, identifiers, variables, binary operations. Missing: control flow (if/else, loops), match statements, full type system, proper indentation handling.

3. **Indentation Handling**: GDScript uses indentation for blocks - current grammar uses newlines, needs enhancement for real GDScript code.

4. **Type System**: Full GDScript type system support pending.

5. **AST to MLIR Conversion**: Code exists and should work, but needs testing with actual MLIR build to verify.

6. **End-to-End Compilation**: Ready to test once AST to MLIR is verified.

## Immediate Next Steps (Priority Order)

1. **Test AST to MLIR Conversion** (IN PROGRESS)
   - Code is implemented and should work
   - Need to test with actual MLIR build
   - Verify conversion works for: functions, returns, literals, identifiers, binary operations, variable declarations
   - Run systematic tests with doctest

2. **Remove Debug Output**
   - Remove or make conditional all debug `std::cout` statements
   - Keep only essential error reporting

3. **Investigate cpp-peglib Issue**
   - Research why Program semantic value isn't stored in parse result
   - Find proper solution instead of global variable workaround
   - May need to file issue with cpp-peglib or use different approach

4. **Test End-to-End Compilation**
   - Verify full pipeline: GDScript → AST → StableHLO → RISC-V → Execution
   - Test with real GDScript samples from godot-dodo dataset

5. **Extend Grammar**
   - Add control flow (if/else, loops)
   - Add proper indentation handling
   - Add match statements
   - Add more GDScript features

## Future Work

- Full GDScript language support
- Optimizations in StableHLO IR
- Better error reporting
- Support for more complex GDScript features (classes, signals, enums, etc.)
- Test with real godot-dodo dataset samples

## Files Structure

```
programs/gdscript-native/
├── parser/              # Parser implementation (to be created)
│   ├── peglib.h        # cpp-peglib header
│   ├── gdscript_parser.h/cpp
│   └── ast.h           # AST node definitions
├── mlir/               # MLIR/StableHLO integration
│   ├── mlir_wrapper.h/cpp
│   └── ir_to_riscv.h/cpp
├── ast_to_ir.h/cpp     # AST to MLIR converter
├── test_data_loader.h/cpp
├── main.cpp
└── CMakeLists.txt

thirdparty/
├── stablehlo/          # StableHLO (git subrepo)
├── llvm-project/       # LLVM project (git subrepo)
├── godot-dodo/         # GDScript dataset (git subrepo)
└── llvm-build/         # LLVM build directory
```

## Testing Strategy

- **Doctest Framework**: Systematic unit testing before and after each implementation step
- **Adhoc Testing**: Manual testing during development
- **Dataset Testing**: Test with real GDScript samples from godot-dodo
- **Integration Testing**: Full compilation pipeline testing

