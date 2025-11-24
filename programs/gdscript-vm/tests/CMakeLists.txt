cmake_minimum_required(VERSION 3.10)
project(gdscript-vm-tests)

# Add doctest include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add source include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../gdscript-native/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../gdscript-native/src/parser)

# Add godot-sandbox API include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/godot-sandbox/program/cpp/docker/api)

# Common parser source files (from gdscript-native)
set(PARSER_SOURCES
    ../../gdscript-native/src/parser/gdscript_tokenizer.cpp
    ../../gdscript-native/src/parser/gdscript_parser.cpp
    ../../gdscript-native/src/parser/errors.cpp
)

# VM source files (commented out - require RISC-V API which doesn't compile on macOS)
# These will be tested on RISC-V platform
# set(VM_SOURCES
#     ../gdscript.cpp
#     ../gdscript_byte_codegen.cpp
#     ../gdscript_function.cpp
#     ../gdscript_utility_functions.cpp
#     ../ast_to_bytecode_adapter.cpp
# )

# Test runner (parser tests only - VM tests require RISC-V platform)
add_executable(test_vm_runner
    test_runner.cpp
    ${PARSER_SOURCES}
    # ${VM_SOURCES}  # Disabled - requires RISC-V API
)

# Set C++20 standard
set_target_properties(test_vm_runner PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Link against godot-sandbox API if needed
# Note: We can't link against the full sandbox due to RISC-V architecture flags on macOS
# These tests will validate the code structure and compilation

