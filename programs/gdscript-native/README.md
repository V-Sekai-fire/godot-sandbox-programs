# GDScript to RISC-V Compiler (using StableHLO)

This program compiles GDScript code to RISC-V machine code using **StableHLO** (a MLIR dialect for ML compute operations) as the intermediate representation.

## Architecture

The compilation pipeline:

```
GDScript Source (.gd)
    ↓
[GDScript Parser] → Parse to AST (placeholder)
    ↓
[StableHLO IR] → Convert AST to StableHLO operations
    ↓
[RISC-V Assembly Emitter] → Walk StableHLO operations, emit assembly text
    ↓
RISC-V Assembly Text (string)
    ↓
[Assembler] → assemble_to() from libRiscvAsmLib.a
    ↓
RISC-V Machine Code (in memory)
    ↓
[Execute] → Register with Sandbox API
```

## StableHLO

This implementation uses [StableHLO](https://github.com/openxla/stablehlo), which is:

- A **portability layer** between ML frameworks and ML compilers
- Built on top of **MLIR** (without requiring LLVM backend)
- Provides **stable operations** for ML compute (add, multiply, constant, etc.)
- Uses **tensor types** for all operations

### Key StableHLO Operations Used

- `stablehlo.constant` - Create constant tensors
- `stablehlo.add` - Element-wise addition
- `stablehlo.subtract` - Element-wise subtraction  
- `stablehlo.multiply` - Element-wise multiplication
- `func.return` - Return from function

## Building

This project uses **git subrepo** to manage StableHLO and LLVM dependencies.

### Quick Start

1. **Setup git subrepos** (after committing your changes):
```bash
./setup-stablehlo-subrepo.sh
```

2. **Build MLIR and StableHLO**:
```bash
cd thirdparty/stablehlo
(hash="$(cat ./build_tools/llvm_version.txt)"; cd ../llvm-project && git fetch origin "$hash" && git checkout "$hash")
MLIR_ENABLE_BINDINGS_PYTHON=OFF build_tools/build_mlir.sh "${PWD}"/../llvm-project/ "${PWD}"/../llvm-build

mkdir -p build && cd build
cmake .. -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DSTABLEHLO_ENABLE_BINDINGS_PYTHON=OFF \
  -DMLIR_DIR="${PWD}"/../../llvm-build/lib/cmake/mlir
cmake --build .
```

3. **Build the project**:
```bash
cd build
cmake .. -GNinja
cmake --build .
```

The CMakeLists.txt will automatically find MLIR and StableHLO in `thirdparty/`.

### Manual Configuration

If you prefer to use custom paths:

```bash
cmake .. \
  -DMLIR_DIR=/path/to/llvm-build/lib/cmake/mlir \
  -DSTABLEHLO_DIR=/path/to/stablehlo/build/lib/cmake/stablehlo
```

See [BUILD.md](BUILD.md) for detailed instructions.

## Usage

The program provides the following Sandbox API functions:

- `compile_gdscript(String gdscript_code)` - Compile GDScript to RISC-V and return a callable function
- `test_compile()` - Test the compilation system with a simple function
- `test_dataset(int count)` - Test compilation with entries from the GDScript dataset
- `get_random_test()` - Get a random GDScript code sample from the dataset

## Implementation Details

### Tensor Types

StableHLO uses tensor types for all operations. For scalar values, we use rank-0 tensors (e.g., `tensor<i64>`).

### RISC-V Emission

The emitter extracts scalar values from StableHLO tensors and generates RISC-V assembly instructions. For rank-0 or single-element tensors, we extract the scalar value directly.

### Current Limitations

- Only supports simple scalar operations (constants, add, subtract, multiply)
- Control flow (branches, loops) not yet implemented
- Full GDScript AST parsing not yet implemented (uses placeholder functions)

## Future Work

- Implement full GDScript AST parser
- Add control flow support (branches, loops)
- Support more StableHLO operations
- Handle multi-dimensional tensors
- Optimize register allocation
