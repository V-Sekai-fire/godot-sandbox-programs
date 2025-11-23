cmake_minimum_required(VERSION 3.10)
project(gdscript-native-tests)

# Add doctest include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add parser include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Test executable for parser tests
add_executable(test_parser
    test_parser.cpp
    ../parser/gdscript_parser.cpp
    ../parser/errors.cpp
)

# Set C++17 standard (required for std::variant and std::any)
set_target_properties(test_parser PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Test executable for compiler tests (TDD style)
add_executable(test_compiler
    test_compiler.cpp
    ../parser/gdscript_parser.cpp
    ../parser/errors.cpp
    ../ast_to_riscv_biscuit.cpp
    ../function_registry.cpp
    ../code_memory_manager.cpp
    # Add biscuit source files for linking
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_compressed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_crypto.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_floating_point.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/code_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/cpuinfo.cpp
)

# Add biscuit include for compiler tests
target_include_directories(test_compiler PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
)

# Set C++20 standard (required for biscuit library)
set_target_properties(test_compiler PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Dataset tests
add_executable(test_dataset
    test_dataset.cpp
    ../parser/gdscript_parser.cpp
    ../parser/errors.cpp
    ../test_data_loader.cpp
)

# Set C++17 standard (required for std::variant and std::any)
set_target_properties(test_dataset PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# ELF generation test
add_executable(test_elf_generation
    ../test_elf_generation.cpp
    ../parser/gdscript_parser.cpp
    ../parser/errors.cpp
    ../ast_to_riscv_biscuit.cpp
    ../elf_generator.cpp
    ../function_registry.cpp
    ../code_memory_manager.cpp
    # Add biscuit source files for linking
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_compressed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_crypto.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_floating_point.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/code_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/cpuinfo.cpp
)

# Set C++20 standard (required for biscuit library)
set_target_properties(test_elf_generation PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Add biscuit include for ELF generation test
target_include_directories(test_elf_generation PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
)

# Note: biscuit is header-only for our use case
# If linking errors occur, we may need to add biscuit source files

# End-to-end features test
add_executable(test_end_to_end_features
    test_end_to_end_features.cpp
    ../parser/gdscript_parser.cpp
    ../parser/errors.cpp
    ../ast_to_riscv_biscuit.cpp
    ../elf_generator.cpp
    ../function_registry.cpp
    ../code_memory_manager.cpp
    # Add biscuit source files for linking
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_compressed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_crypto.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_floating_point.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/code_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/cpuinfo.cpp
)

# Set C++20 standard (required for biscuit library)
set_target_properties(test_end_to_end_features PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Add biscuit include for end-to-end features test
target_include_directories(test_end_to_end_features PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib/build
)

# Link libriscv library
# Add libriscv as a subdirectory to build it
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib 
    ${CMAKE_CURRENT_BINARY_DIR}/libriscv_build
    EXCLUDE_FROM_ALL)

# Link the riscv target
target_link_libraries(test_end_to_end_features riscv)

# macOS: Link Security framework (required for libriscv syscalls)
if(APPLE)
    find_library(SECURITY_FRAMEWORK Security)
    if(SECURITY_FRAMEWORK)
        target_link_libraries(test_end_to_end_features ${SECURITY_FRAMEWORK})
    endif()
endif()

