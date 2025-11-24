# Third-Party Dependencies

This directory contains third-party dependencies managed via git subrepo.

## StableHLO

[StableHLO](https://github.com/openxla/stablehlo) is a MLIR dialect for ML compute operations.

### Setup

1. Run the setup script (after committing your changes):
```bash
./setup-stablehlo-subrepo.sh
```

2. Build MLIR from LLVM:
```bash
cd thirdparty/stablehlo
(hash="$(cat ./build_tools/llvm_version.txt)"; cd ../llvm-project && git fetch origin "$hash" && git checkout "$hash")
MLIR_ENABLE_BINDINGS_PYTHON=OFF build_tools/build_mlir.sh "${PWD}"/../llvm-project/ "${PWD}"/../llvm-build
```

3. Build StableHLO:
```bash
cd thirdparty/stablehlo
mkdir -p build && cd build
cmake .. -GNinja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON \
  -DSTABLEHLO_ENABLE_BINDINGS_PYTHON=OFF \
  -DMLIR_DIR="${PWD}"/../../llvm-build/lib/cmake/mlir
cmake --build .
```

## Git Subrepo Commands

### List all subrepos
```bash
git subrepo status
```

### Update a subrepo
```bash
git subrepo pull thirdparty/stablehlo
git subrepo pull thirdparty/llvm-project
git subrepo pull thirdparty/libriscv
git subrepo pull thirdparty/biscuit
git subrepo pull thirdparty/godot-dodo
git subrepo pull programs/gdscript-native/test_data
```

### Push changes back to upstream
```bash
git subrepo push thirdparty/stablehlo
```

### Remove a subrepo
```bash
git subrepo clean thirdparty/stablehlo
```

## GDScript Dataset (godot-dodo)

The GDScript test dataset is managed as a git subrepo at `thirdparty/godot-dodo/`. It contains the [godot-dodo](https://github.com/minosvasilias/godot-dodo) dataset with 60k+ GDScript samples.

The dataset JSON file is located at:
```
thirdparty/godot-dodo/data/godot_dodo_4x_60k/godot_dodo_4x_60k_data.json
```

Note: There is also a copy/symlink at `programs/gdscript-native/test_data/` for convenience.

## libriscv

[libriscv](https://github.com/fwsGonzo/libriscv) is a RISC-V emulator/sandbox library used for testing ELF binaries. It's the same library used by Godot sandbox internally.

### Setup

libriscv is a header-only library with optional CMake build. To use it:

```bash
# Update libriscv subrepo
git subrepo pull thirdparty/libriscv
```

### Usage

Include the headers:
```cpp
#include <libriscv/machine.hpp>
```

Link against libriscv (if using CMake build):
```cmake
find_package(riscv REQUIRED)
target_link_libraries(your_target riscv)
```

## Directory Structure

```
thirdparty/
├── stablehlo/          # StableHLO repository (git subrepo)
├── llvm-project/       # LLVM project repository (git subrepo)
├── godot-dodo/         # GDScript dataset (godot-dodo, git subrepo)
├── libriscv/           # RISC-V emulator library (git subrepo)
├── biscuit/            # RISC-V code generator (git subrepo)
└── llvm-build/         # LLVM build directory (not in git)

programs/gdscript-native/
└── test_data/          # GDScript dataset (symlink or copy of thirdparty/godot-dodo)
```

## CMake Configuration

When configuring your project, point to the built libraries:

```bash
cmake .. \
  -DMLIR_DIR=${PWD}/thirdparty/llvm-build/lib/cmake/mlir \
  -DSTABLEHLO_DIR=${PWD}/thirdparty/stablehlo/build/lib/cmake/stablehlo
```

