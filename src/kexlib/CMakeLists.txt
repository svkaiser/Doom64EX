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

# CMake 3.1.3 added the CXX_STANDARD property, but Debian is still on 3.0
if (CMAKE_VERSION VERSION_LESS 3.1.3)
  add_definitions(-std=c++14)
endif ()

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

set(SOURCES lib.cc
  format.cc
  gfx/Image.cc
  gfx/PngImage.cc
  gfx/Pixel.cc
  compat/C_gfx.cc)

##------------------------------------------------------------------------------
## Build target
##

add_library(kexlib STATIC ${SOURCES})
target_include_directories(kexlib PRIVATE ${INCLUDES} "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(kexlib m ${LIBRARIES})
set_target_properties(kexlib PROPERTIES CXX_STANDARD 14)

##------------------------------------------------------------------------------
## Build tests
##

if (TESTING)
  find_package(GTest REQUIRED)

    set(TEST_SOURCES
            lib_test.cc

      gfx/Image_test.cc
      gfx/PngImage_test.cc gfx/Pixel_test.cc)

  add_executable(kexlib_test ${TEST_SOURCES})
  target_include_directories(kexlib_test PRIVATE ${INCLUDES} "${CMAKE_SOURCE_DIR}/include" ${GTEST_INCLUDE_DIRS})
  target_link_libraries(kexlib_test kexlib ${GTEST_LIBRARIES})

  add_test(AllTestsInKexlib COMMAND kexlib_test)

  file(COPY "${DATA_DIR}/kexlib_test" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/data/")
endif()
