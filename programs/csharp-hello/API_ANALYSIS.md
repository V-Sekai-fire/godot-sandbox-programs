# Godot Sandbox API Analysis

## Key Findings

### 1. API Structure
- Programs use `#include <api.hpp>` - a C++ header file
- The API provides C++ functions and macros:
  - `print()` - prints to console (variadic, similar to printf)
  - `halt()` - halts program execution
  - `ADD_API_FUNCTION(func, return_type, params, description)` - macro to register API functions
  - `get_node<Sandbox>()` - C++ template function to get Sandbox node
  - `add_property(name, type, default, getter, setter)` - adds a property
  - `Variant` - Godot's variant type (C++ class)

### 2. Syscalls
- `EXTERN_SYSCALL` macro is used to declare syscalls provided by the sandbox runtime
- Common syscalls:
  - `sys_vfetch(unsigned, void *, int)` - fetch variant data
  - `sys_vstore(unsigned *, int, const void *, unsigned)` - store variant data
  - `sys_print` - print function (used in mir example)

### 3. Program Structure
- Programs have a `main()` function (C++ entry point)
- They register API functions using `ADD_API_FUNCTION`
- They call `halt()` at the end to stop execution
- Functions return `Variant` type (Godot's universal type)

### 4. C# Integration Strategy (Pure C# Approach)

**Pure C# Implementation:**
- No C++ wrappers - use P/Invoke directly
- Call libc functions (printf, write) for I/O
- Use direct syscalls where needed
- For Sandbox API functions, they would need to be:
  - Exported as C functions (extern "C"), or
  - Called via C++ name mangling (fragile), or
  - Available through a C-compatible runtime library

**Current Implementation:**
- Uses libc functions (printf, write) for output
- Demonstrates direct syscalls
- Uses `UnmanagedCallersOnly` for functions callable from native code
- Sandbox API integration (print, halt, ADD_API_FUNCTION) requires C-compatible exports

**Variant Handling:**
- Variant is a C++ class, needs to be handled carefully
- May need to pass as pointer or use a C struct representation
- String conversion: C# string -> UTF-8 byte array -> C string
- Full Variant support would require marshalling layer

### 5. Build Process
- Programs are compiled as RISC-V ELF executables
- They're loaded dynamically by the Godot Sandbox
- The sandbox provides the runtime environment and syscalls

### 6. C# AOT Considerations
- .NET Native AOT for RISC-V Linux 64-bit
- P/Invoke requires proper calling conventions
- String marshalling (UTF-8)
- Function pointer handling for callbacks

