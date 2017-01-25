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

set(SOURCE_ROOT_DIR "${CMAKE_SOURCE_DIR}/src/engine")

if (CMAKE_VERSION VERSION_LESS 3.1.3)
  add_definitions(-std=c++14)
endif ()

##------------------------------------------------------------------------------
## Set up external dependencies
##

if (NOT USE_CONAN)
  # SDL2
  find_package(SDL2 REQUIRED)
  add_include_directories(${SDL2_INCLUDE_DIRS})
  add_link_libraries(${SDL2_LIBRARIES})

  # SDL2_Net
  find_package(SDL2_net REQUIRED)
  add_include_directories(${SDL2_NET_INCLUDE_DIRS})
  add_link_libraries(${SDL2_NET_LIBRARIES})

  # zlib
  find_package(ZLIB REQUIRED)

  # libpng
  find_package(PNG REQUIRED)
  add_include_directories(${PNG_INCLUDE_DIRS})
  add_link_libraries(${PNG_LIBRARIES})

  # FluidSynth
  find_package(FluidSynth REQUIRED)
  add_include_directories(${FLUIDSYNTH_INCLUDE_DIR})
  add_link_libraries(${FLUIDSYNTH_LIBRARIES})

  # OpenGL
  if(FORCE_INCLUDE_OPENGL)
    add_include_directories("/usr/include/GL/")
    add_link_libraries("/usr/lib32/libGL.so")
  else()
    find_package(OpenGL REQUIRED)
    add_include_directories(${OPENGL_INCLUDE_DIRS})
    add_link_libraries(${OPENGL_LIBRARIES})
  endif(FORCE_INCLUDE_OPENGL)
endif(NOT USE_CONAN)

##------------------------------------------------------------------------------
## Configure "version.h"
##

set(VERSION_MAJOR 2)
set(VERSION_MINOR 6)
set(VERSION_PATCH 0)
set(VERSION_GIT true)

##------------------------------------------------------------------------------
## Platform-specific sources
##

if (WIN32)
  add_sources(system/win32
    doom64.rc
    )

  add_sources(system
    i_cpu.cc
    )
else ()
  add_sources(system
    i_cpu_posix.cc
    )

  add_link_libraries(m) # math lib
endif ()

##------------------------------------------------------------------------------
## Sources
##

# src/automap
add_sources(automap
  am_draw.cc
  am_map.cc
  )

# src/common
add_sources(common
  doomdata.h
  doomdef.h
  doomstat.h
  doomtype.h
  info.cc
  md5.cc
  tables.cc
  )

# src/console
add_sources(console
  con_console.cc
  )

# src/doom_main
add_sources(doom_main
  d_devstat.cc
  d_englsh.h
  d_event.h
  d_keywds.h
  d_main.cc
  d_net.cc
  d_player.h
  d_think.h
  d_ticcmd.h
  )

# src/finale
add_sources(finale
  f_finale.cc
  in_stuff.cc
  )

# src/game
add_sources(game
  g_actions.cc
  g_controls.h
  g_demo.cc
  g_game.cc
  g_local.h
  g_settings.cc
  )

# src/intermission
add_sources(intermission
  wi_stuff.cc
  )

# src/misc
add_sources(misc
  m_cheat.cc
  m_fixed.cc
  m_keys.cc
  m_menu.cc
  m_misc.cc
  m_password.cc
  m_random.cc
  m_shift.cc
  )

# src/net
add_sources(net
  net_client.cc
  net_common.cc
  net_dedicated.cc
  net_defs.h
  net_io.cc
  net_loop.cc
  net_packet.cc
  net_query.cc
  net_sdl.cc
  net_server.cc
  net_structrw.cc
  )

# src/opengl
add_sources(opengl
  dgl.cc
  gl_draw.cc
  gl_main.cc
  gl_texture.cc
  glad.cc
  )

# src/parser
add_sources(parser
  sc_main.cc
  )

# src/playloop
add_sources(playloop
  p_ceilng.cc
  p_doors.cc
  p_enemy.cc
  p_floor.cc
  p_inter.cc
  p_lights.cc
  p_local.h
  p_macros.cc
  p_map.cc
  p_maputl.cc
  p_mobj.cc
  p_plats.cc
  p_pspr.cc
  p_saveg.cc
  p_setup.cc
  p_sight.cc
  p_spec.cc
  p_switch.cc
  p_telept.cc
  p_tick.cc
  p_user.cc
  )

# src/renderer
add_sources(renderer
  r_bsp.cc
  r_clipper.cc
  r_drawlist.cc
  r_lights.cc
  r_local.h
  r_main.cc
  r_scene.cc
  r_sky.cc
  r_things.cc
  r_wipe.cc
  )

# src/sound
add_sources(sound
  sounds.h
  s_sound.cc
  )

# src/statusbar
add_sources(statusbar
  st_stuff.cc
  )

# src/system
add_sources(system
  i_audio.cc
  i_main.cc
  i_png.cc
  i_swap.h
  i_system.cc
  i_video.cc
  )

# src/wadfile
add_sources(wadfile
  w_file.cc
  w_merge.cc
  w_wad.cc
  )

# src/wadgen
add_sources(wadgen
  deflate-N64.cc
  deflate-N64.h
  gfx.cc
  gfx.h
  level.cc
  level.h
  png.cc
  rom.cc
  rom.h
  sndfont.cc
  sndfont.h
  sound.cc
  sound.h
  sprite.cc
  sprite.h
  texture.cc
  texture.h
  wad.cc
  wad.h
  wadgen.cc
  wadgen.h
  )

# src/zone
add_sources(zone
  z_zone.cc
  )

# kexlib
set(KEXLIB_SOURCES
  gfx/Image.cc
  gfx/PngImage.cc
  gfx/DoomImage.cc
  gfx/Pixel.cc
  fmt/format.cc
  fmt/ostream.cc
  common/Property.cc
        )

##------------------------------------------------------------------------------
## Build target
##

add_executable(doom64ex MACOSX_BUNDLE ${SOURCES} ${KEXLIB_SOURCES})
target_include_directories(doom64ex PRIVATE ${INCLUDES} "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src/engine")
target_link_libraries(doom64ex ${LIBRARIES} ${CONAN_LIBS})
set_property(TARGET doom64ex PROPERTY CXX_STANDARD 14)

##------------------------------------------------------------------------------
## Install target
##

if (NOT WIN32)
  if (APPLE)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/doom64ex.app/Contents/MacOS/doom64ex DESTINATION bin)
  else ()
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/doom64ex DESTINATION bin)
  endif ()
endif ()
