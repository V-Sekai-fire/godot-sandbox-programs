cmake_minimum_required(VERSION 3.10)
project(scenetree_passthrough_standalone)

add_ci_program(scenetree_passthrough_standalone
    main.cpp
)

# Standalone native mode - uses mock handlers, no godot-sandbox dependency
# Define STANDALONE_NATIVE to use mock implementations instead of api.hpp
target_compile_definitions(scenetree_passthrough_standalone PRIVATE STANDALONE_NATIVE)
