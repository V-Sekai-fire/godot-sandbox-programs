#!/bin/bash
# Test RISC-V 64 Linux assembly generation using external toolchain
# Requires: riscv64-linux-gnu-gcc, riscv64-linux-gnu-objdump, qemu-riscv64

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for RISC-V toolchain
if ! command -v riscv64-linux-gnu-gcc &> /dev/null; then
    echo -e "${RED}Error: riscv64-linux-gnu-gcc not found${NC}"
    echo "Install with: sudo apt-get install gcc-riscv64-linux-gnu"
    exit 1
fi

if ! command -v qemu-riscv64 &> /dev/null; then
    echo -e "${YELLOW}Warning: qemu-riscv64 not found (optional for execution)${NC}"
fi

# Test directory
TEST_DIR="test_riscv_output"
mkdir -p "$TEST_DIR"

echo -e "${GREEN}Testing RISC-V 64 Linux assembly generation...${NC}"

# Test 1: Simple function returning 42
cat > "$TEST_DIR/test1.s" << 'EOF'
.option pic
.text
.align 2

.globl hello
.type hello, @function
hello:
    # Function prologue
    addi sp, sp, -16
    sd ra, 8(sp)
    sd s0, 0(sp)
    addi s0, sp, 16
    
    # return 42
    li t0, 42
    mv a0, t0
    
    # Function epilogue
    ld ra, 8(sp)
    ld s0, 0(sp)
    addi sp, sp, 16
    ret
.size hello, .-hello
EOF

echo -e "${YELLOW}Test 1: Simple return 42${NC}"
riscv64-linux-gnu-gcc -c -o "$TEST_DIR/test1.o" "$TEST_DIR/test1.s" 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Assembly compiles successfully${NC}"
    riscv64-linux-gnu-objdump -d "$TEST_DIR/test1.o" | head -20
else
    echo -e "${RED}✗ Assembly compilation failed${NC}"
    exit 1
fi

# Test 2: Function with addition
cat > "$TEST_DIR/test2.s" << 'EOF'
.option pic
.text
.align 2

.globl add_func
.type add_func, @function
add_func:
    # Function prologue
    addi sp, sp, -16
    sd ra, 8(sp)
    sd s0, 0(sp)
    addi s0, sp, 16
    
    # Store arguments (a0, a1) to stack
    sd a0, 16(sp)
    sd a1, 24(sp)
    
    # Load and add
    ld t0, 16(sp)
    ld t1, 24(sp)
    add t2, t0, t1
    mv a0, t2
    
    # Function epilogue
    ld ra, 8(sp)
    ld s0, 0(sp)
    addi sp, sp, 32
    ret
.size add_func, .-add_func
EOF

echo -e "\n${YELLOW}Test 2: Addition function${NC}"
riscv64-linux-gnu-gcc -c -o "$TEST_DIR/test2.o" "$TEST_DIR/test2.s" 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Assembly compiles successfully${NC}"
    riscv64-linux-gnu-objdump -d "$TEST_DIR/test2.o" | head -25
else
    echo -e "${RED}✗ Assembly compilation failed${NC}"
    exit 1
fi

# Test 3: Create a complete executable
cat > "$TEST_DIR/main.c" << 'EOF'
#include <stdio.h>

extern int hello(void);
extern int add_func(int a, int b);

int main() {
    int result1 = hello();
    int result2 = add_func(10, 20);
    
    printf("hello() = %d\n", result1);
    printf("add_func(10, 20) = %d\n", result2);
    
    return (result1 == 42 && result2 == 30) ? 0 : 1;
}
EOF

echo -e "\n${YELLOW}Test 3: Linking with C main${NC}"
riscv64-linux-gnu-gcc -static -o "$TEST_DIR/test_program" "$TEST_DIR/main.c" "$TEST_DIR/test1.o" "$TEST_DIR/test2.o" 2>&1
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Linking successful${NC}"
    
    if command -v qemu-riscv64 &> /dev/null; then
        echo -e "${YELLOW}Running with qemu-riscv64...${NC}"
        qemu-riscv64 "$TEST_DIR/test_program"
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}✓ Program executed successfully${NC}"
        else
            echo -e "${RED}✗ Program execution failed${NC}"
        fi
    else
        echo -e "${YELLOW}qemu-riscv64 not available, skipping execution test${NC}"
    fi
else
    echo -e "${RED}✗ Linking failed${NC}"
    exit 1
fi

echo -e "\n${GREEN}All tests passed!${NC}"
echo "Generated files in: $TEST_DIR/"

