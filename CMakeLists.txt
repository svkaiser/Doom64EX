##------------------------------------------------------------------------------
##
## Doom64EX CMake configuration
##
## LICENSE:
##
## Copyright(C) 2016 Zohar Malamant
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
## 02111-1307, USA.
##

cmake_minimum_required(VERSION 2.8.12)

project(doom64ex C CXX)

##------------------------------------------------------------------------------
## Build Options
##

option(ENABLE_TESTING "Compile unit tests" ON)
option(ENABLE_SYSTEM_FLUIDSYNTH "Link with system-wide fluidsynth and not fluidsynth-lite" OFF)

# If fluidsynth-lite wasn't cloned, fall back to using system fluidsynth.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/fluidsynth/CMakeLists.txt" AND NOT ENABLE_SYSTEM_FLUIDSYNTH)
  set(ENABLE_SYSTEM_FLUIDSYNTH ON)
  message(WARNING "The fluidsynth submodule wasn't initialised. Run `git submodule --init --recursive` to use fluidsynth-lite, or add -DENABLE_SYSTEM_FLUIDSYNTH=ON to use the system-provided FluidSynth library.")
endif()

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" "${CMAKE_SOURCE_DIR}/extern")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}" "${CMAKE_FIND_ROOT_PATH}")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")

if(EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
  set(USE_CONAN ON)
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
else()
  set(USE_CONAN OFF)
endif()

##------------------------------------------------------------------------------
## Compiler Options
##

if(NOT MSVC)
  set(FLAGS "-Wall -Werror -Wno-unknown-pragmas -fno-strict-aliasing")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")
endif()

if(MINGW)
  set(CMAKE_EXE_LINKER_FLAGS "-static -static-libgcc -static-libstdc++ -Wl,--subsystem,windows")
endif(MINGW)

##------------------------------------------------------------------------------
## CMake functions
##

function(add_include_directories)
  unset(NEW_INCLUDES)
  foreach (ARG ${ARGN})
    list(APPEND NEW_INCLUDES ${ARG})
  endforeach ()
  set(INCLUDES ${INCLUDES} ${NEW_INCLUDES} PARENT_SCOPE)
endfunction()

function(add_sources PREFIX)
  unset(NEW_SOURCES)
  foreach (ARG ${ARGN})
    list(APPEND NEW_SOURCES "${SOURCE_ROOT_DIR}/${PREFIX}/${ARG}")
  endforeach ()
  set(SOURCES ${SOURCES} ${NEW_SOURCES} PARENT_SCOPE)
  set(INCLUDES ${INCLUDES} "${SOURCE_ROOT_DIR}/${PREFIX}" PARENT_SCOPE)
endfunction()

function(add_link_libraries)
  unset(NEW_LIBRARIES)
  foreach (ARG ${ARGN})
    list(APPEND NEW_LIBRARIES ${ARG})
  endforeach ()
  set(LIBRARIES ${LIBRARIES} ${NEW_LIBRARIES} PARENT_SCOPE)
endfunction()

##------------------------------------------------------------------------------
## Project-wide Dependencies
##

if(BUILD_TESTS)
  find_package(GTest)
endif(BUILD_TESTS)

##------------------------------------------------------------------------------
## Include subprojects
##

if(NOT ENABLE_SYSTEM_FLUIDSYNTH)
  add_subdirectory("${CMAKE_SOURCE_DIR}/fluidsynth")
endif()

add_subdirectory("${CMAKE_SOURCE_DIR}/src/engine")

##------------------------------------------------------------------------------
## doom64ex.pk3 target
##

# CMake 3.3.2 is the earliest version with support for --format=zip
if(CMAKE_VERSION VERSION_LESS 3.3.2)
  message(WARNING "Your CMake version is fairly old, so you'll need to generate the doom64ex.pk3 file manually. See README.md for more information.")
else()
  # Build doom64ex.pk3
  add_custom_target(pk3 ALL
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/distrib/doom64ex.pk3"
    COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_BINARY_DIR}/doom64ex.pk3" --format=zip "*")

  # Install doom64ex.pk3
  if (NOT WIN32)
    if (APPLE)
      install(FILES ${CMAKE_BINARY_DIR}/doom64ex.pk3 DESTINATION "$ENV{HOME}/Library/Application Support/doom64ex")
    else ()
      install(FILES ${CMAKE_BINARY_DIR}/doom64ex.pk3 DESTINATION share/games/doom64ex)
    endif ()
  endif ()
endif()
