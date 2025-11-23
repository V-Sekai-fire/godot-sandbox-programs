#!/bin/bash
# Copy ELF into container and execute

ELF_FILE="simple.elf"
CONTAINER_ELF="/tmp/test.elf"

echo "=== Testing ELF in Docker Container ==="
echo ""

# Copy file into container and execute
docker run --platform linux/riscv64 --rm \
    -v "$(pwd):/workspace" \
    ubuntu:22.04 sh -c "
        cp /workspace/test_output/$ELF_FILE $CONTAINER_ELF && \
        chmod +x $CONTAINER_ELF && \
        $CONTAINER_ELF
    "

EXIT_CODE=$?
echo ""
echo "Exit code: $EXIT_CODE"

if [ $EXIT_CODE -eq 42 ]; then
    echo "✅ SUCCESS: Program returned 42!"
elif [ $EXIT_CODE -eq 34 ]; then
    echo "⚠️  Program returned 34 (expected 42) - encoding issue"
else
    echo "Program returned: $EXIT_CODE"
fi
