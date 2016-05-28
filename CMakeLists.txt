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

cmake_minimum_required(VERSION 2.8.11)

project(doom64ex C CXX)

set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/extern/build")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")

set(CMAKE_CXX_FLAGS -std=c++14)

##------------------------------------------------------------------------------
## Options
##

option(KEXWAD "Generate kex.wad" ON)
option(ENGINE "Build Doom64EX" ON)
option(TESTING "Build unit tests" OFF)

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
## Include subprojects
##

if (TESTING)
  enable_testing()
endif()

if (KEXWAD)
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/wadtool")
endif()

if (ENGINE)
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/kexlib")
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/engine")
endif()