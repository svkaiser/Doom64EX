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

# kexlib
add_include_directories("${CMAKE_SOURCE_DIR}/include/kexlib")
add_link_libraries(kexlib)

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
    i_cpu.c
    )
else ()
  add_sources(system
    i_cpu_posix.c
    )

  add_link_libraries(m) # math lib
endif ()

##------------------------------------------------------------------------------
## Sources
##

# src/automap
add_sources(automap
  am_draw.c
  am_map.c
  )

# src/common
add_sources(common
  doomdata.h
  doomdef.h
  doomstat.h
  doomtype.h
  info.c
  md5.c
  tables.c
  )

# src/console
add_sources(console
  con_console.c
  con_cvar.c
  )

# src/doom_main
add_sources(doom_main
  d_devstat.c
  d_englsh.h
  d_event.h
  d_keywds.h
  d_main.c
  d_net.c
  d_player.h
  d_think.h
  d_ticcmd.h
  )

# src/finale
add_sources(finale
  f_finale.c
  in_stuff.c
  )

# src/game
add_sources(game
  g_actions.c
  g_controls.h
  g_demo.c
  g_game.c
  g_local.h
  g_settings.c
  )

# src/intermission
add_sources(intermission
  wi_stuff.c
  )

# src/misc
add_sources(misc
  m_cheat.c
  m_fixed.c
  m_keys.c
  m_menu.c
  m_misc.cc
  m_password.c
  m_random.c
  m_shift.c
  )

# src/net
add_sources(net
  net_client.c
  net_common.c
  net_dedicated.c
  net_defs.h
  net_io.c
  net_loop.c
  net_packet.c
  net_query.c
  net_sdl.c
  net_server.c
  net_structrw.c
  )

# src/opengl
add_sources(opengl
  dgl.c
  gl_draw.c
  gl_main.c
  gl_texture.c
  glad.c
  )

# src/parser
add_sources(parser
  sc_main.c
  )

# src/playloop
add_sources(playloop
  p_ceilng.c
  p_doors.c
  p_enemy.c
  p_floor.c
  p_inter.c
  p_lights.c
  p_local.h
  p_macros.c
  p_map.c
  p_maputl.c
  p_mobj.c
  p_plats.c
  p_pspr.c
  p_saveg.c
  p_setup.c
  p_sight.c
  p_spec.c
  p_switch.c
  p_telept.c
  p_tick.c
  p_user.c
  )

# src/renderer
add_sources(renderer
  r_bsp.c
  r_clipper.c
  r_drawlist.c
  r_lights.c
  r_local.h
  r_main.c
  r_scene.c
  r_sky.cc
  r_things.c
  r_wipe.c
  )

# src/sound
add_sources(sound
  sounds.h
  s_sound.c
  )

# src/statusbar
add_sources(statusbar
  st_stuff.c
  )

# src/system
add_sources(system
  i_audio.c
  i_main.c
  i_png.cc
  i_swap.h
  i_system.c
  i_video.c
  )

# src/wadfile
add_sources(wadfile
  w_file.c
  w_merge.c
  w_wad.c
  )

# src/wadgen
add_sources(wadgen
  deflate-N64.c
  deflate-N64.h
  gfx.c
  gfx.h
  level.c
  level.h
  png.c
  rom.c
  rom.h
  sndfont.c
  sndfont.h
  sound.c
  sound.h
  sprite.c
  sprite.h
  texture.c
  texture.h
  wad.c
  wad.h
  wadgen.c
  wadgen.h
  )

# src/zone
add_sources(zone
  z_zone.c
  )

##------------------------------------------------------------------------------
## Build target
##

add_executable(doom64ex MACOSX_BUNDLE ${SOURCES})
target_include_directories(doom64ex PRIVATE ${INCLUDES} "${CMAKE_SOURCE_DIR}/include")
target_link_libraries(doom64ex ${LIBRARIES})
set_property(TARGET doom64ex PROPERTY CXX_STANDARD 14)

##------------------------------------------------------------------------------
## Install target
##

if (NOT WIN32)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/doom64ex DESTINATION bin)
endif ()
