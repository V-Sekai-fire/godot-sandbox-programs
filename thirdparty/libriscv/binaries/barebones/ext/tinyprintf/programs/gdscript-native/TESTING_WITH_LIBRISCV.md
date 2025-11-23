# Testing ELF Binaries with libriscv

This document explains how to test generated RISC-V ELF binaries using `libriscv`, the same library used by Godot sandbox.

## libriscv Setup

`libriscv` is managed as a git subrepo in `thirdparty/libriscv/`.

### Update libriscv

```bash
git subrepo pull thirdparty/libriscv
```

### Build libriscv

```bash
cd thirdparty/libriscv/lib
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install  # Optional: install system-wide
```

## Testing with libriscv

### Method 1: Direct C++ Test

The `test_libriscv.cpp` program loads and executes an ELF file:

```bash
# Build the test program
cd programs/gdscript-native
g++ -std=c++20 -I../../thirdparty/libriscv/lib \
    -o test_libriscv test_libriscv.cpp -lriscv

# Run test
./test_libriscv test_output/simple.elf
```

### Method 2: Docker Test

Build and run the Docker container:

```bash
cd programs/gdscript-native
docker build -f Dockerfile.libriscv -t test-libriscv .
docker run --rm test-libriscv
```

The Dockerfile:
- Copies `thirdparty/libriscv/` from the git subrepo
- Builds libriscv using CMake
- Compiles `test_libriscv.cpp`
- Executes the test with `simple.elf`

## How libriscv Works (Same as Godot Sandbox)

`libriscv` follows the same pattern as Godot sandbox:

1. **Load ELF**: Create a `Machine<RISCV64>` from binary data
   ```cpp
   std::string_view binary_view{...};
   Machine<RISCV64> machine{binary_view, options};
   ```

2. **Setup Linux Environment**: Configure syscalls and environment
   ```cpp
   machine.setup_linux({"program_name"});
   machine.setup_linux_syscalls();
   ```

3. **Execute**: Run the program
   ```cpp
   machine.simulate();  // Run until exit
   ```

4. **Get Results**: Read exit code from register `a0`
   ```cpp
   int64_t exit_code = machine.cpu.reg(10);  // a0 register
   ```

## Comparison: libriscv vs Godot Sandbox

| Feature | libriscv | Godot Sandbox |
|---------|----------|---------------|
| Library | Direct C++ library | Wraps libriscv |
| API | `Machine<RISCV64>` | `Sandbox` class |
| Function Calls | `machine.vmcall("func")` | `sandbox.vmcall("func")` |
| Execution | `machine.simulate()` | `sandbox.vmcall()` |
| Use Case | Testing, CLI tools | Godot integration |

Both use the same underlying `libriscv` library, so testing with `libriscv` directly validates that ELF files will work in Godot sandbox.

## Expected Output

When testing `simple.elf` (which returns 42):

```
=== Testing ELF with libriscv ===
Loaded ELF file: test_output/simple.elf (1234 bytes)
Machine created successfully
Entry point: 0x10000
Linux environment setup complete
Starting execution...

=== Results ===
Program exited with code: 42
Instructions executed: 12345
✅ SUCCESS: Program returned 42 as expected!
```

## Troubleshooting

### libriscv not found

If `-lriscv` fails, build libriscv first:
```bash
cd thirdparty/libriscv/lib
mkdir build && cd build
cmake .. && make
# Then link with: -L/path/to/libriscv/lib/build -lriscv
```

### Include path issues

Make sure to include the correct path:
```cpp
#include <libriscv/machine.hpp>  // From thirdparty/libriscv/lib/libriscv/
```

Use compile flag:
```bash
-I../../thirdparty/libriscv/lib
```

## References

- [libriscv GitHub](https://github.com/fwsGonzo/libriscv)
- [libriscv Documentation](https://github.com/fwsGonzo/libriscv/tree/master/docs)
- Godot Sandbox source: `build/_deps/godot-sandbox-src/src/sandbox.cpp`

