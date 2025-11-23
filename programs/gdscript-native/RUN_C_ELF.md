# How to Run the C ELF (hello.elf)

## File Location
`test_output/hello.elf`

## Using libriscv

```bash
./test_libriscv test_output/hello.elf
```

## Expected Result

The C program `return 42;` should return exit code **42**.

## ELF Info

```bash
# Check ELF structure
riscv64-unknown-elf-readelf -h test_output/hello.elf
riscv64-unknown-elf-readelf -l test_output/hello.elf

# Disassemble
riscv64-unknown-elf-objdump -d test_output/hello.elf
```

## Comparison

The C ELF (`hello.elf`) vs GDScript ELF (`simple.elf`):
- C ELF: Full ELF with libc startup code, larger size
- GDScript ELF: Minimal ELF, direct function entry point
