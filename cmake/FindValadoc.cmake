# FindValadoc.cmake
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

find_program(VALADOC_EXECUTABLE
  NAMES valadoc
  DOC "Valadoc")

if(VALADOC_EXECUTABLE)
  # Get valadoc version
  execute_process(COMMAND ${VALADOC_EXECUTABLE} "--version" OUTPUT_VARIABLE VALADOC_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(SUBSTRING "${VALADOC_VERSION}" 8 -1 VALADOC_VERSION)

  # Imported target
  add_executable(valadoc IMPORTED)
  set_property(TARGET valadoc PROPERTY IMPORTED_LOCATION "${VALADOC_EXECUTABLE}")
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Valadoc
  FOUND_VAR VALADOC_FOUND
  REQUIRED_VARS VALADOC_EXECUTABLE
  VERSION_VAR VALADOC_VERSION)

# valadoc_generate(OUTPUT_DIR
#   PACKAGE_NAME name
#   [PACKAGE_VERSION version]
#   [FLAGS flags…]
#   [PACKAGES packages…]
#   [DOCLET doclet]
#   [ALL])
#
#   PACKAGE_NAME name
#     VAPI name to generate documentation for.
#   PACKAGE_VERSION version
#     Version number of the package.
#   FLAGS …
#     List of flags you wish to pass to valadoc.
#   PACKAGES
#     List of dependencies to pass to valac.
#   DOCLET doclet-name
#     Name of the doclet to use (default: html)
function(valadoc_generate OUTPUT_DIR)
  set (options)
  set (oneValueArgs DOCLET PACKAGE_NAME PACKAGE_VERSION)
  set (multiValueArgs SOURCES PACKAGES FLAGS DEFINITIONS CUSTOM_VAPIS)
  cmake_parse_arguments(VALADOC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  unset (options)
  unset (oneValueArgs)
  unset (multiValueArgs)

  if("${VALADOC_DOCLET}" STREQUAL "")
    list(APPEND VALADOC_FLAGS "--doclet=html")
  else()
    list(APPEND VALADOC_FLAGS "--doclet=${VALADOC_DOCLET}")
  endif()

  if(NOT "${VALADOC_PACKAGE_NAME}" STREQUAL "")
    list(APPEND VALADOC_FLAGS "--package-name=${VALADOC_PACKAGE_NAME}")
  endif()

  if(NOT "${VALADOC_PACKAGE_VERSION}" STREQUAL "")
    list(APPEND VALADOC_FLAGS "--package-version=${VALADOC_PACKAGE_VERSION}")
  endif()

  foreach(pkg ${VALADOC_PACKAGES})
    list(APPEND VALADOC_FLAGS "--pkg=${pkg}")
  endforeach(pkg)

  add_custom_command(
    OUTPUT "${OUTPUT_DIR}"
    COMMAND valadoc
    ARGS
      --force
      -o "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_DIR}"
      ${VALADOC_FLAGS}
      ${VALADOC_SOURCES}
    DEPENDS
      ${VALADOC_SOURCES}
    COMMENT "Generating documentation with Valadoc"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endfunction()
