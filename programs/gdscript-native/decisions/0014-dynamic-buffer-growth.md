# RFC 0014: Dynamic Buffer Growth Strategy for Code Generation

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to implement dynamic buffer growth for code generation, growing the buffer when it reaches 90% capacity rather than using a fixed-size buffer.

## Motivation

Initial implementation used a fixed 8KB buffer for generated code:

- **Problem**: Fixed buffer may be too small for complex functions or too large for simple ones
- **Solution**: Dynamic growth strategy that grows buffer when 90% full

## Detailed Design

### Buffer Growth Strategy

**File**: `ast_to_riscv_biscuit.cpp`

**Implementation**:
```cpp
// Check if buffer is nearly full (90% threshold)
if (_code_buffer.size() - _assembler->GetBuffer()->GetOffset() 
    < _code_buffer.size() * BUFFER_GROWTH_THRESHOLD) {
    // Grow buffer
    size_t new_size = _code_buffer.size() * 2;
    _code_buffer.resize(new_size);
    // Reinitialize assembler with new buffer
}
```

**Constants**:
- `INITIAL_CODE_BUFFER_SIZE = 8192` (8KB initial size)
- `BUFFER_GROWTH_THRESHOLD = 0.9` (grow when 90% full)
- Growth factor: 2x (double size each time)

### Growth Algorithm

1. **Initial Allocation**: 8KB buffer
2. **Monitor Usage**: Track buffer usage during code generation
3. **Growth Trigger**: When buffer is 90% full
4. **Growth Action**: Double buffer size, reinitialize assembler
5. **Repeat**: Continue until code generation complete

## Benefits

1. **Flexibility**: Handles both small and large functions
2. **Memory Efficiency**: Starts small, grows as needed
3. **No Fixed Limits**: Can handle arbitrarily large code
4. **Performance**: Only grows when necessary (90% threshold)

## Implementation Status

✅ **Completed**:
- Dynamic buffer growth
- 90% threshold check
- Buffer doubling strategy
- Assembler reinitialization

## Comparison

| Aspect | Fixed Buffer | Dynamic Growth |
|--------|-------------|----------------|
| **Memory Usage** | Fixed (wasteful) | Adaptive (efficient) |
| **Limitations** | Fixed max size | No hard limit |
| **Complexity** | Simple | Moderate |
| **Performance** | Predictable | Slight overhead on growth |

## Alternatives Considered

### Alternative 1: Fixed Large Buffer
- **Rejected**: Wasteful for small functions

### Alternative 2: Pre-calculate Size
- **Rejected**: Hard to predict, would require two-pass codegen

### Alternative 3: Multiple Allocations
- **Rejected**: More complex, current approach simpler

## Unresolved Questions

- [ ] Should we add maximum buffer size limit?
- [ ] How to optimize growth factor (2x vs 1.5x)?
- [ ] Should we add buffer size statistics?

## References

- [RFC 0011: Constants Centralization](./0011-constants-centralization.md)

