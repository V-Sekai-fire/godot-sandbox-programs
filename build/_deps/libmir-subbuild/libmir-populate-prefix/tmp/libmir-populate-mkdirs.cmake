# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-src")
  file(MAKE_DIRECTORY "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-src")
endif()
file(MAKE_DIRECTORY
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-build"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-subbuild/libmir-populate-prefix"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-subbuild/libmir-populate-prefix/tmp"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-subbuild/libmir-populate-prefix/src/libmir-populate-stamp"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-subbuild/libmir-populate-prefix/src"
  "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-subbuild/libmir-populate-prefix/src/libmir-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-subbuild/libmir-populate-prefix/src/libmir-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/ernest.lee/Downloads/godot-sandbox-programs/build/_deps/libmir-subbuild/libmir-populate-prefix/src/libmir-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
