# FindGObjectIntrospection.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# License:
#
#   Copyright (c) 2016 Evan Nemerson <evan@nemerson.com>
#
#   Permission is hereby granted, free of charge, to any person
#   obtaining a copy of this software and associated documentation
#   files (the "Software"), to deal in the Software without
#   restriction, including without limitation the rights to use, copy,
#   modify, merge, publish, distribute, sublicense, and/or sell copies
#   of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.

find_program(GObjectIntrospection_COMPILER_EXECUTABLE g-ir-compiler)
find_program(GObjectIntrospection_SCANNER_EXECUTABLE g-ir-scanner)

if(CMAKE_INSTALL_FULL_DATADIR)
  set(GObjectIntrospection_REPOSITORY_DIR "${CMAKE_INSTALL_FULL_DATADIR}/gir-1.0")
else()
  set(GObjectIntrospection_REPOSITORY_DIR "${CMAKE_INSTALL_PREFIX}/share/gir-1.0")
endif()

if(CMAKE_INSTALL_FULL_LIBDIR)
  set(GObjectIntrospection_TYPELIB_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/girepository-1.0")
else()
  set(GObjectIntrospection_TYPELIB_DIR "${CMAKE_INSTALL_LIBDIR}/girepository-1.0")
endif()

if(GObjectIntrospection_COMPILER_EXECUTABLE)
  # Imported target
  add_executable(g-ir-compiler IMPORTED)
  set_property(TARGET g-ir-compiler PROPERTY IMPORTED_LOCATION "${GObjectIntrospection_COMPILER_EXECUTABLE}")
endif()

if(GObjectIntrospection_SCANNER_EXECUTABLE)
  # Imported target
  add_executable(g-ir-scanner IMPORTED)
  set_property(TARGET g-ir-scanner PROPERTY IMPORTED_LOCATION "${GObjectIntrospection_SCANNER_EXECUTABLE}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GObjectIntrospection
  REQUIRED_VARS
    GObjectIntrospection_COMPILER_EXECUTABLE
    GObjectIntrospection_SCANNER_EXECUTABLE)

function(gobject_introspection_compile TYPELIB)
  set (options DEBUG VERBOSE)
  set (oneValueArgs MODULE SHARED_LIBRARY)
  set (multiValueArgs FLAGS INCLUDE_DIRS)
  cmake_parse_arguments(GObjectIntrospection_COMPILER "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  unset (options)
  unset (oneValueArgs)
  unset (multiValueArgs)

  get_filename_component(TYPELIB "${TYPELIB}" ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

  if(GObjectIntrospection_COMPILER_DEBUG)
    list(APPEND GObjectIntrospection_COMPILER_FLAGS "--debug")
  endif()

  if(GObjectIntrospection_COMPILER_VERBOSE)
    list(APPEND GObjectIntrospection_COMPILER_FLAGS "--verbose")
  endif()

  if(GObjectIntrospection_SHARED_LIBRARY)
    list(APPEND GObjectIntrospection_COMPILER_FLAGS "--shared-library" "${GObjectIntrospection_SHARED_LIBRARY}")
  endif()

  foreach(include_dir ${GObjectIntrospection_COMPILER_INCLUDE_DIRS})
    list(APPEND GObjectIntrospection_COMPILER_FLAGS "--includedir" "${include_dir}")
  endforeach()

  add_custom_command(
    OUTPUT "${TYPELIB}"
    COMMAND g-ir-compiler
    ARGS
      "-o" "${TYPELIB}"
      ${GObjectIntrospection_COMPILER_FLAGS}
      ${GObjectIntrospection_COMPILER_UNPARSED_ARGUMENTS}
    DEPENDS
      ${GObjectIntrospection_COMPILER_UNPARSED_ARGUMENTS})
endfunction()
