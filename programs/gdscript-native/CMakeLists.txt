cmake_minimum_required(VERSION 3.10)
project(gdscript-native)

# No MLIR/StableHLO dependency - direct AST to RISC-V emission (following BEAM JIT pattern)
# Using biscuit for RISC-V code generation (similar to AsmJit in BEAM JIT)

# Add biscuit library (RISC-V assembler, similar to AsmJit)
include(FetchContent)
FetchContent_Declare(
	biscuit
	GIT_REPOSITORY https://github.com/lioncash/biscuit.git
	GIT_TAG main
)
FetchContent_MakeAvailable(biscuit)

# Biscuit is header-only, just need to include the directory
target_include_directories(gdscript_native PRIVATE ${biscuit_SOURCE_DIR}/include)

# Build the gdscript-native sandbox program
add_ci_program(gdscript_native
	main.cpp
	ast_to_riscv_biscuit.cpp
	test_data_loader.cpp
	parser/gdscript_parser.cpp
)

# No linking needed - biscuit is header-only
# (no MLIR/StableHLO, no external assembler library needed)
