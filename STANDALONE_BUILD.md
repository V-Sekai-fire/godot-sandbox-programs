# Standalone Build Instructions

This document explains how to build the `scenetree_passthrough` JSON-RPC server for **x86_64 standalone mode** (without RISC-V dependencies).

## Overview

There are two modes:

| Mode | `EMBEDDED_RISCV` | Target | Dependencies |
|------|-----------------|--------|--------------|
| Embedded (RISC-V) | ON | `scenetree_passthrough` | Full RISC-V toolchain, `godot-sandbox` |
| Standalone (x86_64) | OFF | `jsonrpc_server` or `scenetree_passthrough_standalone` | Host compiler, mock handlers |

## Problem

The `godot-sandbox` repository's CMakeLists.txt unconditionally sets RISC-V compiler flags (`-mabi=lp64d`, `-march=rv64gc...`), even when NOT building for embedded RISC-V. This causes the host compiler (`g++`) to fail with "unrecognized argument" errors.

## Workaround 1: Use EMBEDDED_RISCV=ON for standalone

Even for standalone builds, use `EMBEDDED_RISCV=ON` which:
- Skips building `asm`, `gdscript`, `libtcc`, `luajit`, `mir`, `robust_skin_weight_transfer`
- Builds `jsonrpc_server` using `godot-sandbox` headers but with RISC-V flags disabled

```bash
cd godot-sandbox-programs

# This skips the problematic asm/gdscript/etc. subdirectories
# But still tries to build godot-sandbox with RISC-V flags
# Workaround: Build a patched version of godot-sandbox locally

# 1. Clone godot-sandbox repo
cd ..
git clone https://github.com/libriscv/godot-sandbox.git
cd godot-sandbox

# 2. Apply patch to fix standalone compilation
# See: godot-sandbox-programs/fix_standalone.patch

# 3. Build it (this creates libgodot-riscv.so)
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# This gives us the libgodot-riscv.so we need

# 4. Now build godot-sandbox-programs with the local godot-sandbox
cd ../../godot-sandbox-programs

# Configure to use our local build
# (Need to modify programs/CMakeLists.txt FetchContent to use local path)
# OR: Copy libgodot-riscv.so to the right location
```

## Workaround 2: Create a patched fork of godot-sandbox

1. Fork https://github.com/libriscv/godot-sandbox
2. Apply the patch from `standalone-fix.cmake` or manually:
   ```cmake
   # In program/cpp/cmake/CMakeLists.txt
   # Wrap RISCV_ABI setting with EMBEDDED_RISCV check:
   
   # Before:
   set(RISCV_ABI "-mabi=lp64d")
   
   # After:
   if(EMBEDDED_RISCV)
     set(RISCV_ABI "-mabi=lp64d")
   else()
     set(RISCV_ABI "")
   endif()
   ```
3. Update `programs/CMakeLists.txt` to use your fork:
   ```cmake
   FetchContent_Declare(
     godot-sandbox
     GIT_REPOSITORY https://github.com/YOUR_USER/godot-sandbox.git
     GIT_TAG your-feature-branch
   )
   ```

## Workaround 3: Use pre-built godot-sandbox

If someone else on your team has already built `libgodot-riscv.so`:

```bash
# Copy the pre-built library
cp /path/to/libgodot-riscv.so godot-sandbox-programs/build/lib/

# Now configure and build
cd godot-sandbox-programs
cmake -B build -DEMBEDDED_RISCV=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target jsonrpc_server
```

## Architecture

For standalone mode, the JSON-RPC server `jsonrpc_server` provides:
- SceneTree methods (via `get_tree().call()`)
- Node methods (via `node.call()`)
- Mock implementations that return JSON responses

All methods are resolved dynamically and passed through to the underlying Godot API (when available) or mocked (when not).

## Testing

```bash
# Test stdio mode
./build/bin/jsonrpc_server --stdio <<< '{"jsonrpc":"2.0","method":"list_methods","id":1}'

# Test TCP mode
./build/bin/jsonrpc_server --port 17777 &
curl -s http://localhost:17777 -d '{"jsonrpc":"2.0","method":"get_tree","id":1}'
kill %1
```

## Creating `scenetree_passthrough_standalone`

To create a proper standalone target:

```bash
# 1. Copy jsonrpc-server to scenetree_passthrough_standalone
cp -r programs/jsonrpc-server programs/scenetree_passthrough_standalone

# 2. Modify CMakeLists.txt
cat > programs/scenetree_passthrough_standalone/CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(scenetree_passthrough_standalone)

add_ci_program(scenetree_passthrough_standalone
    main.cpp
)
EOF

# 3. Create standalone main.cpp that doesn't use api.hpp
#    (This requires significant work to convert all handlers to mock implementations)
```

## Property-Based Testing with RapidCheck

For comprehensive property-based testing of the JSON-RPC API, we integrate [RapidCheck](https://github.com/emil-e/rapidcheck), a C++ framework for property-based testing inspired by QuickCheck.

### Setting Up RapidCheck

RapidCheck is included as a submodule in `programs/scenetree_passthrough_standalone/rapidcheck`:

```bash
cd programs/scenetree_passthrough_standalone
git submodule update --init --recursive
```

### Building Tests

To build the RapidCheck property tests:

```bash
cd godot-sandbox-programs
mkdir -p build && cd build
cmake .. -DBUILD_RAPIDCHECK_TESTS=ON
make -j$(nproc)
```

### Running Tests

Run all property tests:

```bash
ctest --output-on-failure
```

Or run tests manually:

```bash
./test/jsonrpc_compliance
```

### Available Property Tests

The test suite includes properties for:

1. **Request Structure** - Every valid request has required fields (`jsonrpc`, `method`)
2. **JSON-RPC Version** - Version must always be "2.0"
3. **Method Names** - Must be valid identifiers (alphanumeric + underscore)
4. **Parameter Types** - Can be arrays, objects, or omitted
5. **Error Responses** - Must have `code` and `message` fields
6. **ID Preservation** - IDs are preserved between request and response

### Writing New Property Tests

Add new test functions following the pattern:

```cpp
void myPropertyTest() {
    check(
        "Description of the property",
        []() {
            // Test logic using RapidCheck generators
            RC_ASSERT(condition);
        }
    );
}
```

Common generators available:
- `numeric<T>()` - Random numeric values
- `text()` - Random strings
- `element(vector)` - Random choice from a list
- `container(vector, generator)` - Random-sized container
- `map(map, keyGen, valueGen)` - Random key-value map

### Test Output Example

```
=== All RapidCheck property tests PASSED ===
```

If a property fails, RapidCheck will shrink the input to find the minimal counterexample.

## Future Work

The ideal solution is to:
1. Create a `main_standalone.cpp` that uses mock handlers
2. Add `STANDALONE` CMake option
3. Build `jsonrpc_server` by default, `scenetree_passthrough_standalone` when requested
4. Document the difference between the two targets

This requires modifying the upstream godot-sandbox repository or creating a fork.
