# C# Hello World for Godot Sandbox

A minimal C# program compiled to RISC-V Linux 64-bit ELF format for use with the [Godot Sandbox](https://github.com/libriscv/godot-sandbox).

## Overview

This program demonstrates how to create a pure C# program that:
- Compiles to RISC-V Linux 64-bit using .NET Native AOT
- Uses P/Invoke to call system functions (libc)
- Demonstrates direct syscalls from C#
- Can be loaded and executed by the Godot Sandbox

## Requirements

- RISC-V .NET SDK (10.0.100-rtm.25561.199 or later)
  - Download from: https://github.com/filipnavara/dotnet-riscv/releases
  - Extract to `/opt/dotnet-riscv` or set `DOTNET_ROOT` environment variable
- RISC-V cross-compilation toolchain (`riscv64-linux-gnu-gcc-12`)
- CMake 3.10 or later

## Building

The program is built using CMake, which calls `dotnet publish` with Native AOT compilation:

```bash
# From the project root
mkdir -p .build
cd .build
cmake .. -DCMAKE_BUILD_TYPE=Release
make csharp_hello
```

The compiled ELF executable will be in `.build/bin/csharp-hello`.

## Architecture

### Pure C# Implementation

This program is implemented entirely in C# without C++ wrappers:

1. **System Calls**: Uses P/Invoke to call libc functions (`write`, `printf`) and direct syscalls
2. **Unmanaged Interop**: Uses `UnmanagedCallersOnly` for functions that can be called from native code
3. **Native AOT**: Compiled to native RISC-V code using .NET Native AOT

### Key Components

- `Program.cs`: Main program with P/Invoke declarations and example functions
- `csharp-hello.csproj`: .NET project file configured for Native AOT
- `CMakeLists.txt`: Build integration with the existing CMake system

## P/Invoke and System Calls

The program demonstrates several interop patterns:

1. **Direct Syscalls**: Uses Linux syscall interface directly
   ```csharp
   syscall(64, 1, buf, count); // write syscall
   ```

2. **Libc Functions**: Calls standard C library functions
   ```csharp
   write(1, buf, count); // stdout
   printf(format);
   ```

3. **UnmanagedCallersOnly**: Functions that can be called from native code
   ```csharp
   [UnmanagedCallersOnly]
   public static long Fibonacci(int n) { ... }
   ```

## Sandbox API Integration

**Note**: Direct integration with the Godot Sandbox C++ API (`print()`, `halt()`, `ADD_API_FUNCTION`) requires C-compatible bindings. The current implementation uses standard libc functions for output.

To fully integrate with the Sandbox API, you would need:
- C-compatible function exports from the sandbox runtime, or
- Knowledge of C++ name mangling for the specific functions, or
- A C wrapper library (but this would violate the "pure C#" requirement)

## Usage in Godot Sandbox

Once built, the `csharp-hello` ELF file can be loaded by a Godot Sandbox node:

```gdscript
var sandbox = $Sandbox
var program = sandbox.download_program("csharp-hello")
# The program will execute its Main() function
```

## Limitations

1. **Sandbox API**: Direct calls to Sandbox API functions (`print()`, `halt()`, etc.) are not yet implemented as they require C++ linkage. The program uses libc alternatives.

2. **Variant Handling**: Godot's `Variant` type is a C++ class. Full integration would require marshalling between C# types and Variant.

3. **Function Registration**: The `ADD_API_FUNCTION` macro is a C++ compile-time feature. Runtime registration would require additional infrastructure.

## Future Improvements

- Investigate C-compatible exports from godot-sandbox
- Implement Variant marshalling for full API integration
- Add runtime function registration mechanism
- Explore using `UnmanagedCallersOnly` callbacks for API functions

## See Also

- [Godot Sandbox Documentation](https://libriscv.no/docs/host_langs/godot_integration/godot_docs/)
- [.NET Native AOT](https://learn.microsoft.com/en-us/dotnet/core/deploying/native-aot/)
- [RISC-V .NET](https://github.com/filipnavara/dotnet-riscv)

