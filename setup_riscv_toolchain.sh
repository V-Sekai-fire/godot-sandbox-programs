#!/bin/bash
set -e

cd "$(dirname "$0")"

echo "=== Setting up RISC-V Toolchain ==="

# Check if toolchain is already set up
if [ -d "riscv64-toolchain" ] && [ -f "riscv64-toolchain/bin/riscv64-linux-gnu-gcc" ]; then
    echo "RISC-V toolchain already set up."
else
    # Download Bootlin toolchain
    if [ ! -f "riscv64-lp64d--glibc--stable-2025.08-1.tar.xz" ]; then
        echo "Downloading RISC-V toolchain..."
        curl -L https://toolchains.bootlin.com/downloads/releases/toolchains/riscv64-lp64d/tarballs/riscv64-lp64d--glibc--stable-2025.08-1.tar.xz -o riscv64-lp64d--glibc--stable-2025.08-1.tar.xz
    else
        echo "Toolchain archive already exists."
    fi

    # Extract
    if [ ! -d "riscv64-lp64d--glibc--stable-2025.08-1" ]; then
        echo "Extracting toolchain..."
        tar -xf riscv64-lp64d--glibc--stable-2025.08-1.tar.xz
    fi

    # Move to expected location
    if [ ! -d "riscv64-toolchain" ]; then
        mv riscv64-lp64d--glibc--stable-2025.08-1 riscv64-toolchain
        echo "Toolchain moved to riscv64-toolchain/"
    fi

    # Clean up
    rm -f riscv64-lp64d--glibc--stable-2025.08-1.tar.xz
fi

# Create symlinks for riscv64-linux-gnu tools
echo "Creating symlinks..."
cd riscv64-toolchain/bin
ln -sf riscv64-buildroot-linux-gnu-gcc riscv64-linux-gnu-gcc 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-g++ riscv64-linux-gnu-g++ 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-ld riscv64-linux-gnu-ld 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-ar riscv64-linux-gnu-ar 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-ranlib riscv64-linux-gnu-ranlib 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-strip riscv64-linux-gnu-strip 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-as riscv64-linux-gnu-as 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-nm riscv64-linux-gnu-nm 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-objdump riscv64-linux-gnu-objdump 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-objcopy riscv64-linux-gnu-objcopy 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-readelf riscv64-linux-gnu-readelf 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-addr2line riscv64-linux-gnu-addr2line 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-elfedit riscv64-linux-gnu-elfedit 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-c++ riscv64-linux-gnu-c++ 2>/dev/null || true
ln -sf riscv64-buildroot-linux-gnu-cpp riscv64-linux-gnu-cpp 2>/dev/null || true

echo "=== Toolchain setup complete ==="
echo "Verify with: riscv64-linux-gnu-gcc --version"
