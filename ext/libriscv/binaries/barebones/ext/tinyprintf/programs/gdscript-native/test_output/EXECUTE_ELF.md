# How to Execute the RISC-V ELF File

## Option 1: Godot Sandbox (Recommended - Primary Target)

The ELF file is designed to run in Godot sandbox. This is the intended execution environment.

### In Godot GDScript:
```gdscript
# Option A: Load from file
var buffer = FileAccess.get_file_as_bytes("res://simple.elf")
var sandbox = Sandbox.new()
sandbox.load_buffer(buffer)

# Option B: Use static factory method
var buffer = FileAccess.get_file_as_bytes("res://simple.elf")
var sandbox = Sandbox.FromBuffer(buffer)

# The sandbox will load and execute the ELF automatically
# Functions will be available via vmcall
```

### Using the compile_gdscript API:
```gdscript
# Compile GDScript to ELF
var elf_data = compile_gdscript("func test():\n    return 42\n")

# Save to file
var file = FileAccess.open("res://output.elf", FileAccess.WRITE)
file.store_buffer(elf_data)
file.close()

# Load in sandbox
var sandbox = Sandbox.new()
sandbox.set_program_from_buffer(elf_data)
```

## Option 2: Docker with RISC-V (User-mode Emulation)

If you have Docker installed:

```bash
# Pull RISC-V image
docker pull riscv64/ubuntu:22.04

# Copy ELF into container and execute
docker run --rm -v $(pwd)/test_output:/mnt riscv64/ubuntu:22.04 /mnt/simple.elf
```

## Option 3: QEMU System Emulator (Requires Kernel)

For full system emulation, you need a RISC-V kernel:

```bash
# This requires a RISC-V kernel image
qemu-system-riscv64 -kernel test_output/simple.elf -nographic
```

## Option 4: Linux VM with RISC-V Toolchain

If you have a Linux VM or WSL:

```bash
# Install qemu-user-static
sudo apt-get install qemu-user-static

# Execute
qemu-riscv64 test_output/simple.elf
echo $?  # Should print 42
```

## Quick Test: Verify ELF Structure

Even without execution, you can verify the ELF is correct:

```bash
# Check file type
file test_output/simple.elf

# Inspect headers
riscv64-unknown-elf-readelf -h test_output/simple.elf
riscv64-unknown-elf-readelf -l test_output/simple.elf

# Disassemble
riscv64-unknown-elf-objdump -d test_output/simple.elf
```
