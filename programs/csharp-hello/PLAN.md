# C# RISC-V Godot Sandbox Program - Implementation Plan

## Overview
Create a C# program that can be AOT-compiled to RISC-V Linux 64-bit .elf format, interacts with the Godot Sandbox API (similar to the C++ examples), and integrates with the existing CMake build system.

## Implementation Steps

### 1. Dev Container Setup
- Create `.devcontainer/devcontainer.json` with AMD64 Linux base image
- Install RISC-V cross-compilation toolchain (`riscv64-linux-gnu-gcc`)
- Download and extract the RISC-V .NET SDK (10.0.100-rtm.25561.199) to `/opt/dotnet-riscv`
- Set up environment variables for .NET RISC-V toolchain

### 2. C# Program Structure
- Create `programs/csharp-hello/` directory
- Create `Program.cs` with minimal C# code that:
  - Uses P/Invoke to call Godot Sandbox C API functions
  - Registers API functions using `add_api_function` equivalent
  - Calls `halt()` at the end
  - Demonstrates system calls (e.g., `write` syscall or file I/O)

### 3. C ABI Bindings
- Research or create C function signatures for Sandbox API:
  - `print(string)` - print to console
  - `halt()` - halt execution
  - `add_api_function(name, return_type, params, description)` - register API function
  - `get_sandbox_node()` - get Sandbox node reference
- Create `NativeMethods.cs` with P/Invoke declarations
- Map C++ types to C# types (e.g., `Variant` handling, `String` conversion)

### 4. AOT Compilation Configuration
- Create `.csproj` file configured for:
  - Native AOT compilation (`PublishAot=true`)
  - RISC-V Linux target (`linux-riscv64`)
  - Self-contained deployment
  - Trimming enabled
- Configure `Directory.Build.props` if needed for common settings

### 5. CMake Integration
- Create `programs/csharp-hello/CMakeLists.txt` that:
  - Detects .NET SDK location
  - Runs `dotnet publish` with AOT flags for RISC-V
  - Outputs `.elf` file to build directory
  - Integrates with existing `add_ci_program` pattern

### 6. Build Script Updates
- Update `build.sh` to handle C# programs (or create separate build script)
- Ensure RISC-V .NET SDK is available in PATH

### 7. Testing & Documentation
- Create `README.md` explaining the C# program
- Document P/Invoke requirements and C ABI considerations
- Test that the compiled .elf works in Godot Sandbox

## Key Files to Create/Modify
- `.devcontainer/devcontainer.json` - Dev container configuration
- `programs/csharp-hello/Program.cs` - Main C# program
- `programs/csharp-hello/NativeMethods.cs` - P/Invoke declarations
- `programs/csharp-hello/csharp-hello.csproj` - .NET project file
- `programs/csharp-hello/CMakeLists.txt` - CMake build integration
- `programs/csharp-hello/README.md` - Documentation

## Challenges to Address
1. **C ABI Mapping**: Need to determine exact C function signatures from C++ `api.hpp` wrapper
2. **Variant Type**: Godot's `Variant` type needs proper marshalling between C and C#
3. **String Handling**: UTF-8 string conversion between C# and C
4. **RISC-V AOT**: Ensure .NET Native AOT supports RISC-V Linux 64-bit target
5. **System Calls**: Properly invoke Linux syscalls from C# (may need `DllImport` for libc functions)

## Research Needed
- Examine godot-sandbox repository for C API headers (if they exist)
- Verify RISC-V .NET SDK AOT support and required flags
- Understand RISC-V Linux 64-bit calling conventions for P/Invoke

## Implementation Todos

1. **devcontainer**: Create .devcontainer/devcontainer.json with AMD64 Linux, RISC-V toolchain, and .NET SDK setup
2. **csharp_structure**: Create programs/csharp-hello directory structure with Program.cs and basic project files
3. **c_abi_research**: Research or create C function signatures for Godot Sandbox API (print, halt, add_api_function, etc.)
4. **pinvoke_bindings**: Create NativeMethods.cs with P/Invoke declarations for Sandbox API functions (depends on c_abi_research)
5. **aot_config**: Configure .csproj for Native AOT compilation targeting linux-riscv64 (depends on csharp_structure)
6. **cmake_integration**: Create CMakeLists.txt that builds C# program using dotnet publish with AOT flags (depends on aot_config and pinvoke_bindings)
7. **build_testing**: Test the build process in dev container and verify .elf output (depends on cmake_integration and devcontainer)
8. **documentation**: Create README.md documenting the C# program, P/Invoke setup, and usage (depends on build_testing)

## RISC-V .NET SDK Reference
- Download URL: https://github.com/filipnavara/dotnet-riscv/releases/download/10.0.100-rtm.25561.199/dotnet-sdk-10.0.100-rtm.25561.199-linux-riscv64.tar.gz
- Version: 10.0.100-rtm.25561.199
- Target: linux-riscv64

