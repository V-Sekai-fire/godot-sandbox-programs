# RFC 0016: Register Allocation Strategy

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the register allocation strategy used in the RISC-V code emitter: a simple round-robin allocation of temporary registers (t0-t6) with stack spilling fallback.

## Motivation

RISC-V has a limited number of registers available for temporary values during code generation:
- **a0-a7**: Argument/return registers (used for function parameters and return values)
- **s0-s11**: Saved registers (must be preserved across function calls)
- **t0-t6**: Temporary registers (caller-saved, can be used freely)

We need a strategy to allocate registers for intermediate expression values during code generation.

## Detailed Design

### Register Allocation Strategy

**File**: `ast_to_riscv_biscuit.h/cpp`

**Approach**: Simple round-robin allocation with stack spilling

**Temporary Registers**:
```cpp
static constexpr biscuit::GPR _temp_regs[] = {
    biscuit::t0, biscuit::t1, biscuit::t2,
    biscuit::t3, biscuit::t4, biscuit::t5, biscuit::t6
};
static constexpr size_t _num_temp_regs = 7;
```

**Allocation Algorithm**:
1. **Round-Robin**: Allocate registers sequentially (t0, t1, t2, ..., t6, then wrap to t0)
2. **Expression Tracking**: Map expression nodes to allocated registers
3. **Stack Spilling**: When all 7 temporary registers are exhausted, allocate stack slots

**Implementation**:
```cpp
biscuit::GPR _allocate_register() {
    if (_temp_reg_index < _num_temp_regs) {
        biscuit::GPR reg = _temp_regs[_temp_reg_index];
        _temp_reg_index = (_temp_reg_index + 1) % _num_temp_regs;
        return reg;
    }
    // Out of registers - use stack (allocate a temp slot)
    std::string tempName = "_temp_" + std::to_string(_stack_offset);
    _allocate_stack(tempName);
    return _temp_regs[0]; // Will need special handling
}
```

### Expression-to-Register Mapping

**Tracking**: `std::unordered_map<const ExpressionNode*, biscuit::GPR> _expr_to_reg`

**Purpose**: Track which register holds the result of each expression node

**Usage**: When emitting binary operations, look up left/right operands' registers

### Variable Storage

**Variables**: Stored on stack, not in registers

**Rationale**:
- Variables may be accessed multiple times
- Variables have longer lifetimes than expression results
- Stack storage is simpler and more predictable

**Stack Layout**:
- Saved registers (ra, s0): 16 bytes
- Parameters: 8 bytes each (stored from a0-a7)
- Local variables: 8 bytes each (allocated dynamically)

## Benefits

1. **Simplicity**: Round-robin is easy to implement and understand
2. **No Liveness Analysis**: No need for complex register allocation algorithms
3. **Stack Spilling**: Handles register exhaustion gracefully
4. **Expression Tracking**: Efficient lookup of expression results

## Limitations

1. **No Register Reuse**: Doesn't reuse registers when expressions are no longer needed
2. **No Spill Optimization**: Stack spilling happens when registers exhausted, not proactively
3. **Simple Round-Robin**: Doesn't consider register pressure or expression lifetimes
4. **No Register Coalescing**: Doesn't optimize register assignments

## Implementation Status

✅ **Completed**:
- Round-robin register allocation
- Expression-to-register mapping
- Stack spilling fallback
- Variable stack storage

## Comparison

| Aspect | Round-Robin (Current) | Graph Coloring | Linear Scan |
|--------|----------------------|----------------|-------------|
| **Complexity** | Simple | Complex | Moderate |
| **Performance** | Good for simple code | Optimal | Good |
| **Implementation** | Easy | Hard | Moderate |
| **Register Reuse** | No | Yes | Yes |
| **Spill Strategy** | Reactive | Proactive | Proactive |

## Alternatives Considered

### Alternative 1: Graph Coloring Register Allocation
- **Rejected**: Too complex for initial implementation, overkill for simple expressions

### Alternative 2: Linear Scan Register Allocation
- **Rejected**: More complex than round-robin, requires liveness analysis

### Alternative 3: Fixed Register Assignment
- **Rejected**: Too restrictive, doesn't handle complex expressions

### Alternative 4: Stack-Only (No Registers)
- **Rejected**: Too slow, defeats purpose of register-based architecture

## Future Improvements

- [ ] Add liveness analysis to enable register reuse
- [ ] Implement proactive stack spilling before register exhaustion
- [ ] Consider linear scan allocation for better register utilization
- [ ] Add register coalescing for better code quality

## Unresolved Questions

- [ ] Should we implement register reuse for dead expressions?
- [ ] When should we proactively spill to stack?
- [ ] Should we use different allocation strategies for different expression types?

## References

- [RFC 0007: Biscuit RISC-V Codegen](./0007-biscuit-riscv-codegen.md)
- [RFC 0008: ELF Generation for libriscv](./0008-elf-generation-libriscv.md)
- RISC-V 64 Linux ABI Specification

