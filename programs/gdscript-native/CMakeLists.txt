cmake_minimum_required(VERSION 3.10)
project(gdscript-native)

# Find MLIR (required by StableHLO)
# MLIR_DIR should be set to the build directory's lib/cmake/mlir
# If using git subrepo, default to thirdparty/llvm-build/lib/cmake/mlir
if(NOT DEFINED MLIR_DIR)
	set(MLIR_DIR "${CMAKE_SOURCE_DIR}/thirdparty/llvm-build/lib/cmake/mlir")
endif()

find_package(MLIR REQUIRED CONFIG)

# Find StableHLO
# STABLEHLO_DIR should be set to the build directory's lib/cmake/stablehlo
# If using git subrepo, default to thirdparty/stablehlo/build/lib/cmake/stablehlo
if(NOT DEFINED STABLEHLO_DIR)
	set(STABLEHLO_DIR "${CMAKE_SOURCE_DIR}/thirdparty/stablehlo/build/lib/cmake/stablehlo")
endif()

find_package(StableHLO REQUIRED CONFIG)

message(STATUS "Using MLIR: ${MLIR_DIR}")
message(STATUS "MLIR libraries: ${MLIR_LIBS}")
message(STATUS "Using StableHLO: ${StableHLO_DIR}")
message(STATUS "StableHLO libraries: ${StableHLO_LIBS}")

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
	mlir/mlir_wrapper.cpp
	mlir/ir_to_riscv.cpp
	ast_to_ir.cpp
	test_data_loader.cpp
	parser/gdscript_parser.cpp
)

# Link with MLIR, StableHLO, and RISC-V assembler library
mlir_setup_definitions(gdscript_native)
target_link_libraries(gdscript_native PRIVATE
	riscv_asmjit
	${MLIR_LIBS}
	${StableHLO_LIBS}
)

if (ZIG_COMPILER)
	target_sources(gdscript_native PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../asm/extra/obstack.c)
endif()
