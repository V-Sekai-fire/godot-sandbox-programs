# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-src")
  file(MAKE_DIRECTORY "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-src")
endif()
file(MAKE_DIRECTORY
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-build"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-subbuild/godot-sandbox-populate-prefix"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-subbuild/godot-sandbox-populate-prefix/tmp"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-subbuild/godot-sandbox-populate-prefix/src/godot-sandbox-populate-stamp"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-subbuild/godot-sandbox-populate-prefix/src"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-subbuild/godot-sandbox-populate-prefix/src/godot-sandbox-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-subbuild/godot-sandbox-populate-prefix/src/godot-sandbox-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/godot-sandbox-subbuild/godot-sandbox-populate-prefix/src/godot-sandbox-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
