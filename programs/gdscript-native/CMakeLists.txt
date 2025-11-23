cmake_minimum_required(VERSION 3.10)
project(gdscript-native)

# No MLIR/StableHLO dependency - direct AST to RISC-V emission (following BEAM JIT pattern)
# Using biscuit for RISC-V code generation (similar to AsmJit in BEAM JIT)

# Build the gdscript-native sandbox program
add_ci_program(gdscript_native
	main.cpp
	ast_to_riscv_biscuit.cpp
	test_data_loader.cpp
	parser/gdscript_parser.cpp
	parser/errors.cpp
	function_registry.cpp
	code_memory_manager.cpp
	elf_generator.cpp
)

# Add biscuit library (RISC-V assembler, similar to AsmJit)
# Biscuit is available as a git subrepo in ext/biscuit
# Biscuit is header-only, just need to include the directory
target_include_directories(gdscript_native PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../ext/biscuit/include)

# Set C++20 standard (required for biscuit library)
set_target_properties(gdscript_native PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# No linking needed - biscuit is header-only
# (no MLIR/StableHLO, no external assembler library needed)

# Add tests subdirectory
add_subdirectory(tests)
