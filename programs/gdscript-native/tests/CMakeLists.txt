cmake_minimum_required(VERSION 3.10)
project(gdscript-native-tests)

# Add doctest include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add source include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src/parser)

# Common parser source files
set(PARSER_SOURCES
    ../src/parser/gdscript_tokenizer.cpp
    ../src/parser/gdscript_parser.cpp
    ../src/parser/errors.cpp
)

# Common compiler source files
set(COMPILER_SOURCES
    ${PARSER_SOURCES}
    ../src/ast_to_riscv_biscuit.cpp
    ../src/function_registry.cpp
    ../src/code_memory_manager.cpp
    ../src/elf_generator.cpp
    # Add biscuit source files for linking
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_compressed.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_crypto.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_floating_point.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/assembler_vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/code_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/src/cpuinfo.cpp
)

# Parser tests
add_executable(test_parser_basic
    test_parser_basic.cpp
    ${PARSER_SOURCES}
)

add_executable(test_parser_expressions
    test_parser_expressions.cpp
    ${PARSER_SOURCES}
)

add_executable(test_parser_statements
    test_parser_statements.cpp
    ${PARSER_SOURCES}
)

# Set C++17 standard for parser tests
set_target_properties(test_parser_basic test_parser_expressions test_parser_statements PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Compiler tests
add_executable(test_compiler_basic
    test_compiler_basic.cpp
    ${COMPILER_SOURCES}
)

add_executable(test_compiler_operators
    test_compiler_operators.cpp
    ${COMPILER_SOURCES}
)

add_executable(test_compiler_system
    test_compiler_system.cpp
    ${COMPILER_SOURCES}
)

add_executable(test_compiler_codegen
    test_compiler_codegen.cpp
    ${COMPILER_SOURCES}
)

add_executable(test_compiler_integration
    test_compiler_integration.cpp
    ${COMPILER_SOURCES}
)

add_executable(test_compiler_features
    test_compiler_features.cpp
    ${COMPILER_SOURCES}
)

add_executable(test_compiler_execution
    test_compiler_execution.cpp
    ${COMPILER_SOURCES}
)

# Add biscuit and libriscv includes for compiler tests
target_include_directories(test_compiler_basic PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)
target_include_directories(test_compiler_operators PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)
target_include_directories(test_compiler_system PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)
target_include_directories(test_compiler_codegen PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)
target_include_directories(test_compiler_integration PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)
target_include_directories(test_compiler_features PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)
target_include_directories(test_compiler_execution PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)

# Debug simple test
add_executable(test_debug_simple
    test_debug_simple.cpp
    ${COMPILER_SOURCES}
)

target_include_directories(test_debug_simple PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)

# Link libriscv library for test_debug_simple (reuse the one from test_end_to_end_features)
target_link_libraries(test_debug_simple riscv)

# Set C++20 standard for compiler tests
set_target_properties(test_compiler_basic test_compiler_operators test_compiler_system 
    test_compiler_codegen test_compiler_integration test_compiler_features test_compiler_execution
    test_debug_simple PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# End-to-end features test
add_executable(test_end_to_end_features
    test_end_to_end_features.cpp
    ${COMPILER_SOURCES}
    ../src/elf_generator.cpp
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

# Link the riscv target (C++ library)
target_link_libraries(test_end_to_end_features riscv)

# macOS: Link Security framework (required for libriscv syscalls)
if(APPLE)
    find_library(SECURITY_FRAMEWORK Security)
    if(SECURITY_FRAMEWORK)
        target_link_libraries(test_end_to_end_features ${SECURITY_FRAMEWORK})
        target_link_libraries(test_debug_simple ${SECURITY_FRAMEWORK})
        target_link_libraries(test_compiler_basic ${SECURITY_FRAMEWORK})
        target_link_libraries(test_compiler_operators ${SECURITY_FRAMEWORK})
        target_link_libraries(test_compiler_system ${SECURITY_FRAMEWORK})
        target_link_libraries(test_compiler_codegen ${SECURITY_FRAMEWORK})
        target_link_libraries(test_compiler_integration ${SECURITY_FRAMEWORK})
        target_link_libraries(test_compiler_features ${SECURITY_FRAMEWORK})
        target_link_libraries(test_compiler_execution ${SECURITY_FRAMEWORK})
    endif()
endif()

# AST Interpreter test
add_executable(test_ast_interpreter
    test_ast_interpreter.cpp
    ${PARSER_SOURCES}
    ../src/ast_interpreter.cpp
)

# Adhoc tests (converted from manual testing)
add_executable(test_adhoc
    test_adhoc.cpp
    ${PARSER_SOURCES}
    ../src/ast_interpreter.cpp
)

# Set C++17 standard for AST interpreter test
set_target_properties(test_ast_interpreter PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Set C++17 standard for adhoc test
set_target_properties(test_adhoc PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Test runner - runs all tests in a single executable
add_executable(test_runner
    test_runner.cpp
    ${COMPILER_SOURCES}
    ../src/elf_generator.cpp
    ../src/ast_interpreter.cpp
    ../src/riscv_code_templates.cpp
)

# Set C++20 standard for test runner
set_target_properties(test_runner PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Add biscuit include for test runner
target_include_directories(test_runner PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/biscuit/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../ext/libriscv/lib
)

# Link libriscv library for test runner
target_link_libraries(test_runner riscv)

# macOS: Link Security framework for test runner
if(APPLE)
    if(SECURITY_FRAMEWORK)
        target_link_libraries(test_runner ${SECURITY_FRAMEWORK})
    endif()
endif()
