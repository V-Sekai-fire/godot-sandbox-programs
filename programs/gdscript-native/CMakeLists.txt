cmake_minimum_required(VERSION 3.10)
project(gdscript-native)

# No MLIR/StableHLO dependency - direct AST to RISC-V emission (following BEAM JIT pattern)

# Add the RISC-V asmjit static library (for assemble_to function)
add_library(riscv_asmjit STATIC IMPORTED)
if (ZIG_COMPILER)
	set_target_properties(riscv_asmjit PROPERTIES
		IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../asm/zig-libRiscvAsmLib.a)
else()
	set_target_properties(riscv_asmjit PROPERTIES
		IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../asm/libRiscvAsmLib.a)
endif()

# Add zlib dependency (required by asmjit)
include(FetchContent)
FetchContent_Declare(
	zlib
	GIT_REPOSITORY https://github.com/madler/zlib.git
	GIT_TAG develop
)
FetchContent_MakeAvailable(zlib)
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
target_link_libraries(riscv_asmjit INTERFACE zlibstatic)

# Build the gdscript-native sandbox program
add_ci_program(gdscript_native
	main.cpp
	ast_to_riscv.cpp
	test_data_loader.cpp
	parser/gdscript_parser.cpp
)

# Link with RISC-V assembler library (no MLIR/StableHLO)
target_link_libraries(gdscript_native PRIVATE
	riscv_asmjit
)

if (ZIG_COMPILER)
	target_sources(gdscript_native PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../asm/extra/obstack.c)
endif()
