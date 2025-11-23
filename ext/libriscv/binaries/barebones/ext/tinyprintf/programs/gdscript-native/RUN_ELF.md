# How to Run the Generated RISC-V ELF

## Current Setup

The ELF file is generated at: `test_output/simple.elf`

## Running with libriscv

```bash
./test_libriscv test_output/simple.elf
```

## Expected Result

The function `func test(): return 42` should return exit code **42**.

## Current Issue

Getting "Execution space protection fault" - investigating and fixing.

## Debugging

Check ELF structure:
```bash
riscv64-unknown-elf-readelf -l test_output/simple.elf
riscv64-unknown-elf-objdump -d test_output/simple.elf
```

The ELF has correct program header:
- `p_filesz: 60 bytes (0x3c)` ✅
- `p_memsz: 60 bytes (0x3c)` ✅
- Flags: `R E` (readable and executable) ✅
