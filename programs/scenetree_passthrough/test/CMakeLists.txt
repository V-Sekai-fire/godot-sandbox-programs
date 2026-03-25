cmake_minimum_required(VERSION 3.10)
project(scenetree_passthrough_tests)

set(CMAKE_CXX_STANDARD 17)

# Add RapidCheck (it's in programs/rapidcheck)
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../rapidcheck
    ${CMAKE_BINARY_DIR}/rapidcheck
)

# Test executable
add_executable(jsonrpc_tests
    test_jsonrpc_compliance.cpp
)

# Link with RapidCheck and pthread
target_link_libraries(jsonrpc_tests rapidcheck pthread)

