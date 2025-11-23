# QEMU Testing Instructions

## Installation

### macOS (Homebrew)
```bash
brew install qemu
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install qemu-user-static
```

## Testing the ELF File

Once QEMU is installed, test the generated ELF file:

```bash
# Run the ELF file
qemu-riscv64 test_output/simple.elf

# Check exit code (should be 42)
echo $?

# With verbose output
qemu-riscv64 -d exec test_output/simple.elf
```

## Expected Behavior

The `simple.elf` program should:
1. Load 42 into register a0 (return value)
2. Return (exit with code 42)

So `qemu-riscv64 test_output/simple.elf` should exit with code 42.

## Alternative: Test in Godot Sandbox

Since the ELF file is designed for Godot sandbox, you can also test it there:

```gdscript
# In Godot
var buffer = FileAccess.get_file_as_bytes("res://simple.elf")
var sandbox = Sandbox.new()
sandbox.set_program_from_buffer(buffer)
# The sandbox will load and execute the ELF
```
