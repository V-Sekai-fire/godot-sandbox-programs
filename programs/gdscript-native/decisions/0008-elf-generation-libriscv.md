# RFC 0008: ELF Generation for libriscv Compatibility

**Status**: Accepted  
**Created**: 2025-11-23  
**Authors**: Development Team

## Summary

This RFC documents the decision to generate RISC-V 64 Linux ELF files with specific alignment requirements for compatibility with the libriscv emulator library (same library used by Godot sandbox).

## Motivation

Generated RISC-V machine code needs to be packaged as ELF files for execution. The libriscv library (used by Godot sandbox) has specific requirements for ELF structure:

- **Critical Requirement**: `sh_addr - p_vaddr == sh_offset - p_offset`
- This ensures offset from segment base in memory equals offset from segment start in file
- Required by libriscv's `serialize_execute_segment()` function

## Detailed Design

### ELF Structure

**File**: `elf_generator.h/cpp`

**Key Constants** (from `constants.h`):
- `ELF_ENTRY_POINT = 0x10000` - Virtual address for code segment
- `PAGE_SIZE = 0x1000` - 4KB page alignment
- `ELF_CODE_OFFSET = 0x78` - Offset where code starts in file

**ELF Layout**:
```
File Layout:
[0x0]     ELF Header (64 bytes)
[0x40]    Program Header (56 bytes)
[0x78]    Code Section (.text)
          Section Headers
          String Table
```

**Memory Layout**:
```
[0x10000] Code Section (executable)
```

**Critical Alignment**:
- `p_offset = 0x0` (segment starts at file offset 0)
- `p_vaddr = 0x10000` (segment base virtual address)
- `p_filesz = aligned segment size` (includes headers + code)
- `p_memsz = actual code size` (code only, not headers)
- `.text sh_addr = p_vaddr + (sh_offset - p_offset)` (maintains offset relationship)
- `e_entry = text_sh_addr` (entry point at start of .text section)

### Implementation

**ELF Generation Steps**:
1. Write ELF header with entry point
2. Write program header with correct alignment
3. Write code section at `ELF_CODE_OFFSET`
4. Write section headers
5. Write string table
6. Ensure all alignment requirements met

**Validation**:
- Test with `libriscv` to ensure compatibility
- Verify entry point execution
- Check segment loading

## Benefits

1. **libriscv Compatibility**: Works with same library as Godot sandbox
2. **Correct Execution**: ELF files execute properly in emulator
3. **Proper Alignment**: Meets all RISC-V Linux ABI requirements
4. **Testable**: Can validate ELF structure before integration

## Implementation Status

✅ **Completed**:
- ELF header generation
- Program header with correct alignment
- Section header generation
- String table generation
- libriscv compatibility validation

## Testing

**File**: `test_libriscv.cpp`

**Validation**:
- Load ELF file in libriscv
- Execute and verify exit codes
- Debug mode available for instruction-by-instruction debugging

## Alternatives Considered

### Alternative 1: Use Standard ELF Tools
- **Rejected**: Need programmatic control, specific alignment requirements

### Alternative 2: Minimal ELF (No Sections)
- **Rejected**: libriscv needs proper section headers for symbol resolution

### Alternative 3: Use Different Emulator
- **Rejected**: libriscv is already integrated, used by Godot sandbox

## Unresolved Questions

- [ ] Should we add symbol table for debugging?
- [ ] How to handle multiple code sections?
- [ ] Should we add relocation support?

## References

- [RFC 0002: Testing Environments](./0002-testing-environments.md)

