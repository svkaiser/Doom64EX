##------------------------------------------------------------------------------
##
## Doom64EX External Dependencies CMake configuration
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

cmake_minimum_required(VERSION 3.4)

project(Doom64EX_Extern)

include(ExternalProject)

set(SOURCE_DIR "${CMAKE_BINARY_DIR}/source")
set(INSTALL_DIR "${CMAKE_BINARY_DIR}")

##------------------------------------------------------------------------------
## Source URLs
##
set(SDL2_URL "https://www.libsdl.org/release/SDL2-2.0.4.tar.gz")
set(SDL2_MD5 "44fc4a023349933e7f5d7a582f7b886e")

##------------------------------------------------------------------------------
## SDL2
##
set(SDL2_PREFIX "${SOURCE_DIR}/sdl2")

ExternalProject_Add(SDL2
  PREFIX ${SDL2_PREFIX}
  URL ${SDL2_URL}
  URL_MD5 ${SDL2_MD5}

  DOWNLOAD_DIR ${SDL2_PREFIX}/download

  INSTALL_DIR ${INTALL_DIR}

  # Only compile the shared lib
  CMAKE_ARGS -DSDL_STATIC=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

  # Audio is for chumps
  CMAKE_ARGS -DOSS=OFF -DALSA=OFF -DPULSEAUDIO=OFF -DSNDIO=OFF -DDSOUND=OFF -DDISKAUDIO=OFF -DDUMMYAUDIO=OFF
  CMAKE_ARGS -DFUSIONSOUND=OFF

  # Video is for suckers
  CMAKE_ARGS -DVIDEO_WAYLAND=OFF -DVIDEO_MIR=OFF -DVIDEO_RPI=OFF -DDIRECTX=OFF
  )