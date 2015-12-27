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

cmake_minimum_required(VERSION 2.6)

project(doom64ex C)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")

##------------------------------------------------------------------------------
## Options
##

option(BUILD_PWAD "Generate kexdata.wad" ON)
option(BUILD_ENGINE "Build Doom64EX" ON)

##------------------------------------------------------------------------------
## Include subprojects
##

if (BUILD_PWAD)
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/wadtool")

  # generate kexdata.wad using wadtool
  add_custom_target(kexdata
    COMMAND "${WADTOOL_BINARY}" "${DATA_DIR}/kexdata" "${CMAKE_BINARY_DIR}/kexdata.wad"
    DEPENDS wadtool
    )
endif()

if (BUILD_ENGINE)
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/engine")
endif()