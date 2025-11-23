cmake_minimum_required(VERSION 3.10)

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

# Test executable for compiler tests (TDD style)
add_executable(test_compiler
    test_compiler.cpp
    ../parser/gdscript_parser.cpp
    ../parser/errors.cpp
    ../ast_to_riscv_biscuit.cpp
    ../function_registry.cpp
    ../code_memory_manager.cpp
)

# Add biscuit include for compiler tests
target_include_directories(test_compiler PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/biscuit/include
)

# Dataset tests
add_executable(test_dataset
    test_dataset.cpp
    ../parser/gdscript_parser.cpp
    ../test_data_loader.cpp
)

