cmake_minimum_required(VERSION 3.10)
project(gdscript-vm)

# GDScript VM - Port of Godot's GDScript bytecode VM to godot-sandbox
# Uses gdextension API from godot-sandbox

# Build the gdscript-vm sandbox program
add_ci_program(gdscript_vm
	main.cpp
	gdscript_byte_codegen.cpp
	gdscript_function.cpp
	gdscript_utility_functions.cpp
	gdscript_vm.cpp
	gdscript.cpp
	ast_to_bytecode_adapter.cpp
)

# Set C++20 standard
set_target_properties(gdscript_vm PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Add tests subdirectory
add_subdirectory(tests)

