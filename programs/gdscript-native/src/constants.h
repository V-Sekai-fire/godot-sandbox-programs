#pragma once

#include <cstdint>
#include <cstddef>

namespace gdscript {
namespace constants {

// ELF generation constants
constexpr uint64_t ELF_ENTRY_POINT = 0x10000;
constexpr size_t PAGE_SIZE = 0x1000;
constexpr size_t ELF_HEADER_SIZE = 64;
constexpr size_t PROGRAM_HEADER_SIZE = 56;
constexpr size_t ELF_CODE_OFFSET = ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE; // 0x78

// Code generation constants
constexpr size_t INITIAL_CODE_BUFFER_SIZE = 8192;
constexpr double BUFFER_GROWTH_THRESHOLD = 0.9; // Grow when 90% full
constexpr int ESTIMATED_LOCAL_VARS_SIZE = 64;
constexpr int BYTES_PER_PARAMETER = 8;
constexpr int STACK_ALIGNMENT = 16;
constexpr int SAVED_REGISTERS_SIZE = 16; // ra + s0

// RISC-V ABI constants
constexpr int MAX_ARGUMENT_REGISTERS = 8; // a0-a7
constexpr int NUM_TEMP_REGISTERS = 7; // t0-t6

} // namespace constants
} // namespace gdscript

