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

set(INSTALL_DIR "${CMAKE_BINARY_DIR}")

##------------------------------------------------------------------------------
## Source URLs
##
set(SDL2_URL "https://www.libsdl.org/release/SDL2-2.0.4.tar.gz")
set(SDL2_MD5 "44fc4a023349933e7f5d7a582f7b886e")

set(SDL2_net_URL "https://www.libsdl.org/projects/SDL_net/release/SDL2_net-2.0.1.tar.gz")
set(SDL2_net_MD5 "5c1d9d1cfa63301b141cb5c0de2ea7c4")

set(SDL2_net_VC_URL "https://www.libsdl.org/projects/SDL_net/release/SDL2_net-devel-2.0.1-VC.zip")
set(SDL2_net_MINGW_URL "https://www.libsdl.org/projects/SDL_net/release/SDL2_net-devel-2.0.1-mingw.tar.gz")

set(PkgConfig_URL "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/pkg-config_0.23-3_win32.zip")

set(GLib_URL "http://ftp.gnome.org/pub/GNOME/sources/glib/2.24/glib-2.24.2.tar.gz")
set(GLib_MD5 "071b8e246197d334e20ab67956aef936")
set(GLib_BIN_URL "http://ftp.gnome.org/pub/GNOME/binaries/win32/glib/2.24/glib_2.24.2-2_win32.zip")
set(GLib_DEV_URL "http://ftp.gnome.org/pub/GNOME/binaries/win31/glib/2.24/glib-dev_2.24.2-2_win32.zip")

if(WIN32)
  set(proxy_libintl_URL "http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/proxy-libintl-dev_20090911_win32.zip")
endif()

set(FluidSynth_URL "https://sourceforge.net/projects/fluidsynth/files/fluidsynth-1.1.6/fluidsynth-1.1.6.tar.gz/download")
set(FluidSynth_MD5 "ae5aca6de824b4173667cbd3a310b263")

set(zlib_URL "http://zlib.net/zlib-1.2.8.tar.gz")
set(zlib_MD5 "44d667c142d7cda120332623eab69f40")

set(libpng_URL "http://download.sourceforge.net/libpng/libpng-1.6.21.tar.gz")
set(libpng_MD5 "aca36ec8e0a3b406a5912243bc243717")

##------------------------------------------------------------------------------
## SDL2
##
ExternalProject_Add(SDL2
  URL ${SDL2_URL}
  URL_MD5 ${SDL2_MD5}
  INSTALL_DIR ${INSTALL_DIR}

  # Only compile the shared lib
  CMAKE_ARGS -SDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_LDOPEN=ON -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

  # Audio is for chumps
  CMAKE_ARGS -DOSS=OFF -DALSA=OFF -DPULSEAUDIO=OFF -DSNDIO=OFF -DDSOUND=OFF -DDISKAUDIO=OFF -DDUMMYAUDIO=OFF
  CMAKE_ARGS -DFUSIONSOUND=OFF

  # Video is for suckers
  CMAKE_ARGS -DVIDEO_WAYLAND=OFF -DVIDEO_MIR=OFF -DVIDEO_RPI=OFF -DVIDEO_OPENGLES=OFF -DDIRECTX=OFF

  CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})

##------------------------------------------------------------------------------
## SDL2_net
##

if(WIN32)
  # SDL2_net doesn't have a CMakeLists.txt, so just download the binaries
  if(MSVC)
    ExternalProject_Add(SDL2_net
      URL ${SDL2_net_VC_URL}
      INSTALL_DIR ${INSTALL_DIR}

      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND
      COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/lib/x86 <INSTALL_DIR>/lib
      COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include <INSTALL_DIR>/include)
  elseif(MINGW)
    ExternalProject_Add(SDL2_net
      URL ${SDL2_net_MINGW_URL}
      INSTALL_DIR ${INSTALL_DIR}

      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND
      COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/i686-w64-mingw32/bin <INSTALL_DIR>/bin
      COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/i686-w64-mingw32/lib <INSTALL_DIR>/lib
      COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/i686-w64-mingw32/include <INSTALL_DIR>/include)
  endif()
else()
  # Build using Autotools
  ExternalProject_Add(SDL2_net
    URL ${SDL2_net_URL}
    URL_MD5 ${SDL2_net_MD5}
    DEPENDS SDL2
    INSTALL_DIR ${INSTALL_DIR}

    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --with-sdl-prefix=<INSTALL_DIR>)
endif()

##------------------------------------------------------------------------------
## GLib
##

if(WIN32)
  ExternalProject_Add(GLib_DEV
    URL ${GLib_DEV_URL}
    INSTALL_DIR ${INSTALL_DIR}

    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/lib <INSTALL_DIR>/lib
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include <INSTALL_DIR>/include)

  ExternalProject_Add(GLib_BIN
    URL ${GLib_BIN_URL}
    INSTALL_DIR ${INSTALL_DIR}

    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/bin <INSTALL_DIR>/bin)

  add_custom_target(GLib DEPENDS GLib_DEV GLib_BIN)
else()
  ExternalProject_Add(GLib
    URL ${GLib_URL}
    URL_MD5 ${GLib_MD5}
    INSTALL_DIR ${INSTALL_DIR}

    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>)
endif()

##------------------------------------------------------------------------------
## pkg-config
##

if(WIN32)
  ExternalProject_Add(PkgConfig
    URL ${PkgConfig_URL}
    DEPENDS GLib
    INSTALL_DIR ${INSTALL_DIR}

    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/bin <INSTALL_DIR>/bin)
endif()

##------------------------------------------------------------------------------
## proxy-libintl
##

if(WIN32)
  ExternalProject_Add(proxy_libintl
    URL ${proxy_libintl_URL}
    INSTALL_DIR ${INSTALL_DIR}

    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/include <INSTALL_DIR>/include
    COMMAND ${CMAKE_COMMAND} -E copy_directory <SOURCE_DIR>/lib <INSTALL_DIR>/lib)
endif()

##------------------------------------------------------------------------------
## FluidSynth
##
ExternalProject_Add(FluidSynth
  URL ${FluidSynth_URL}
  URL_MD5 ${FluidSynth_MD5}
  DEPENDS GLib # PkgConfig proxy_libintl

  INSTALL_DIR ${INSTALL_DIR}

  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

  # We don't need these audio drivers
  CMAKE_ARGS -Denable-libsndfile=OFF -Denable-aufile=OFF -Denable-jack=OFF -Denable-midishare=OFF
  CMAKE_ARGS -Denable-ladcca=OFF -Denable-lash=OFF
  CMAKE_ARGS -Denable-coremidi=OFF

  # A MIDI synth with IPv6 support... sure...
  CMAKE_ARGS -Denable-readline=OFF -Denable-dbus=OFF -Denable-ipv6=OFF

  # Fix Windows' stdio.h complaining about macro'd snprintf
  #PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/patched-config_win32.h <SOURCE_DIR>/src/config_win32.cmake

  CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_PREFIX_PATH=<INSTALL_DIR> -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON)

##------------------------------------------------------------------------------
## zlib
##
ExternalProject_Add(zlib
  URL ${zlib_URL}
  URL_MD5 ${zlib_MD5}
  INSTALL_DIR ${INSTALL_DIR}

  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>

  CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})

##------------------------------------------------------------------------------
## libpng
##
ExternalProject_Add(libpng
  URL ${libpng_URL}
  URL_MD5 ${libpng_MD5}
  DEPENDS zlib
  INSTALL_DIR ${INSTALL_DIR}

  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
  CMAKE_ARGS -DPNG_TESTS=OFF -DPNG_STATIC=OFF

  CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
