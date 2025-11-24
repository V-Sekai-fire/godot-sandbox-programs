# Quick Start: Execute the ELF File

## ✅ Docker (Easiest - Works Now!)

```bash
cd programs/gdscript-native
docker run --rm -v "$(pwd)/test_output:/mnt" riscv64/ubuntu:22.04 /mnt/simple.elf
echo $?  # Check exit code
```

## 🎮 Godot Sandbox (Primary Target)

```gdscript
# In Godot editor or script
var buffer = FileAccess.get_file_as_bytes("res://simple.elf")
var sandbox = Sandbox.FromBuffer(buffer)

# Or using compile_gdscript API:
var elf_data = compile_gdscript("func test():\n    return 42\n")
var sandbox = Sandbox.FromBuffer(elf_data)
```

## 🔍 Verify Without Execution

```bash
# Check it's a valid ELF
file test_output/simple.elf

# Inspect structure
riscv64-unknown-elf-readelf -h test_output/simple.elf
riscv64-unknown-elf-objdump -d test_output/simple.elf
```
