# Standalone build fix for godot-sandbox program/cpp/cmake/CMakeLists.txt
#
# This file contains a CMake function that applies patches to godot-sandbox
# CMakeLists.txt to fix standalone build support.
#
# The issue: https://github.com/libriscv/godot-sandbox CMakeLists.txt
# unconditionally sets RISCV_ABI="-mabi=lp64d" which breaks host compiler builds.
#
# This patch wraps the RISCV_ABI setting with EMBEDDED_RISCV check.

function(apply_godot_sandbox_standalone_fix)
    # Check if godot-sandbox source exists in build directory
    set(godot_cmake "${CMAKE_CURRENT_BINARY_DIR}/_deps/godot-sandbox-build/godot-sandbox-download/console/install/program/cpp/cmake/CMakeLists.txt")
    
    # Also check other possible locations
    set(alternative_locations
        "${CMAKE_CURRENT_BINARY_DIR}/_deps/godot-sandbox-subbuild/godot-sandbox-populate-prefix/src/godot-sandbox-src/program/cpp/cmake/CMakeLists.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/build/_deps/godot-sandbox-src/program/cpp/cmake/CMakeLists.txt"
    )
    
    set(source_file "")
    foreach(loc ${godot_cmake} ${alternative_locations})
        if(EXISTS "${loc}")
            set(source_file "${loc}")
            break()
        endif()
    endforeach()
    
    if(NOT source_file)
        message(STATUS "godot-sandbox CMakeLists.txt not found, cannot apply patch")
        return()
    endif()
    
    message(STATUS "Applying godot-sandbox standalone patch to: ${source_file}")
    
    # Read the file
    file(READ "${source_file}" content)
    
    # Check if patch is already applied
    if(content MATCHES "if \(EMBEDDED_RISCV\).*set\(RISCV_ABI")
        message(STATUS "Patch already applied")
        return()
    endif()
    
    # Apply patch manually using string replacement
    # The code has:
    #
    # else()
    #   ...
    #   set(RISCV_ABI "-mabi=lp64d")  <- Line ~69, inside EMBEDDED_RISCV=OFF block
    # endif()
    #
    # We need to wrap the set(RISCV_ABI ...) with if(EMBEDDED_RISCV)
    
    string(REPLACE 
        "set(RISCV_ABI \"-mabi=lp64d\")" 
        "if(EMBEDDED_RISCV)\n    set(RISCV_ABI \"-mabi=lp64d\")\nendif()"
        patched_content
        "${content}"
    )
    
    # Write back
    file(WRITE "${source_file}" "${patched_content}")
    
    message(STATUS "Applied godot-sandbox standalone patch")
endfunction()

# Register the patch to run after configure
apply_godot_sandbox_standalone_fix()
