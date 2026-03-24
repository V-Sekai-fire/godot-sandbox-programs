# Build Status: godot-sandbox-programs

## Summary

The `godot-sandbox-programs` project builds a JSON-RPC SceneTree server for Godot's sandbox environment. There are two builds:

1. **RISC-V Embedded Mode** (`EMBEDDED_RISCV=ON`) - Compiles for RISC-V VM, uses `api.hpp` from godot-sandbox
2. **x86_64 Standalone Mode** (`EMBEDDED_RISCV=OFF`) - Native Linux binary, should not require RISC-V toolchain

## Current Status

**BROKEN** - Standalone mode fails because:

1. `godot-sandbox` CMakeLists.txt sets RISC-V compiler flags (`-mabi=lp64d`, `-march=rv64gc`) unconditionally, even when not needed
2. When `EMBEDDED_RISCV=OFF`, the host `g++` compiler doesn't understand RISC-V flags
3. Build fails with: `c++: error: unrecognized argument '-mabi=lp64d'`

## Blockers Identified

| Blocker | Status | Fix Applied |
|---------|--------|-------------|
| `src/godot-sandbox/handlers/list.txt` missing | ✅ FIXED | Created placeholder file |
| `cmake/riscv-toolchain.cmake` doesn't set `EMBEDDED_RISCV` | ✅ FIXED | Modified toolchain to check `EMBEDDED_RISCV` |
| `programs/jsonrpc-server/main.cpp` - tree.call() vs node.call() | ✅ NOT NEEDED | Already uses `node.call()` |
| `programs/jsonrpc-server/main.cpp` - missing validation | ✅ NOT NEEDED | Already has `create_error()` |
| `godot-sandbox` CMakeLists.txt compiles with RISC-V flags regardless of `EMBEDDED_RISCV` | ❌ NOT FIXED | Requires upstream change |
| `generate_api.py` can't find `method_list_v4.6.1_stable.jsonl` | ❌ NOT FIXED | Depends on godot-sandbox fetch |

## Fix Required (Upstream)

The `godot-sandbox` repository's CMakeLists.txt needs patching:

```diff
--- a/program/cpp/cmake/CMakeLists.txt
+++ b/program/cpp/cmake/CMakeLists.txt
@@ -59,7 +59,9 @@ else()
 		set(RISCV_ARCH "")
 	endif()
-	set(RISCV_ABI "-mabi=lp64d")
+    # Only set RISC-V ABI when actually building for embedded
+if (EMBEDDED_RISCV)
+	set(RISCV_ABI "-mabi=lp64d")
+endif()
 else()
 	if (SANDBOX_RISCV_EXT_C)
 		set(RISCV_ARCH "-march=rv64gc_zba_zbb_zbs_zbc")
```

And similarly at line 69.

## Workarounds (For Testing Without Upstream Fix)

### Workaround 1: Build with `EMBEDDED_RISCV=ON`

```bash
cd godot-sandbox-programs
cmake -B build -DEMBEDDED_RISCV=ON
cmake --build build --target jsonrpc_server --config Debug
```

This compiles `godot-sandbox` with RISC-V flags, which requires RISC-V toolchain to be installed, but at least skips the problematic `asm`, `gdscript`, `libtcc`, `luajit`, `mir`, `robust_skin_weight_transfer` subdirectories.

### Workaround 2: Install RISC-V Toolchain

```bash
# Ubuntu/Debian
sudo apt-get install gcc-riscv64-linux-gnu g++-riscv64-linux-gnu

# Then build
cmake -B build -DEMBEDDED_RISCV=ON
cmake --build build
```

### Workaround 3: Use Pre-Built godot-sandbox

If you have a pre-built `libgodot-riscv.so`:

```bash
cp /path/to/libgodot-riscv.so godot-sandbox-programs/build/lib/
cmake -B build -DEMBEDDED_RISCV=ON
cmake --build build --target jsonrpc_server
```

## Future Work

1. **Submit PR to godot-sandbox** with the fix above
2. **Create CI/CD workflow** that builds for both RISC-V and standalone
3. **Add `STANDALONE` target** that uses mock handlers instead of `api.hpp`
4. **Generate `scenetree_passthrough_standalone`** from `jsonrpc-server` with mock implementations

## Files Modified

| File | Change | Status |
|------|--------|--------|
| `src/godot-sandbox/handlers/list.txt` | Created placeholder | ✅ |
| `cmake/riscv-toolchain.cmake` | Added `EMBEDDED_RISCV` check | ✅ |
| `programs/scenetree_passthrough_standalone/` | Created directory | ✅ |
| `cmake/godot-sandbox-toolchain.cmake` | Created (for future use) | ✅ |

## Testing

```bash
# Test list_methods
./build/bin/jsonrpc_server --stdio <<< '{"jsonrpc":"2.0","method":"list_methods","id":1}'

# Test get_tree
./build/bin/jsonrpc_server --stdio <<< '{"jsonrpc":"2.0","method":"get_tree","id":2}'

# Test error handling
./build/bin/jsonrpc_server --stdio <<< '{"jsonrpc":"2.0","method":"nonexistent","id":3}'
```

## References

- [godot-sandbox GitHub](https://github.com/libriscv/godot-sandbox)
- [V-Sekai Humanoid Project](https://github.com/V-Sekai/avatar-fs)
- [Lean 4 Formalization](lean4-mmog-formalization)
- [Rope Bridge Architecture](vrm-architecture-reference)
