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

set(SDL2_net_URL "https://www.libsdl.org/projects/SDL_net/release/SDL2_net-2.0.1.tar.gz")
set(SDL2_net_MD5 "5c1d9d1cfa63301b141cb5c0de2ea7c4")

set(GLib_URL "http://ftp.gnome.org/pub/GNOME/sources/glib/2.24/glib-2.24.2.tar.gz")
set(GLib_MD5 "071b8e246197d334e20ab67956aef936")

set(FluidSynth_URL "https://sourceforge.net/projects/fluidsynth/files/fluidsynth-1.1.6/fluidsynth-1.1.6.tar.gz/download")
set(FluidSynth_MD5 "ae5aca6de824b4173667cbd3a310b263")

set(libpng_URL "http://download.sourceforge.net/libpng/libpng-1.6.21.tar.gz")
set(libpng_MD5 "aca36ec8e0a3b406a5912243bc243717")

##------------------------------------------------------------------------------
## SDL2
##
set(SDL2_PREFIX "${SOURCE_DIR}/sdl2")

ExternalProject_Add(SDL2
  PREFIX ${SDL2_PREFIX}
  URL ${SDL2_URL}
  URL_MD5 ${SDL2_MD5}

  DOWNLOAD_DIR ${SDL2_PREFIX}/download

  INSTALL_DIR ${INSTALL_DIR}

  # Only compile the shared lib
  CMAKE_ARGS -DSDL_STATIC=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

  # Audio is for chumps
  CMAKE_ARGS -DOSS=OFF -DALSA=OFF -DPULSEAUDIO=OFF -DSNDIO=OFF -DDSOUND=OFF -DDISKAUDIO=OFF -DDUMMYAUDIO=OFF
  CMAKE_ARGS -DFUSIONSOUND=OFF

  # Video is for suckers
  CMAKE_ARGS -DVIDEO_WAYLAND=OFF -DVIDEO_MIR=OFF -DVIDEO_RPI=OFF -DDIRECTX=OFF
  )

##------------------------------------------------------------------------------
## SDL2_net
##
set(SDL2_net_PREFIX "${SOURCE_DIR}/sdl2_net")

ExternalProject_Add(SDL2_net
  PREFIX ${SDL2_net_PREFIX}
  URL ${SDL2_net_URL}
  URL_MD5 ${SDL2_net_MD5}

  DOWNLOAD_DIR ${SDL2_net_PREFIX}/download

  INSTALL_DIR ${INSTALL_DIR}

  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  )

##------------------------------------------------------------------------------
## GLib
##
set(GLib_PREFIX "${SOURCE_DIR}/glib")

ExternalProject_Add(GLib
  PREFIX ${GLib_PREFIX}
  URL ${GLib_URL}
  URL_MD5 ${GLib_MD5}

  DOWNLOAD_DIR ${GLib_PREFIX}/download

  INSTALL_DIR ${INSTALL_DIR}

  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  )

##------------------------------------------------------------------------------
## FluidSynth
##
set(FluidSynth_PREFIX "${SOURCE_DIR}/fluidsynth")

ExternalProject_Add(FluidSynth
  PREFIX ${FluidSynth_PREFIX}
  URL ${FluidSynth_URL}
  URL_MD5 ${FluidSynth_MD5}
  DEPENDS GLib

  DOWNLOAD_DIR ${FluidSynth_PREFIX}/download

  INSTALL_DIR ${INSTALL_DIR}

  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

  # We don't need these audio drivers
  CMAKE_ARGS -Denable-libsndfile=OFF -Denable-aufile=OFF -Denable-jack=OFF -Denable-midishare=OFF
  CMAKE_ARGS -Denable-ladcca=OFF -Denable-lash=OFF
  CMAKE_ARGS -Denable-coremidi=OFF

  # A MIDI synth with IPv6 support... sure...
  CMAKE_ARGS -Denable-readline=OFF -Denable-dbus=OFF -Denable-ipv6=OFF
  )

##------------------------------------------------------------------------------
## libpng
##
set(libpng_PREFIX "${SOURCE_DIR}/libpng")

ExternalProject_Add(libpng
  PREFIX ${libpng_PREFIX}
  URL ${libpng_URL}
  URL_MD5 ${libpng_MD5}

  DOWNLOAD_DIR ${libpng_PREFIX}/download

  INSTALL_DIR ${INSTALL_DIR}

  CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
  )