#!/bin/bash
# Test ELF file with libriscv in Docker

set -e

echo "=== Building Docker image with libriscv ==="
docker build -f Dockerfile.libriscv -t test-libriscv .

echo ""
echo "=== Running ELF test with libriscv ==="
docker run --rm test-libriscv

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
