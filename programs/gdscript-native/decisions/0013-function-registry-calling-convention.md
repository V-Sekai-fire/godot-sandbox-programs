# RFC 0013: Function Registry and Calling Convention

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to implement a function registry system and C++ wrapper functions to bridge between generated RISC-V assembly code (which returns `int64_t`) and Godot's API (which expects `Variant`).

## Motivation

Generated RISC-V assembly functions follow standard RISC-V 64 Linux ABI:
- Return `int64_t` in register `a0`
- But Godot API expects `Variant(*)()` function signature
- Need to bridge between assembly (guest) and C++ (host)

## Detailed Design

### Function Registry

**File**: `function_registry.h/cpp`

**Purpose**: Track compiled function addresses by name

**Implementation**:
```cpp
class FunctionRegistry {
    std::unordered_map<std::string, void*> _functions;
    
public:
    void register_function(const std::string& name, void* address);
    void* get_function(const std::string& name) const;
};
```

### C++ Wrapper Functions

**File**: `main.cpp`

**Approach**: Create C++ lambda wrappers for each compiled function

**Flow**:
```
GDScript → Callable → C++ Wrapper → Generated Assembly → int64_t → Variant → GDScript
```

**Implementation**:
```cpp
// Helper to call assembly and get int64_t result
int64_t call_assembly_function(void* func_ptr);

// Create wrapper that converts int64_t → Variant
auto wrapper = [func_ptr]() -> Variant {
    int64_t result = call_assembly_function(func_ptr);
    return Variant(static_cast<int>(result));
};

// Register with Godot API
ADD_API_FUNCTION(wrapper, ...);
```

## Benefits

1. **ABI Compatibility**: Bridges RISC-V ABI to Godot API
2. **Type Conversion**: Handles int64_t → Variant conversion
3. **Function Lookup**: Registry enables function calls from generated code
4. **Future-Proof**: Ready for inter-function calls

## Implementation Status

✅ **Completed**:
- `FunctionRegistry` class
- `call_assembly_function()` helper
- C++ wrapper generation
- int64_t → Variant conversion
- Function registration

## Future Considerations

- For functions with parameters: May need syscall/helper approach
- For complex return types: May need direct Variant construction in assembly
- Inter-function calls: Function registry ready for this

## Alternatives Considered

### Alternative 1: Direct Variant Return in Assembly
- **Rejected**: Too complex, Variant is C++ class, not simple type

### Alternative 2: Use Godot's Internal Calling Convention
- **Rejected**: Want independence, standard RISC-V ABI is cleaner

### Alternative 3: No Wrapper, Direct Registration
- **Rejected**: Type mismatch (int64_t vs Variant)

## Unresolved Questions

- [ ] How to handle function parameters in wrappers?
- [ ] Should we support complex return types?
- [ ] How to handle inter-function calls?

## References

- [RFC 0004: Dual-Mode Architecture](./0004-dual-mode-architecture.md)

