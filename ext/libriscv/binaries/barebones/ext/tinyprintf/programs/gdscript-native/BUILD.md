# Building GDScript Native with StableHLO (Git Subrepo)

This guide explains how to build the GDScript to RISC-V compiler using StableHLO managed via git subrepo.

## Prerequisites

1. **Git Subrepo**: Install git-subrepo if not already installed
   ```bash
   # macOS
   brew install git-subrepo
   
   # Or install manually from https://github.com/ingydotnet/git-subrepo
   ```

2. **Build Tools**: CMake, Ninja, and a C++ compiler
   ```bash
   # macOS
   brew install cmake ninja
   
   # Linux
   sudo apt install cmake ninja-build
   ```

## Setup Git Subrepos

### Step 1: Commit Your Changes

Git subrepo requires a clean working directory:

```bash
git add .
git commit -m "Add StableHLO integration"
```

### Step 2: Run Setup Script

```bash
./setup-stablehlo-subrepo.sh
```

This will:
- Clone StableHLO into `thirdparty/stablehlo/`
- Clone LLVM project into `thirdparty/llvm-project/`

### Step 3: Build MLIR

StableHLO depends on MLIR, which must be built from LLVM:

```bash
cd thirdparty/stablehlo

# Check out the correct LLVM commit
(hash="$(cat ./build_tools/llvm_version.txt)"; cd ../llvm-project && git fetch origin "$hash" && git checkout "$hash")

# Build MLIR (this takes 10-20 minutes)
MLIR_ENABLE_BINDINGS_PYTHON=OFF build_tools/build_mlir.sh "${PWD}"/../llvm-project/ "${PWD}"/../llvm-build
```

### Step 4: Build StableHLO

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

## Building the Project

Once StableHLO is built, configure and build the project:

```bash
cd build
cmake .. -GNinja
cmake --build .
```

The CMakeLists.txt will automatically find MLIR and StableHLO in the `thirdparty/` directory.

## Updating Subrepos

To update StableHLO to the latest version:

```bash
git subrepo pull thirdparty/stablehlo
```

Then rebuild MLIR and StableHLO as needed.

## Troubleshooting

### MLIR_DIR not found

If CMake can't find MLIR, ensure you've built it:
```bash
ls thirdparty/llvm-build/lib/cmake/mlir
```

### StableHLO_DIR not found

If CMake can't find StableHLO, ensure you've built it:
```bash
ls thirdparty/stablehlo/build/lib/cmake/stablehlo
```

### Manual Path Override

You can override the default paths:
```bash
cmake .. \
  -DMLIR_DIR=/custom/path/to/mlir \
  -DSTABLEHLO_DIR=/custom/path/to/stablehlo
```

## Directory Structure

```
godot-sandbox-programs/
├── thirdparty/
│   ├── stablehlo/          # StableHLO git subrepo
│   │   ├── build/          # StableHLO build (not in git)
│   │   └── ...
│   ├── llvm-project/       # LLVM git subrepo
│   │   └── ...
│   └── llvm-build/         # LLVM build (not in git)
├── programs/
│   └── gdscript-native/
│       └── ...
└── ...
```

