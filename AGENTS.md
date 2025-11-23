# GDScript to RISC-V Compiler - Project Documentation

## Project Overview

This project implements a compiler that converts GDScript source code to RISC-V 64 Linux machine code using direct code generation (following BEAM JIT pattern). The compiler generates RISC-V machine code directly from the AST using the **biscuit** library, eliminating the need for MLIR/StableHLO intermediate representation. The compiler is designed to work within the Godot Sandbox environment.

## Current Status (Latest Updates)

✅ **Codebase Refactored to Godot C++ Naming Conventions**
- All methods use snake_case (e.g., `get_type()`, `emit_function()`)
- All private members use `_` prefix (e.g., `_code_buffer`, `_stack_offset`)
- All private methods use `_` prefix (e.g., `_allocate_register()`)
- Consistent with Godot engine code style throughout

✅ **All Deprecated Code Removed**
- Entire `deprecated/` directory deleted (MLIR-based code)
- MLIR-based test files removed
- CMakeLists.txt cleaned up (no MLIR dependencies)
- Codebase now uses only direct AST → RISC-V compilation with biscuit

✅ **Core Features Implemented**
- Direct AST to RISC-V machine code generation (biscuit)
- Function registry and calling convention
- Memory management (RAII)
- Structured error handling
- Comparison operators (==, !=, <, >, <=, >=)
- Dynamic stack size tracking
- Buffer growth strategy

## Architecture

```
GDScript Source Code
    ↓
[cpp-peglib Parser] → AST (Abstract Syntax Tree)
    ↓
[Direct AST to RISC-V Emitter (biscuit)] → RISC-V 64 Machine Code
    ↓
[Execute] → Register with Sandbox API
```

**Note**: Following BEAM JIT pattern - no MLIR/StableHLO dependency. Direct code generation from AST to RISC-V 64 Linux machine code using **biscuit** (similar to how BEAM JIT uses AsmJit).

### Architectural Decisions

#### Why AST → RISC-V (Not Godot Bytecode → RISC-V)

**Decision**: Use direct AST → RISC-V compilation, NOT Godot bytecode format.

**Rationale**:
- **BEAM JIT Pattern**: BEAM JIT generates machine code directly from BEAM instructions (not from AST, but still direct). Our approach generates directly from AST, which is even simpler.
- **Performance**: Native code generation eliminates VM overhead. No interpretation layer needed.
- **Simplicity**: Single translation step (AST → RISC-V) is simpler than two steps (GDScript → Bytecode → RISC-V).
- **Independence**: No dependency on Godot's internal bytecode format, which:
  - Contains non-serializable pointers
  - May change between Godot versions
  - Is designed for VM interpretation, not direct code generation
  - Is not publicly documented/stable
- **Control**: Full control over code generation allows for optimizations and RISC-V-specific improvements.

**Alternative Considered**: Using Godot's bytecode format would require:
- Reverse-engineering or depending on Godot internals
- Translating stack-based bytecode to register-based RISC-V
- Handling version compatibility issues
- More complex architecture

**Conclusion**: Direct AST → RISC-V compilation aligns with BEAM JIT philosophy of simplicity and direct code generation.

#### Parser Choice (Hand-Written Recursive Descent)

**Current**: Using cpp-peglib (PEG parser generator) - **TO BE REPLACED**

**Status**: Working but has limitations:
- Requires workarounds for Program rule semantic value storage
- Fragile semantic value extraction (string-based type checking)
- Limited error reporting capabilities
- Complex std::any conversions (shared_ptr → unique_ptr)

**Decision**: **Switch to hand-written recursive descent parser**

**Rationale**:
- **Better Error Messages**: Direct control over error reporting with source locations
- **Direct AST Construction**: No std::any conversions needed, build AST nodes directly
- **Easier Debugging**: Clear parsing logic, easy to step through in debugger
- **Full Control**: Complete control over parsing behavior and error recovery
- **Simpler**: No external parser generator, just C++ code
- **Alignment**: Matches BEAM JIT philosophy of simplicity and direct code generation

**Implementation Plan**:
1. **Lexer**: Tokenize source code with source location tracking
2. **Parser**: Recursive descent parser with direct AST construction
3. **Error Handling**: Structured error system with multiple error collection
4. **Migration**: Implement alongside existing parser, test thoroughly, then replace

**Benefits Over cpp-peglib**:
- Eliminates all workarounds (no Program rule issue, no std::any conversions)
- Better error messages with source context
- Type-safe AST construction
- Easier to extend with new language features
- Better performance (no parser generator overhead)

#### Godot Sandbox Calling Convention

**Current State**: Generated assembly functions return `int64_t` in register `a0`, but Godot expects `Variant` (C++ class).

**Problem**: 
- Generated RISC-V code follows standard RISC-V 64 Linux ABI (returns int64 in a0)
- Godot API expects `Variant(*)()` function signature
- Need to bridge between assembly (guest) and C++ (host)

**Solution**: C++ Wrapper Function (Recommended)
- Create C++ wrapper functions that call generated assembly
- Wrapper converts int64 result to Variant
- Register wrappers with `ADD_API_FUNCTION`

**Implementation Approach**:
1. **Function Registry**: Track generated function addresses by name
2. **Wrapper Generation**: Create C++ lambda wrappers for each compiled function
3. **Assembly Call Helper**: Helper function to call assembly and get int64 result
4. **Variant Conversion**: Convert int64 → Variant in wrapper

**Flow**:
```
GDScript → Callable → C++ Wrapper → Generated Assembly → int64 → Variant → GDScript
```

**Status**: ✅ **IMPLEMENTED**

**Implementation**:
- `FunctionRegistry` class created (`function_registry.h/cpp`)
- `call_assembly_function()` helper implemented (Godot C++ naming)
- C++ wrapper generation in `compile_gdscript()`
- Functions registered and wrappers created automatically
- int64 → Variant conversion in wrappers
- All methods use snake_case naming (Godot C++ conventions)

**Future Considerations**:
- For functions with parameters: May need syscall/helper approach
- For complex return types: May need direct Variant construction in assembly
- Inter-function calls: Function registry ready for this

## Architectural Improvements Needed

### 1. Error Handling and Reporting ✅ **COMPLETED**

**Previous State**: Basic error messages, no structured error system.

**Status**: ✅ **IMPLEMENTED**

**Implementation**:
- `CompilationError` class with ErrorType enum (Parse, Semantic, Codegen)
- `SourceLocation` struct (line, column)
- `ErrorCollection` class for multiple errors per compilation
- Formatted error messages with source context
- Integrated into parser (`parser/errors.h/cpp`)

**Remaining Improvements**:
- **Structured Error Types**: Create error hierarchy (ParseError, CompileError, RuntimeError)
- **Error Context**: Include source location (line/column) in all errors
- **Error Collection**: Support multiple errors per compilation (don't stop at first error)
- **Error Formatting**: User-friendly error messages with source code snippets
- **Error Recovery**: Continue parsing after errors to find more issues

**Implementation**:
```cpp
class CompilationError {
    enum class ErrorType { Parse, Semantic, Codegen };
    ErrorType type;
    std::string message;
    SourceLocation location;
    std::string context; // Source code snippet
};
```

### 2. Memory Management ✅ **COMPLETED**

**Previous State**: `mmap` allocations in `main.cpp` are not tracked or cleaned up.

**Status**: ✅ **IMPLEMENTED**

**Implementation**:
- `CodeMemoryManager` class created (`code_memory_manager.h/cpp`)
- `ExecutableMemory` RAII wrapper (auto cleanup with munmap)
- All `mmap` allocations tracked in manager
- Buffer growth strategy in `ASTToRISCVEmitter` (grows when 90% full)

**Remaining Improvements**:
- **Memory Tracking**: Track all executable memory allocations
- **Cleanup Mechanism**: Provide way to free allocated memory
- **Memory Pool**: Consider memory pool for code generation
- **Buffer Management**: Handle buffer growth in emitter (currently fixed 8KB)

**Implementation**:
- Add `CodeMemoryManager` class to track and manage executable memory
- Implement RAII wrapper for executable memory
- Add buffer growth strategy in `ASTToRISCVEmitter`

### 3. Code Organization and Cleanup ✅ **COMPLETED**

**Previous State**: Legacy files existed but were not used.

**Status**: ✅ **IMPLEMENTED**

**Implementation**:
- **Removed** all deprecated MLIR-based code:
  - Deleted `ast_to_ir.cpp/h` (MLIR-based AST to IR conversion)
  - Deleted `ast_to_riscv.cpp/h` (old RISC-V assembly text emitter)
  - Deleted entire `mlir/` directory (MLIR integration code)
  - Deleted MLIR-based test files (`test_integration.cpp`, `test_ast_to_mlir.cpp`)
- Cleaned up `tests/CMakeLists.txt` to remove MLIR/StableHLO dependencies
- Only one code generation path exists: `ast_to_riscv_biscuit.h/cpp`
- Codebase now uses only direct AST → RISC-V compilation (BEAM JIT pattern)

### 4. Visitor Pattern for AST Traversal

**Current State**: Emitter uses switch statements on node types.

**Improvements Needed**:
- **AST Visitor Pattern**: Implement visitor pattern for cleaner code generation
- **Separation of Concerns**: Separate different concerns (codegen, optimization, analysis)
- **Extensibility**: Easier to add new node types and operations

**Benefits**:
- Cleaner code organization
- Easier to add optimizations
- Better separation between AST and code generation
- Type-safe node handling

**Implementation**:
```cpp
class ASTVisitor {
    virtual void visitProgram(ProgramNode* node) = 0;
    virtual void visitFunction(FunctionNode* node) = 0;
    // ... etc
};

class CodegenVisitor : public ASTVisitor {
    // Code generation logic
};
```

### 5. Symbol Table and Name Resolution

**Current State**: No symbol table - variables tracked by name in emitter.

**Improvements Needed**:
- **Symbol Table**: Implement proper symbol table for scoping
- **Name Resolution**: Resolve identifiers to symbols (variables, functions, etc.)
- **Scope Management**: Track variable scopes (function, block, etc.)
- **Type Information**: Store type information in symbol table

**Benefits**:
- Proper scoping rules
- Better error messages (undefined variable, etc.)
- Type checking support
- Function call resolution

**Implementation**:
```cpp
class SymbolTable {
    struct Symbol {
        std::string name;
        Type type;
        int stackOffset;
        Scope scope;
    };
    std::vector<Scope> scopes; // Stack of scopes
};
```

### 6. Type System

**Current State**: All values default to 64-bit integers.

**Improvements Needed**:
- **Type Representation**: Define type system (int, float, string, bool, etc.)
- **Type Inference**: Infer types from literals and operations
- **Type Checking**: Validate type compatibility
- **Type Annotations**: Support GDScript type hints

**Implementation**:
```cpp
class Type {
    enum class Kind { Int, Float, String, Bool, Void, Unknown };
    Kind kind;
    // ... type information
};
```

### 7. Multi-Pass Compilation

**Current State**: Single-pass code generation (AST → RISC-V).

**Improvements Needed**:
- **Analysis Pass**: Type checking, symbol resolution
- **Optimization Pass**: Dead code elimination, constant folding, etc.
- **Code Generation Pass**: Generate RISC-V code
- **Validation Pass**: Verify generated code

**Benefits**:
- Better error detection
- Optimization opportunities
- Cleaner separation of concerns
- Easier debugging

**Architecture**:
```
AST → [Analysis] → [Optimization] → [Code Generation] → RISC-V
```

### 8. Testing Architecture

**Current State**: Basic unit tests, no integration testing framework.

**Improvements Needed**:
- **Integration Test Framework**: Test full compilation pipeline
- **Golden File Testing**: Compare generated code against expected output
- **Property-Based Testing**: Test with random GDScript samples
- **Performance Testing**: Benchmark compilation and execution

**Implementation**:
- Create `CompilerTest` framework
- Add golden file comparison for generated RISC-V code
- Integrate with godot-dodo dataset for property testing

### 9. Code Generation Strategy

**Current State**: Stack size calculated before body emission, may need adjustment.

**Improvements Needed**:
- **Two-Pass Code Generation**: 
  - Pass 1: Calculate stack size, register needs
  - Pass 2: Generate code with known sizes
- **Or Dynamic Stack Adjustment**: Adjust stack size during generation
- **Label Management**: Proper label generation for control flow

**Current Issue**: Stack size calculated in prologue but may grow during body emission.

### 10. Documentation and Diagrams

**Current State**: Text-based architecture description.

**Improvements Needed**:
- **Architecture Diagrams**: Visual representation of compilation pipeline
- **Data Flow Diagrams**: Show how data flows through compiler
- **Component Diagrams**: Show relationships between components
- **Sequence Diagrams**: Show compilation process for example code

**Tools**: Consider Mermaid diagrams in markdown or separate diagram files.

### 11. Configuration and Options

**Current State**: No configuration system.

**Improvements Needed**:
- **Compiler Options**: Optimization levels, debug info, etc.
- **Target Configuration**: RISC-V variants, extensions
- **Output Format**: Machine code, assembly text, or both

**Implementation**:
```cpp
struct CompilerOptions {
    bool optimize = false;
    bool debugInfo = false;
    RISCVTarget target = RISCVTarget::RV64GC;
    OutputFormat output = OutputFormat::MachineCode;
};
```

### 12. Function Call Support Architecture

**Current State**: Function registry implemented, function call code generation pending.

**Status**: ⚠️ **PARTIALLY IMPLEMENTED**

**Completed**:
- ✅ **Function Registry**: `FunctionRegistry` class tracks compiled functions (`function_registry.h/cpp`)
- ✅ **Calling Convention**: C++ wrapper functions convert int64 → Variant
- ✅ **Function Lookup**: `get_function()` resolves function names to addresses (snake_case)

**Remaining**:
- **Function Call Code Generation**: Generate RISC-V code to call other functions
- **Inter-Function Calls**: Support calling other compiled functions from generated code

## Priority Order for Architectural Improvements

1. **High Priority** (Critical for functionality): ✅ **COMPLETED**
   - ✅ Memory management (leak prevention) - **DONE**
   - ✅ Error handling (better user experience) - **DONE**
   - ✅ Code organization cleanup (reduce confusion) - **DONE**

2. **Medium Priority** (Improves code quality):
   - Visitor pattern (better code organization)
   - Symbol table (proper scoping)
   - Type system (type safety)

3. **Low Priority** (Nice to have):
   - Multi-pass compilation (optimization)
   - Testing architecture (quality assurance)
   - Documentation diagrams (understanding)

## Current Implementation Status

### ✅ Completed Components

1. **Direct AST to RISC-V 64 Linux Emitter using biscuit** ✅ **COMPLETED**
   - Direct code generation from AST to machine code (following BEAM JIT pattern)
   - Uses **biscuit** library (MIT license) - similar to AsmJit used in BEAM JIT
   - **No MLIR/StableHLO dependency** - all deprecated code removed
   - Generates RISC-V 64 Linux machine code directly (`ast_to_riscv_biscuit.h/cpp`)
   - Follows RISC-V 64 Linux ABI and calling conventions
   - Uses Godot C++ naming conventions (snake_case, _ prefix for private members)
   - Supports: functions, returns, literals, identifiers, binary operations, variable declarations, comparison operators
   - **Key advantage**: Generates machine code directly (like BEAM JIT), no text assembly step needed

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
   - CMake integration (no MLIR/StableHLO dependencies - all removed)
   - biscuit library integration (header-only, no linking needed)
   - Parser files integrated into build
   - Clean test configuration (no MLIR dependencies)

6. **Code Style and Naming Conventions** ✅ **COMPLETED**
   - Refactored entire codebase to use Godot C++ naming conventions
   - All methods use snake_case (e.g., `get_type()`, `emit_function()`)
   - All private members use `_` prefix (e.g., `_code_buffer`, `_stack_offset`)
   - All private methods use `_` prefix (e.g., `_allocate_register()`)
   - Consistent with Godot engine code style

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

2. **Direct AST to RISC-V 64 Linux Emitter using biscuit** (COMPLETED)
   - **Status**: Fully implemented following BEAM JIT pattern
   - **Implementation**:
     - Uses **biscuit** library for direct machine code generation (`ast_to_riscv_biscuit.h/cpp`)
     - Similar to how BEAM JIT uses AsmJit - generates machine code directly, not assembly text
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
   - **Supports (Updated)**: 
     - Comparison operators (==, !=, <, >, <=, >=) ✅ **ADDED**
     - Improved stack size calculation with dynamic tracking ✅ **IMPROVED**
     - Improved register allocation with cleanup ✅ **IMPROVED**
     - Buffer growth strategy (grows when 90% full) ✅ **ADDED**
     - Godot C++ naming conventions throughout ✅ **REFACTORED**
   - **Missing**: 
     - Control flow (if/else, loops)
     - Function calls
     - Float literals (converted to int)
     - String literals

### ✅ Newly Completed Components

1. **Godot Sandbox Calling Convention** ✅ **COMPLETED**
   - `FunctionRegistry` class to track compiled function addresses
   - `call_assembly_function()` helper to call assembly and get int64 result
   - C++ wrapper generation in `compile_gdscript()` that converts int64 → Variant
   - Functions automatically registered and wrappers created
   - All methods use snake_case naming (Godot C++ conventions)
   - Files: `function_registry.h/cpp`

2. **Memory Management** ✅ **COMPLETED**
   - `CodeMemoryManager` class to track executable memory allocations
   - `ExecutableMemory` RAII wrapper for automatic cleanup (munmap)
   - Buffer growth strategy in `ASTToRISCVEmitter` (grows when 90% full)
   - All `mmap` allocations tracked and managed
   - Files: `code_memory_manager.h/cpp`

3. **Structured Error Handling** ✅ **COMPLETED**
   - `CompilationError` class with error types (Parse, Semantic, Codegen)
   - `ErrorCollection` class for multiple errors per compilation
   - `SourceLocation` struct for line/column tracking
   - Formatted error messages with source context
   - Integrated into parser
   - Files: `parser/errors.h/cpp`

4. **Code Organization Cleanup** ✅ **COMPLETED**
   - **Removed** all deprecated MLIR-based code (entire `deprecated/` directory deleted)
   - Removed MLIR-based test files
   - Cleaned up CMakeLists.txt to remove MLIR dependencies
   - Only one code generation path: `ast_to_riscv_biscuit.h/cpp`
   - Codebase now uses only direct AST → RISC-V compilation

5. **RISC-V Emitter Improvements** ✅ **COMPLETED**
   - Comparison operators implemented (==, !=, <, >, <=, >=)
   - Stack size calculation improved with dynamic tracking
   - Register allocation improved with cleanup notes
   - Buffer growth strategy added (grows when 90% full)
   - All methods refactored to snake_case with _ prefix for private

6. **Code Style Refactoring** ✅ **COMPLETED**
   - Entire codebase refactored to Godot C++ naming conventions
   - All methods: camelCase → snake_case (e.g., `getType()` → `get_type()`)
   - All private members: added `_` prefix (e.g., `codeBuffer` → `_code_buffer`)
   - All private methods: added `_` prefix (e.g., `emitFunction()` → `_emit_function()`)
   - Consistent with Godot engine code style throughout

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
- **biscuit**: RISC-V runtime code generator (MIT license) - similar to AsmJit, integrated via FetchContent

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

### Phase 3: Parser Implementation (In Progress - Bug Fixes)
- [x] Implement basic parser with cpp-peglib
- [x] Basic grammar for functions, returns, literals, identifiers
- [x] Variable declarations support
- [x] Binary operations support
- [x] Fix BinaryOp operator extraction (use semantic action)
- [x] Complete variable declaration handling (type hints, initializers)
- [ ] Improve return statement expression extraction
- [ ] Implement error message retrieval from parser
- [ ] Full GDScript grammar (control flow, types, indentation, etc.) - incremental work
- [ ] Handle indentation-based blocks properly

### Phase 4: AST to RISC-V Emitter (In Progress - Improvements)
- [x] Implement direct AST to RISC-V 64 Linux machine code emitter
- [x] Handwritten instruction templates (following BEAM JIT pattern)
- [x] RISC-V 64 Linux ABI compliance
- [x] Stack frame management
- [x] Register allocation
- [x] Support for: functions, returns, literals, identifiers, binary operations, variable declarations
- [x] Fix stack size calculation (track dynamic growth) ✅
- [x] Improve register allocation (cleanup notes added) ✅
- [x] Add comparison operators (==, !=, <, >, <=, >=) ✅
- [x] Buffer growth strategy (grows when 90% full) ✅
- [x] Refactor to Godot C++ naming conventions ✅
- [ ] Control flow support (if/else, loops)
- [ ] Function calls
- [ ] Full type system support

### Phase 5: Testing (In Progress)
- [x] Write doctest test infrastructure
- [x] Basic parser tests created and passing
- [x] AST building tests created and passing
- [x] Test script for external RISC-V toolchain created
- [x] Integration with main.cpp for end-to-end testing
- [ ] Add AST structure validation to tests
- [ ] Update test script to test compiler-generated code
- [ ] Run external toolchain tests
- [ ] Test with godot-dodo dataset samples

### Phase 6: Documentation (In Progress)
- [x] Document architectural decisions (AST → RISC-V, not bytecode)
- [x] Update AGENTS.md to reflect current state (deprecated code removed, naming conventions)
- [ ] Update README.md to reflect biscuit-based architecture (remove MLIR/StableHLO references)
- [ ] Consolidate status files (CURRENT_STATUS.md, IMPLEMENTATION_STATUS.md)

### Phase 7: Code Cleanup ✅ **COMPLETED**
- [x] Remove all deprecated MLIR-based code
- [x] Remove MLIR-based test files
- [x] Clean up CMakeLists.txt (remove MLIR dependencies)
- [x] Refactor to Godot C++ naming conventions
- [x] Update all method names to snake_case
- [x] Add _ prefix to all private members and methods

## Known Limitations

1. **cpp-peglib Semantic Value Storage**: cpp-peglib doesn't store the Program rule's semantic value in the parse result (returns void). Workaround: store ProgramData in parser instance member variable `last_program_data` and retrieve it in `parse()` method. This is a documented workaround. **Future**: May switch to hand-written parser to eliminate this limitation.

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
   - ✅ Comparison operators (==, !=, <, >, <=, >=) - **DONE**
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
- **Parser**: Consider switching to hand-written recursive descent parser if cpp-peglib limitations become too burdensome

## Files Structure

```
programs/gdscript-native/
├── parser/              # Parser implementation
│   ├── peglib.h        # cpp-peglib header
│   ├── gdscript_parser.h/cpp
│   ├── ast.h           # AST node definitions
│   └── errors.h/cpp    # Error handling (CompilationError, ErrorCollection)
├── ast_to_riscv_biscuit.h/cpp  # Direct AST to RISC-V 64 Linux emitter (using biscuit)
├── function_registry.h/cpp     # Function registry for calling convention
├── code_memory_manager.h/cpp   # Executable memory management (RAII)
├── test_data_loader.h/cpp      # Test data loader for godot-dodo dataset
├── test_riscv_assembly.sh      # Test script for external RISC-V toolchain
├── main.cpp                    # Sandbox integration
├── CMakeLists.txt
└── tests/                      # Test suite
    ├── test_parser.cpp         # Parser tests
    ├── test_compiler.cpp       # TDD-style compiler tests
    ├── test_dataset.cpp        # Dataset tests
    └── CMakeLists.txt

thirdparty/
├── godot-dodo/         # GDScript dataset (git subrepo)
└── biscuit/            # RISC-V code generator (header-only, similar to AsmJit)
```

**Note**: All deprecated MLIR-based code has been removed. The codebase now uses only direct AST → RISC-V compilation with biscuit.

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

