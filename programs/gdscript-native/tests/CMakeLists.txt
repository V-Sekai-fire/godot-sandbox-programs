cmake_minimum_required(VERSION 3.10)

# Add doctest include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add parser include directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Find MLIR (required for AST to MLIR tests)
if(NOT DEFINED MLIR_DIR)
    set(MLIR_DIR "${CMAKE_SOURCE_DIR}/thirdparty/llvm-build/lib/cmake/mlir")
endif()

find_package(MLIR QUIET CONFIG)

# Test executable for parser tests
add_executable(test_parser
    test_parser.cpp
    ../parser/gdscript_parser.cpp
)

# Test executable for AST to MLIR tests (if MLIR available)
if(MLIR_FOUND)
    find_package(StableHLO QUIET CONFIG)
    if(StableHLO_FOUND)
        add_executable(test_ast_to_mlir
            test_ast_to_mlir.cpp
            ../parser/gdscript_parser.cpp
            ../ast_to_ir.cpp
            ../mlir/mlir_wrapper.cpp
            ../mlir/ir_to_riscv.cpp
        )
        
        mlir_setup_definitions(test_ast_to_mlir)
        target_link_libraries(test_ast_to_mlir PRIVATE
            ${MLIR_LIBS}
            ${StableHLO_LIBS}
        )
        
        # Integration tests
        add_executable(test_integration
            test_integration.cpp
            ../parser/gdscript_parser.cpp
            ../ast_to_ir.cpp
            ../mlir/mlir_wrapper.cpp
            ../mlir/ir_to_riscv.cpp
        )
        
        mlir_setup_definitions(test_integration)
        target_link_libraries(test_integration PRIVATE
            ${MLIR_LIBS}
            ${StableHLO_LIBS}
        )
        
        # Dataset tests
        add_executable(test_dataset
            test_dataset.cpp
            ../parser/gdscript_parser.cpp
            ../test_data_loader.cpp
        )
    endif()
endif()

