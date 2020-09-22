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
project(Doom64EX C CXX)

##------------------------------------------------------------------------------
## Build Options
##

option(ENABLE_TESTING "Compile unit tests" ON)
option(ENABLE_SYSTEM_FLUIDSYNTH "Link with system-wide fluidsynth and not fluidsynth-lite" OFF)
option(ENABLE_GTK3 "Display windows using GTK+3" OFF)
option(VERSION_DEV "Add git commit hash to window title" ON)

# If fluidsynth-lite wasn't cloned, fall back to using system fluidsynth.
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/fluidsynth/CMakeLists.txt" AND NOT ENABLE_SYSTEM_FLUIDSYNTH)
  set(ENABLE_SYSTEM_FLUIDSYNTH ON)
  message(WARNING "The fluidsynth submodule wasn't initialised. Run `git submodule --init --recursive` to use fluidsynth-lite, or add -DENABLE_SYSTEM_FLUIDSYNTH=ON to use the system-provided FluidSynth library.")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" "${CMAKE_SOURCE_DIR}/extern")
set(CMAKE_FIND_ROOT_PATH "${CMAKE_PREFIX_PATH}" "${CMAKE_FIND_ROOT_PATH}")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")
set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
  OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

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

if(NOT MSVC AND CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Debug mode: Enabling stricter warnings")
  set(FLAGS "-Wall -Wno-unknown-pragmas -Wno-undefined-var-template -fno-strict-aliasing")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAGS}")

  if(NOT MINGW)
    message(STATUS "Debug mode: Enabling AddressSanitizer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  endif()
endif()

if(MINGW)
  add_definitions(-DFMT_USE_WINDOWS_H=0)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows -static")
endif(MINGW)

# Add installation path as pre-processor definition
add_definitions(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

##------------------------------------------------------------------------------
## Configure config.hh
##

execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE VERSION_GIT
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(VERSION_MAJOR 3)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

if(VERSION_DEV)
  set(VERSION_FULL "${VERSION}-git+${VERSION_GIT}")
endif()

configure_file("${CMAKE_SOURCE_DIR}/src/config.hh.in" "${CMAKE_BINARY_DIR}/config/config.hh")

##------------------------------------------------------------------------------
## Dependencies
##

if(NOT USE_CONAN)
  # SDL2
  find_package(SDL2 REQUIRED)

  # SDL2_Net
  find_package(SDL2_net REQUIRED)

  # zlib
  find_package(ZLIB REQUIRED)

  # libpng
  find_package(PNG REQUIRED)

  # FluidSynth
  if(ENABLE_SYSTEM_FLUIDSYNTH)
    find_package(FluidSynth REQUIRED)
  endif()

  # OpenGL
  find_package(OpenGL REQUIRED)

  # GTK3
  if(ENABLE_GTK3)
    find_package(GTK3 REQUIRED)
    find_package(Vala REQUIRED)
  endif(ENABLE_GTK3)
endif(NOT USE_CONAN)

if(BUILD_TESTS)
  find_package(GTest)
endif(BUILD_TESTS)

##------------------------------------------------------------------------------
## Include subprojects
##

if(NOT ENABLE_SYSTEM_FLUIDSYNTH)
  add_subdirectory("${CMAKE_SOURCE_DIR}/fluidsynth" EXCLUDE_FROM_ALL)
endif()

add_subdirectory("${CMAKE_SOURCE_DIR}/src/engine")

if(ENABLE_GTK3)
  add_subdirectory("${CMAKE_SOURCE_DIR}/src/gtk3")
endif(ENABLE_GTK3)

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
    COMMAND "${CMAKE_COMMAND}" -E tar "cfv" "${CMAKE_BINARY_DIR}/doom64ex.pk3" --format=zip "--files-from=${CMAKE_SOURCE_DIR}/distrib/doom64ex_pk3.txt")

  # Install doom64ex.pk3
  if(NOT WIN32)
    if(APPLE)
      install(FILES "${CMAKE_BINARY_DIR}/doom64ex.pk3" DESTINATION "$ENV{HOME}/Library/Application Support/doom64ex")
    else()
      install(FILES "${CMAKE_BINARY_DIR}/doom64ex.pk3" DESTINATION "share/games/doom64ex")
    endif()
  endif()
endif()

##------------------------------------------------------------------------------
## Unix installation : manpage & launcher
##

include(Gzip)

if(UNIX AND NOT APPLE)
  compress_gz(-9 "${CMAKE_SOURCE_DIR}/distrib/doom64ex.6" "${CMAKE_BINARY_DIR}/distrib/doom64ex.6.gz")
  install(FILES "${CMAKE_BINARY_DIR}/distrib/doom64ex.6.gz" DESTINATION "share/man/man6")
  install(FILES "${CMAKE_SOURCE_DIR}/distrib/doom64ex.desktop" DESTINATION "share/applications")
  install(FILES "${CMAKE_SOURCE_DIR}/distrib/doom64ex.png" DESTINATION "share/icons")
endif()

##------------------------------------------------------------------------------
## Packages
##

set(CPACK_STRIP_FILES "games/doom64ex")
set(CPACK_PACKAGE_VERSION "${VERSION}")
set(CPACK_PACKAGE_FILE_NAME "doom64ex-${CPACK_PACKAGE_VERSION}-${CMAKE_HOST_SYSTEM_PROCESSOR}")
set(CPACK_PACKAGE_VENDOR "svkaiser")
set(CPACK_PACKAGE_CONTACT "svkaiser")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Doom64EX is a reverse-engineering project aimed to recreate Doom64 as close as possible with additional modding features.")

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://doom64ex.wordpress.com/")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_URL "https://doom64ex.wordpress.com/")
set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-2.0")

if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR TGZ TBZ2 TXZ DEB) # PKG for freebsd (cmake >= 3.13)
  find_program(RPMBUILD rpmbuild)
  if(RPMBUILD)
    message(STATUS "Found rpmbuild: ${RPMBUILD}")
	set(CPACK_GENERATOR ${CPACK_GENERATOR} RPM)
  else()
    message(STATUS "Not found rpmbuild: RPM package won't be generated")
  endif()
endif()

include(CPack)
