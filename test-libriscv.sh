#!/bin/bash
# Test if libriscv can run the RISC-V .NET SDK without mutex issues

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}Testing libriscv with RISC-V .NET SDK...${NC}"

# Check if libriscv is built
LIBRISCV_DIR="./libriscv-test"
EMULATOR="$LIBRISCV_DIR/emulator/rvlinux"

if [ ! -f "$EMULATOR" ]; then
    echo -e "${YELLOW}libriscv emulator not found. Building...${NC}"
    cd "$LIBRISCV_DIR/emulator"
    if [ -f "build.sh" ]; then
        ./build.sh
    else
        echo -e "${RED}Error: build.sh not found${NC}"
        exit 1
    fi
    cd - > /dev/null
fi

# Check if .NET SDK exists
DOTNET_SDK="./.dotnet-riscv/dotnet"
if [ ! -f "$DOTNET_SDK" ]; then
    echo -e "${YELLOW}.NET SDK not found. Downloading...${NC}"
    ./download-dotnet-riscv.sh
fi

# Test running dotnet --version with libriscv
echo -e "${GREEN}Testing: libriscv running .NET SDK...${NC}"
"$EMULATOR" "$DOTNET_SDK" --version 2>&1

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ libriscv can run the SDK!${NC}"
    echo "Testing dotnet publish..."
    "$EMULATOR" "$DOTNET_SDK" publish programs/csharp-hello/csharp-hello.csproj \
        -r linux-riscv64 -c Release --self-contained true \
        /p:PublishAot=true 2>&1 | head -50
else
    echo -e "${RED}❌ libriscv failed to run SDK${NC}"
    exit 1
fi

