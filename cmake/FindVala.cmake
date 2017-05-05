# FindVala.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# This file contains functions which can be used to integrate Vala
# compilation with CMake.  It is intended as a replacement for Jakob
# Westhoff's FindVala.cmake and UseVala.cmake.  It uses fast-vapis for
# faster parallel compilation, and per-target directories for
# generated sources to allow reusing source files across, even with
# different options.
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

set(VALAC_NAMES valac)

set(_FIND_VALA_CURRENT_VERSION 98)
while(_FIND_VALA_CURRENT_VERSION GREATER 0)
  list(APPEND VALAC_NAME "valac-1.${_FIND_VALA_CURRENT_VERSION}")
  math(EXPR _FIND_VALA_CURRENT_VERSION "${_FIND_VALA_CURRENT_VERSION} - 2")
endwhile()
set(_FIND_VALA_CURRENT_VERSION 98)
while(_FIND_VALA_CURRENT_VERSION GREATER 0)
  list(APPEND VALAC_NAME "valac-1.${_FIND_VALA_CURRENT_VERSION}")
  math(EXPR _FIND_VALA_CURRENT_VERSION "${_FIND_VALA_CURRENT_VERSION} - 2")
endwhile()
unset(_FIND_VALA_CURRENT_VERSION)

find_program(VALA_EXECUTABLE
  NAMES ${VALAC_NAMES})
mark_as_advanced(VALA_EXECUTABLE)

unset(VALAC_NAMES)

if(VALA_EXECUTABLE)
  # Determine valac version
  execute_process(COMMAND ${VALA_EXECUTABLE} "--version"
    OUTPUT_VARIABLE VALA_VERSION)
  string(REGEX REPLACE "^.*Vala ([0-9]+\\.[0-9]+\\.[0-9]+(\\.[0-9]+(\\-[0-9a-f]+)?)?).*$" "\\1" VALA_VERSION "${VALA_VERSION}")

  add_executable(valac IMPORTED)
  set_property(TARGET valac PROPERTY IMPORTED_LOCATION "${VALA_EXECUTABLE}")
endif(VALA_EXECUTABLE)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vala
    REQUIRED_VARS VALA_EXECUTABLE
    VERSION_VAR VALA_VERSION)

function(_vala_mkdir_for_file file)
  get_filename_component(dir "${file}" DIRECTORY)
  file(MAKE_DIRECTORY "${dir}")
endfunction()

macro(_vala_parse_source_file_path source)
  set (options)
  set (oneValueArgs SOURCE TYPE OUTPUT_PATH OUTPUT_DIR GENERATED_SOURCE FAST_VAPI)
  set (multiValueArgs)
  cmake_parse_arguments(VALAPATH "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  unset (options)
  unset (oneValueArgs)
  unset (multiValueArgs)

  if(VALAPATH_SOURCE)
    get_filename_component("${VALAPATH_SOURCE}" "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  if(VALAPATH_TYPE)
    string(REGEX MATCH "[^\\.]+$" "${VALAPATH_TYPE}" "${source}")
    string(TOLOWER "${${VALAPATH_TYPE}}" "${VALAPATH_TYPE}")
  endif()

  if(VALAPATH_OUTPUT_PATH OR VALAPATH_GENERATED_SOURCE OR VALAPATH_OUTPUT_DIR OR VALAPATH_FAST_VAPI)
    get_filename_component(srcfile "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

    string(LENGTH "${CMAKE_BINARY_DIR}" dirlen)
    string(SUBSTRING "${srcfile}" 0 ${dirlen} tmp)
    if("${CMAKE_BINARY_DIR}" STREQUAL "${tmp}")
      string(SUBSTRING "${srcfile}" ${dirlen} -1 tmp)
      set(outpath "build${tmp}")
    else()
      string(LENGTH "${CMAKE_SOURCE_DIR}" dirlen)
      string(SUBSTRING "${srcfile}" 0 ${dirlen} tmp)
      if("${CMAKE_SOURCE_DIR}" STREQUAL "${tmp}")
        string(SUBSTRING "${srcfile}" ${dirlen} -1 tmp)
        set(outpath "source${tmp}")
      else ()
        # TODO: this probably doesn't work correctly on Windows…
        set(outpath "root${tmp}")
      endif()
    endif()

    unset(tmp)
    unset(dirlen)
    unset(srcfile)
  endif()

  if(VALAPATH_OUTPUT_PATH)
    set("${VALAPATH_OUTPUT_PATH}" "${outpath}")
  endif()

  if(VALAPATH_GENERATED_SOURCE)
    string(REGEX REPLACE "\\.(vala|gs)$" ".c" "${VALAPATH_GENERATED_SOURCE}" "${outpath}")
  endif()

  if(VALAPATH_FAST_VAPI)
    string(REGEX REPLACE "\\.(vala|gs)$" ".vapi" "${VALAPATH_FAST_VAPI}" "${outpath}")
  endif()

  if(VALAPATH_OUTPUT_DIR)
    get_filename_component("${VALAPATH_OUTPUT_DIR}" "${outpath}" DIRECTORY)
  endif()

  unset(outpath)
endmacro()

# vala_precompile_target(
#   TARGET
#   GENERATED_SOURCES
#   SOURCES…
#   [VAPI vapi-name.vapi]
#   [GIR name-version.gir]
#   [HEADER name.h]
#   [FLAGS …]
#   [PACKAGES …]
#   [DEPENDS …])
#
# This function will use valac to generate C code.
#
# This function uses fast VAPIs to help improve parallelization and
# incremental build times.  The problem with this is that CMake
# doesn't allow file-level dependencies across directories; if you're
# generating code in one directory (for example, a library) and would
# like to use it in another directory and are building in parallel,
# the build can fail.  To prevent this, this function will create a
# ${TARGET}-vala top-level target (which *is* usable from other
# directories).
#
# Options:
#
#   TARGET
#     Target to create; it's generally best to make this similar to
#     your executable or library target name (e.g., for a "foo"
#     executable, "foo-vala" might be a good name), but not
#     technically required.
#   GENERATED_SOURCES
#     Variable in which to store the list of generated sources (which
#     you should pass to add_executable or add_library).
#   SOURCES
#     Vala sources to generate C from.  You should include *.vala,
#     *.gs, and uninstalled *.vapi files here; you may also include
#     C/C++ sources (they will simply be passed directly through to
#     the GENERATED_SOURCES variable).
#   VAPI name.vapi
#     If you would like to have valac generate a VAPI (basically, if
#     you are generating a library not an executable), pass the file
#     name here.
#   GIR name-version.gir
#     If you would like to have valac generate a GIR, pass the file
#     name here.
#   HEADER name.h
#     If you would like to have valac generate a C header, pass the
#     file name here.
#   FLAGS …
#     List of flags you wish to pass to valac.  They will be added to
#     the flags in VALA_COMPILER_FLAGS and VALA_COMPILER_FLAGS_DEBUG
#     (for Debug builds) or VALA_COMPILER_FLAGS_RELEASE (for Release
#     builds).
#   PACKAGES
#     List of dependencies to pass to valac.
#   DEPENDS
#     Any additional dependencies you would like.
macro(vala_precompile_target TARGET GENERATED_SOURCES)
  set (options)
  set (oneValueArgs VAPI GIR HEADER)
  set (multiValueArgs FLAGS PACKAGES DEPENDS)
  cmake_parse_arguments(VALAC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  unset (options)
  unset (oneValueArgs)
  unset (multiValueArgs)

  set(VALA_SOURCES)
  set(VALA_VAPIS)
  set(VALA_OUTPUT_SOURCES)

  if(VALAC_VAPI)
    list(APPEND non_source_out_files "${VALAC_VAPI}")
    list(APPEND non_source_valac_args "--vapi" "${VALAC_VAPI}")
  endif()

  if(VALAC_GIR)
    list(APPEND non_source_out_files "${VALAC_GIR}")
    list(APPEND non_source_valac_args
      "--gir" "${VALAC_GIR}"
      "--library" "${TARGET}"
      "--shared-library" "${CMAKE_SHARED_LIBRARY_PREFIX}${TARGET}${CMAKE_SHARED_LIBRARY_SUFFIX}")
  endif()

  if(VALAC_HEADER)
    list(APPEND non_source_out_files "${VALAC_HEADER}")
    list(APPEND non_source_valac_args --header "${VALAC_HEADER}")
  endif()

  # Split up the input files into three lists; one containing vala and
  # genie sources, one containing VAPIs, and one containing C files
  # (which may seem a bit silly, but it does open up the possibility
  # of declaring your source file list once instead of having separate
  # lists for Vala and C).
  foreach(source ${VALAC_UNPARSED_ARGUMENTS})
    _vala_parse_source_file_path("${source}"
      SOURCE source_full
      TYPE type)

    if("vala" STREQUAL "${type}" OR "gs" STREQUAL "${type}")
      list(APPEND VALA_SOURCES "${source_full}")
    elseif("vapi" STREQUAL "${type}")
      list(APPEND VALA_VAPIS "${source_full}")
    elseif(
        "c"   STREQUAL "${type}" OR
        "h"   STREQUAL "${type}" OR
        "cc"  STREQUAL "${type}" OR
        "cpp" STREQUAL "${type}" OR
        "cxx" STREQUAL "${type}" OR
        "hpp" STREQUAL "${type}")
      list(APPEND VALA_OUTPUT_SOURCES "${source_full}")
    endif()

    unset(type)
    unset(source_full)
  endforeach()

  # Set the common flags to pass to every valac invocation.
  set(VALAFLAGS ${VALAC_FLAGS} ${VALA_VAPIS})
  foreach(pkg ${VALAC_PACKAGES})
    list(APPEND VALAFLAGS "--pkg" "${pkg}")
  endforeach()
  list(APPEND VALAFLAGS ${VALA_COMPILER_FLAGS})
  if (CMAKE_BUILD_TYPE MATCHES "Debug")
    list(APPEND VALAFLAGS ${VALA_COMPILER_FLAGS_DEBUG})
  elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    list(APPEND VALAFLAGS ${VALA_COMPILER_FLAGS_RELEASE})
  endif()

  # Where to put the output
  set(TARGET_DIR "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}")

  set(FAST_VAPI_STAMPS)

  # Create fast VAPI targets for each vala source
  foreach(source ${VALA_SOURCES})
    _vala_parse_source_file_path("${source}"
      FAST_VAPI fast_vapi_path)

    # We need somewhere to put the output…
    _vala_mkdir_for_file("${TARGET_DIR}/${fast_vapi_path}")

    # Create the target
    add_custom_command(
      OUTPUT "${TARGET_DIR}/${fast_vapi_path}.stamp"
      BYPRODUCTS "${TARGET_DIR}/${fast_vapi_path}"
      DEPENDS
        "${source}"
        ${VALA_VAPIS}
        ${VALAC_DEPENDS}
      COMMAND "${VALA_EXECUTABLE}"
      ARGS
        "${source}"
        --fast-vapi "${TARGET_DIR}/${fast_vapi_path}"
        ${VALAFLAGS}
      COMMAND "${CMAKE_COMMAND}" ARGS -E touch "${TARGET_DIR}/${fast_vapi_path}.stamp"
      COMMENT "Generating fast VAPI ${TARGET_DIR}/${fast_vapi_path}")

    list(APPEND FAST_VAPI_STAMPS "${TARGET_DIR}/${fast_vapi_path}.stamp")

    unset(fast_vapi_path)
  endforeach()

  # Create a ${TARGET_DIR}-fast-vapis target which depens on all the fast
  # vapi stamps.  We can use this as a dependency to make sure all
  # fast-vapis are up to date.
  add_custom_command(
    OUTPUT "${TARGET_DIR}/fast-vapis.stamp"
    COMMAND "${CMAKE_COMMAND}" ARGS -E touch "${TARGET_DIR}/fast-vapis.stamp"
    DEPENDS
      ${FAST_VAPI_STAMPS}
      ${VALAC_DEPENDS}
    COMMENT "Generating fast VAPIs for ${TARGET}")

  add_custom_target("${TARGET}-fast-vapis"
    DEPENDS "${TARGET_DIR}/fast-vapis.stamp")

  set(VALA_GENERATED_SOURCE_STAMPS)

  # Add targets to generate C sources
  foreach(source ${VALA_SOURCES})
    _vala_parse_source_file_path("${source}"
      OUTPUT_PATH output_path
      OUTPUT_DIR output_dir
      GENERATED_SOURCE generated_source)

    set(use_fast_vapi_flags)
    foreach(src ${VALA_SOURCES})
      if(NOT "${src}" STREQUAL "${source}")
        _vala_parse_source_file_path("${src}"
          FAST_VAPI src_fast_vapi_path)

        list(APPEND use_fast_vapi_flags --use-fast-vapi "${TARGET_DIR}/${src_fast_vapi_path}")

        unset(src_fast_vapi_path)
      endif()
    endforeach()

    add_custom_command(
      OUTPUT "${TARGET_DIR}/${generated_source}.stamp"
      BYPRODUCTS "${TARGET_DIR}/${generated_source}"
      COMMAND "${VALA_EXECUTABLE}"
      ARGS
        -d "${TARGET_DIR}/${output_dir}"
        -C
        "${source}"
        ${VALAFLAGS}
        ${use_fast_vapi_flags}
      COMMAND "${CMAKE_COMMAND}" ARGS -E touch "${TARGET_DIR}/${generated_source}.stamp"
      DEPENDS
        "${TARGET}-fast-vapis"
        "${source}"
        ${VALA_VAPIS}
      COMMENT "Generating ${TARGET_DIR}/${generated_source}")
    unset(use_fast_vapi_flags)

    list(APPEND VALA_OUTPUT_SOURCES "${TARGET_DIR}/${generated_source}")
    list(APPEND VALA_GENERATED_SOURCE_STAMPS "${TARGET_DIR}/${generated_source}.stamp")

    unset(fast_vapi_path)
    unset(output_dir)
    unset(generated_source)
  endforeach()

  add_custom_command(
    OUTPUT "${TARGET_DIR}/stamp"
    COMMAND "${CMAKE_COMMAND}" ARGS -E touch "${TARGET_DIR}/stamp"
    DEPENDS ${VALA_GENERATED_SOURCE_STAMPS}
    COMMENT "Generating sources from Vala for ${TARGET}")

  set("${GENERATED_SOURCES}" ${VALA_OUTPUT_SOURCES})

  if(non_source_out_files)
    set(use_fast_vapi_flags)
    foreach(source ${VALA_SOURCES})
      _vala_parse_source_file_path("${source}"
        FAST_VAPI fast_vapi_path)

      list(APPEND use_fast_vapi_flags --use-fast-vapi "${TARGET_DIR}/${fast_vapi_path}")

      unset(fast_vapi_path)
    endforeach()

    add_custom_command(OUTPUT ${non_source_out_files}
      COMMAND ${VALA_EXECUTABLE}
      ARGS
        -C
        ${non_source_valac_args}
        ${VALAFLAGS}
        ${use_fast_vapi_flags}
      DEPENDS
        "${TARGET}-fast-vapis"
        ${VALA_VAPIS})
    unset(use_fast_vapi_flags)
  endif()

  # CMake doesn't allow file-level dependencies across directories, so
  # we provide a target we can depend on from other directories.
  add_custom_target("${TARGET}"
    DEPENDS
      "${TARGET_DIR}/stamp"
      ${non_source_out_files}
      ${VALAC_DEPENDS}
      ${VALA_GENERATED_SOURCE_STAMPS})

  unset(non_source_out_files)
  unset(non_source_valac_args)

  unset(VALA_GENERATED_SOURCE_STAMPS)
  unset(FAST_VAPI_STAMPS)
  unset(TARGET_DIR)
  unset(VALAFLAGS)
  unset(VALA_SOURCES)
  unset(VALA_VAPIS)
  unset(VALA_OUTPUT_SOURCES)
endmacro()
