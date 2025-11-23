#!/bin/bash
# Script to inspect generated ELF files with RISC-V toolchain

set -e

ELF_FILE="${1:-test_output/test.elf}"

if [ ! -f "$ELF_FILE" ]; then
    echo "Error: ELF file not found: $ELF_FILE"
    echo "Usage: $0 [elf_file]"
    exit 1
fi

echo "=== File Type ==="
file "$ELF_FILE"
echo ""

echo "=== ELF Header (readelf -h) ==="
riscv64-unknown-elf-readelf -h "$ELF_FILE" 2>/dev/null || echo "readelf not available"
echo ""

echo "=== Program Headers (readelf -l) ==="
riscv64-unknown-elf-readelf -l "$ELF_FILE" 2>/dev/null || echo "readelf not available"
echo ""

echo "=== Section Headers (readelf -S) ==="
riscv64-unknown-elf-readelf -S "$ELF_FILE" 2>/dev/null || echo "readelf not available"
echo ""

echo "=== Symbols (readelf -s) ==="
riscv64-unknown-elf-readelf -s "$ELF_FILE" 2>/dev/null || echo "readelf not available"
echo ""

echo "=== Disassembly (objdump -d) ==="
riscv64-unknown-elf-objdump -d "$ELF_FILE" 2>/dev/null || echo "objdump not available"
echo ""

echo "=== Hex Dump (first 256 bytes) ==="
hexdump -C "$ELF_FILE" | head -16

