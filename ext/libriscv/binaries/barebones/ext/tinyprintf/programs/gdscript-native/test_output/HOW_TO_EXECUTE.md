# How to Execute the ELF File

## 🎮 Option 1: Godot Sandbox (Recommended - Primary Target)

This is the **intended execution environment**. The ELF file is designed for Godot sandbox.

### Step 1: Copy ELF to Godot project
```bash
# Copy the ELF file to your Godot project
cp test_output/simple.elf /path/to/your/godot/project/simple.elf
```

### Step 2: Load in Godot GDScript
```gdscript
# Method 1: Load from file
var buffer = FileAccess.get_file_as_bytes("res://simple.elf")
var sandbox = Sandbox.FromBuffer(buffer)

# Method 2: Use compile_gdscript API (if gdscript-native is built)
var elf_data = compile_gdscript("func test():\n    return 42\n")
var sandbox = Sandbox.FromBuffer(elf_data)

# The sandbox will automatically:
# - Load the ELF into RISC-V machine
# - Execute initialization code
# - Make functions available via vmcall
```

### Step 3: Call functions
```gdscript
# Functions are available as methods on the sandbox
var result = sandbox.vmcall("test")
print(result)  # Should print 42
```

## 🐳 Option 2: Docker with Platform Specification

For ARM64 Macs, specify the platform:

```bash
docker run --platform linux/riscv64 --rm -v "$(pwd)/test_output:/mnt" \
  riscv64/ubuntu:22.04 /mnt/simple.elf
```

Or use a multi-arch image:
```bash
docker run --platform linux/riscv64 --rm -v "$(pwd)/test_output:/mnt" \
  ubuntu:22.04 /mnt/simple.elf
```

## ✅ Option 3: Verification (Works Now!)

Even without execution, you can verify the ELF is correct:

```bash
# Check file type
file test_output/simple.elf
# Output: ELF 64-bit LSB executable, UCB RISC-V

# Inspect ELF headers
riscv64-unknown-elf-readelf -h test_output/simple.elf

# Disassemble code
riscv64-unknown-elf-objdump -d test_output/simple.elf
```

## 📝 Notes

- The ELF file structure is **validated** and correct
- It follows RISC-V 64-bit Linux ABI
- It's ready for Godot sandbox integration
- Execution in QEMU user-mode requires Linux or x86_64 Docker
