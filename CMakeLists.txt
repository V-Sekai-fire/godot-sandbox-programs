cmake_minimum_required(VERSION 3.14)
project(godot-sandbox-programs)

# CI/CD Mode vs Standalone Mode
# ==============================
# CI/CD (full build with RISC-V toolchain): Builds everything including
#   - asm, gdscript, libtcc, luajit, mir, robust_skin_weight_transfer
#   - Uses RISC-V64 Bootlin toolchain for cross-compilation
#
# Standalone (default for x86_64 hosts): Only builds programs/
#   - Uses mock handlers, no RISC-V dependencies
#   - Creates scenetree_passthrough_standalone binary
#
option(BUILD_FULL "Build full RISC-V toolchain dependencies (CI/CD mode)" OFF)
option(EMBEDDED_RISCV "Build for embedded RISC-V godot-sandbox" OFF)

if(BUILD_FULL)
    # Full CI/CD build - enable all subdirectories
    # Requires RISC-V64 Bootlin toolchain to be configured
    add_subdirectory(ext)
    add_subdirectory(programs)
else()
    # Default standalone build - skip RISC-V dependencies
    message(STATUS "Standalone mode: skipping RISC-V dependencies (set BUILD_FULL=ON for CI/CD)")
    add_subdirectory(programs)
endif()
