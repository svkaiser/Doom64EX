##------------------------------------------------------------------------------
##
## Doom64EX CMake configuration
##
## LICENSE:
##
## Copyright(C) 2014-2016 Zohar Malamant
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

##------------------------------------------------------------------------------
## Set up external dependencies
##

# libpng
find_package(PNG REQUIRED)
add_include_directories(${PNG_INCLUDE_DIRS})
add_link_libraries(${PNG_LIBRARIES})

##------------------------------------------------------------------------------
## Sources
##

# We're lazy, so add all source files
file(GLOB_RECURSE C_SOURCES "${CMAKE_SOURCE_DIR}/src/kexlib/*.c")
file(GLOB_RECURSE C_HEADERS "${CMAKE_SOURCE_DIR}/src/kexlib/*.h")
file(GLOB_RECURSE CC_SOURCES "${CMAKE_SOURCE_DIR}/src/kexlib/*.cc")

##------------------------------------------------------------------------------
## Build target
##

add_library(kexlib STATIC ${C_SOURCES} ${C_HEADERS} ${CC_SOURCES})
target_include_directories(kexlib PRIVATE ${INCLUDES} "${CMAKE_SOURCE_DIR}/include/kexlib")
target_link_libraries(kexlib m ${LIBRARIES})

##------------------------------------------------------------------------------
## Build tests
##

if (TESTING)
  find_package(GTest REQUIRED)

  file(GLOB_RECURSE TEST_SOURCES "${CMAKE_SOURCE_DIR}/test/kexlib/*.cc")

  add_executable(kexlib_test ${TEST_SOURCES})
  target_include_directories(kexlib_test PRIVATE ${INCLUDES} "${CMAKE_SOURCE_DIR}/include/kexlib" ${GTEST_INCLUDE_DIRS})
  target_link_libraries(kexlib_test kexlib ${GTEST_BOTH_LIBRARIES})

  add_test(AllTestsInKexlib COMMAND kexlib_test)

  file(COPY "${DATA_DIR}/kexlib_test" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/data/")
endif()