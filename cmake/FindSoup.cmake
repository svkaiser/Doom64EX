# FindSoup.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for libsoup.
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

find_package(PkgConfig)

set(Soup_DEPS
  GIO)

if(PKG_CONFIG_FOUND)
  pkg_search_module(Soup_PKG libsoup-2.4)
endif()

find_library(Soup_LIBRARY soup-2.4 HINTS ${Soup_PKG_LIBRARY_DIRS})
set(Soup soup-2.4)

if(Soup_LIBRARY AND NOT Soup_FOUND)
  add_library(${Soup} SHARED IMPORTED)
  set_property(TARGET ${Soup} PROPERTY IMPORTED_LOCATION "${Soup_LIBRARY}")
  set_property(TARGET ${Soup} PROPERTY INTERFACE_COMPILE_OPTIONS "${Soup_PKG_CFLAGS_OTHER}")

  find_path(Soup_INCLUDE_DIR "libsoup/soup.h"
    HINTS ${Soup_PKG_INCLUDE_DIRS})

  if(Soup_INCLUDE_DIR)
    file(STRINGS "${Soup_INCLUDE_DIR}/libsoup/soup-version.h" Soup_MAJOR_VERSION REGEX "^#define SOUP_MAJOR_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define SOUP_MAJOR_VERSION \\(([0-9]+)\\)$" "\\1" Soup_MAJOR_VERSION "${Soup_MAJOR_VERSION}")
    file(STRINGS "${Soup_INCLUDE_DIR}/libsoup/soup-version.h" Soup_MINOR_VERSION REGEX "^#define SOUP_MINOR_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define SOUP_MINOR_VERSION \\(([0-9]+)\\)$" "\\1" Soup_MINOR_VERSION "${Soup_MINOR_VERSION}")
    file(STRINGS "${Soup_INCLUDE_DIR}/libsoup/soup-version.h" Soup_MICRO_VERSION REGEX "^#define SOUP_MICRO_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define SOUP_MICRO_VERSION \\(([0-9]+)\\)$" "\\1" Soup_MICRO_VERSION "${Soup_MICRO_VERSION}")
    set(Soup_VERSION "${Soup_MAJOR_VERSION}.${Soup_MINOR_VERSION}.${Soup_MICRO_VERSION}")
    unset(Soup_MAJOR_VERSION)
    unset(Soup_MINOR_VERSION)
    unset(Soup_MICRO_VERSION)

    list(APPEND Soup_INCLUDE_DIRS ${Soup_INCLUDE_DIR})
    set_property(TARGET ${Soup} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${Soup_INCLUDE_DIR}")
  endif()
endif()

set(Soup_DEPS_FOUND_VARS)
foreach(soup_dep ${Soup_DEPS})
  find_package(${soup_dep})

  list(APPEND Soup_DEPS_FOUND_VARS "${soup_dep}_FOUND")
  list(APPEND Soup_INCLUDE_DIRS ${${soup_dep}_INCLUDE_DIRS})

  set_property (TARGET "${Soup}" APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${soup_dep}}")
endforeach(soup_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Soup
    REQUIRED_VARS
      Soup_LIBRARY
      Soup_INCLUDE_DIRS
      ${Soup_DEPS_FOUND_VARS}
    VERSION_VAR
      Soup_VERSION)

unset(Soup_DEPS_FOUND_VARS)
