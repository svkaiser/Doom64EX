##------------------------------------------------------------------------------
##
## Doom64EX CMake configuration
##
## LICENSE:
##
## Copyright(C) 2017 Zohar Malamant
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
## Sources
##

set(SOURCES
  console.vala
  thread.vala)

##------------------------------------------------------------------------------
## Build target
##

vala_precompile_target(${PROJECT_NAME}-gtk3-vala
  SOURCES
  ${SOURCES}

  PACKAGES
  "gtk+-3.0")

add_library("${PROJECT_NAME}-gtk3" SHARED ${SOURCES})
add_dependencies("${PROJECT_NAME}-gtk3" "${PROJECT_NAME}-gtk3-vala")
set_property(TARGET "${PROJECT_NAME}-gtk3" PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries("${PROJECT_NAME}-gtk3"
  ${GTK3_LIBRARY}
  ${GIO_LIBRARY}
  ${GObject_LIBRARY}
  ${GLib_LIBRARY})

target_include_directories("${PROJECT_NAME}-gtk3" PUBLIC
  ${GTK3_INCLUDE_DIRS})

##------------------------------------------------------------------------------
## Install target
##
