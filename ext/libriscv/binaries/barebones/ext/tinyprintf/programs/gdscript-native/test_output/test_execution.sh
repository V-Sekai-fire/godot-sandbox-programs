#!/bin/bash
# Test ELF execution in Docker

set -e

ELF_FILE="${1:-test_output/simple.elf}"

echo "=== Testing ELF Execution in Docker ==="
echo ""

# Check if file exists
if [ ! -f "$ELF_FILE" ]; then
    echo "Error: ELF file not found: $ELF_FILE"
    exit 1
fi

echo "ELF file: $ELF_FILE"
echo "File type: $(file "$ELF_FILE")"
echo ""

# Try direct execution (if native RISC-V)
echo "Attempting direct execution..."
docker run --platform linux/riscv64 --rm \
    -v "$(pwd)/test_output:/mnt" \
    ubuntu:22.04 /mnt/simple.elf 2>&1 || true

echo ""
echo "Installing qemu-user-static and testing..."
docker run --platform linux/riscv64 --rm \
    -v "$(pwd)/test_output:/mnt" \
    ubuntu:22.04 sh -c "
        apt-get update -qq >/dev/null 2>&1 && \
        apt-get install -y -qq qemu-user-static >/dev/null 2>&1 && \
        qemu-riscv64 /mnt/simple.elf
    "

EXIT_CODE=$?
echo ""
echo "Exit code: $EXIT_CODE"

if [ $EXIT_CODE -eq 42 ]; then
    echo "✅ SUCCESS: Program returned 42 as expected!"
elif [ $EXIT_CODE -eq 34 ]; then
    echo "⚠️  Program returned 34 (expected 42) - encoding issue"
else
    echo "Program returned: $EXIT_CODE"
fi
