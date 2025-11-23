# RFC 0011: Centralized Constants for Magic Number Elimination

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to centralize all magic numbers as named constants in a single header file, improving code maintainability and readability.

## Motivation

Initial implementation had magic numbers scattered throughout the codebase:

- **Problem**: Hard-coded values (0x10000, 0x1000, 8, 16, etc.) throughout code
- **Solution**: Extract to named constants in `constants.h`

## Detailed Design

### Constants File

**File**: `constants.h`

**Categories**:

**ELF Generation Constants**:
- `ELF_ENTRY_POINT = 0x10000` - Virtual address for code segment
- `PAGE_SIZE = 0x1000` - 4KB page alignment
- `ELF_HEADER_SIZE = 64` - ELF header size
- `PROGRAM_HEADER_SIZE = 56` - Program header size
- `ELF_CODE_OFFSET = 0x78` - Offset where code starts

**Code Generation Constants**:
- `INITIAL_CODE_BUFFER_SIZE = 8192` - Initial buffer size (8KB)
- `ESTIMATED_LOCAL_VARS_SIZE = 64` - Stack space for locals
- `BYTES_PER_PARAMETER = 8` - RISC-V 64-bit parameter size
- `SAVED_REGISTERS_SIZE = 16` - ra (8) + s0 (8)
- `MAX_ARGUMENT_REGISTERS = 8` - a0-a7
- `BUFFER_GROWTH_THRESHOLD = 0.9` - Grow when 90% full

**ELF Section Constants**:
- `NUM_ELF_SECTIONS = 3` - Null, .text, .shstrtab
- `SHSTRTAB_SECTION_SIZE = 16` - String table size

### Usage

**Before**:
```cpp
_assembler->SD(biscuit::ra, stackSize - 8, biscuit::sp);
_assembler->SD(biscuit::s0, stackSize - 16, biscuit::sp);
```

**After**:
```cpp
using namespace constants;
_assembler->SD(biscuit::ra, stackSize - 8, biscuit::sp);
_assembler->SD(biscuit::s0, stackSize - SAVED_REGISTERS_SIZE, biscuit::sp);
```

## Benefits

1. **Maintainability**: Change value in one place
2. **Readability**: Named constants are self-documenting
3. **Type Safety**: `constexpr` ensures compile-time constants
4. **Consistency**: Same values used everywhere
5. **Documentation**: Constants serve as documentation

## Implementation Status

✅ **Completed**:
- `constants.h` created
- All magic numbers extracted
- `constexpr` used for compile-time constants
- Integrated throughout codebase

## Comparison

| Aspect | Magic Numbers | Named Constants |
|--------|--------------|-----------------|
| **Maintainability** | Change in many places | Change in one place |
| **Readability** | Unclear meaning | Self-documenting |
| **Type Safety** | None | `constexpr` |
| **Documentation** | None | Constants explain values |

## Alternatives Considered

### Alternative 1: Configuration File
- **Rejected**: These are compile-time constants, not runtime config

### Alternative 2: Class Constants
- **Rejected**: Namespace constants are simpler, no class needed

### Alternative 3: Keep Magic Numbers
- **Rejected**: Poor maintainability, unclear code

## Unresolved Questions

- [ ] Should we add more constants as code grows?
- [ ] Should we group constants by feature area?
- [ ] Should we add documentation comments for each constant?

## References

- [RFC 0008: ELF Generation](./0008-elf-generation-libriscv.md)
- [RFC 0007: Biscuit RISC-V Codegen](./0007-biscuit-riscv-codegen.md)

