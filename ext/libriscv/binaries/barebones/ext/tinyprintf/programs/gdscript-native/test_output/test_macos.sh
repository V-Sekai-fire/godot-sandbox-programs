#!/bin/bash
# Test ELF execution on macOS

echo "=== Testing RISC-V ELF on macOS ==="
echo ""

# Option 1: Try using qemu-system-riscv64 with minimal kernel
echo "Option 1: Using qemu-system-riscv64 (requires kernel)"
echo "This would need a RISC-V kernel image..."
echo ""

# Option 2: Docker with proper setup
echo "Option 2: Docker with RISC-V platform"
echo "Trying Docker with proper volume mount..."
docker run --platform linux/riscv64 --rm \
    -v "$(pwd):/workspace" \
    ubuntu:22.04 sh -c "
        cd /workspace/test_output && \
        cp simple.elf /tmp/test.elf && \
        chmod +x /tmp/test.elf && \
        apt-get update -qq >/dev/null 2>&1 && \
        apt-get install -y -qq qemu-user-static >/dev/null 2>&1 && \
        qemu-riscv64 /tmp/test.elf
    "
EXIT_CODE=$?
echo "Exit code: $EXIT_CODE"
