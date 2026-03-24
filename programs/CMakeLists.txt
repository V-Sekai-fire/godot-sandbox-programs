# For standalone builds, we don't need godot-sandbox
# The embedded RISC-V mode uses godot-sandbox, but standalone mode uses mock handlers

function (add_ci_program name)
	add_executable(${name} ${ARGN})
	set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endfunction()

# Build the trivial examples
# NOTE: hello-world requires RISC-V godot-sandbox API (api.hpp with Sandbox class)
# and is only built for embedded RISC-V targets. Skip for standalone builds.
if (NOT EMBEDDED_RISCV)
	message(STATUS "Skipping hello-world (RISC-V only)")
else()
	add_subdirectory(hello-world)
endif()


# ==================== Common JSON-RPC Library ====================
# Shared infrastructure for both embedded and standalone modes.
# This builds the scenetree_common library once.
#
# Handler implementation is controlled by SCENETREE_HANDLERS:
#   - "mock" for standalone/stdio (no Godot dependency)
#   - "real" for embedded godot-sandbox (uses Godot C++ API)  
#   - "none" if frontend provides its own handlers
set(SCENETREE_HANDLERS "mock" CACHE STRING "Handler implementation: mock, real, or none")

add_subdirectory(scenetree_common)


# ==================== Standalone SceneTree Passthrough ====================
# Builds a standalone executable (TCP/stdio server) using the common JSON-RPC sublibrary.
# Only built when NOT in embedded RISC-V mode (i.e., building for host/native platform).
# When EMBEDDED_RISCV=ON, we use godot-sandbox which provides the SceneTree via RISC-V VM.
if (NOT EMBEDDED_RISCV)
	# Build the standalone executable separately
	# Pass the location of the already-built scenetree_common library
	add_subdirectory(scenetree_passthrough_standalone)
endif()


# ==================== RISC-V Subdirectories (CI/CD Mode Only) ====================
# These subdirectories require RISC-V64 Bootlin toolchain and are only built
# when BUILD_FULL=ON (CI/CD mode). They are NOT needed for standalone builds.
if (BUILD_FULL)
	# asm - Godot AssemblyScript module (WebAssembly compilation target)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ext/asm/CMakeLists.txt)
		add_subdirectory(ext/asm)
	else()
		message(WARNING "asm subdirectory not found or disabled")
	endif()
	
	# gdscript - Godot scripting language bindings/interpreter
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ext/gdscript/CMakeLists.txt)
		add_subdirectory(ext/gdscript)
	else()
		message(WARNING "gdscript subdirectory not found or disabled")
	endif()
	
	# libtcc - Tiny C Compiler library (JIT compilation for scripting)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ext/libtcc/CMakeLists.txt)
		add_subdirectory(ext/libtcc)
	else()
		message(WARNING "libtcc subdirectory not found or disabled")
	endif()
	
	# luajit - LuaJIT Lua interpreter (embedded scripting)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ext/luajit/CMakeLists.txt)
		add_subdirectory(ext/luajit)
	else()
		message(WARNING "luajit subdirectory not found or disabled")
	endif()
	
	# mir - MIR compiler (JIT for dynamic languages)
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ext/mir/CMakeLists.txt)
		add_subdirectory(ext/mir)
	else()
		message(WARNING "mir subdirectory not found or disabled")
	endif()
	
	# robust_skin_weight_transfer - Mesh skinning utilities
	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ext/robust_skin_weight_transfer/CMakeLists.txt)
		add_subdirectory(ext/robust_skin_weight_transfer)
	else()
		message(WARNING "robust_skin_weight_transfer subdirectory not found or disabled")
	endif()
endif()
