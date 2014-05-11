##------------------------------------------------------------------------------
##
## Doom64EX CMake configuration
##
## LICENSE:
##
## Copyright(C) 2014 Zohar Malamant
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

project(doom64ex)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(SOURCE_ROOT_DIR "${CMAKE_SOURCE_DIR}/src")

##------------------------------------------------------------------------------
## CMake functions
##

function(add_sources PREFIX)
	unset(NEW_SOURCES)
	foreach(ARG ${ARGN})
		list(APPEND NEW_SOURCES "${SOURCE_ROOT_DIR}/${PREFIX}/${ARG}")
	endforeach()
	set(SOURCES ${SOURCES} ${NEW_SOURCES} PARENT_SCOPE)
	include_directories("${SOURCE_ROOT_DIR}/${PREFIX}")
endfunction()

function(add_link_libraries)
	unset(NEW_LIBRARIES)
	foreach(ARG ${ARGN})
		list(APPEND NEW_LIBRARIES ${ARG})
	endforeach()
	set(LIBRARIES ${LIBRARIES} ${NEW_LIBRARIES} PARENT_SCOPE)
endfunction()

##------------------------------------------------------------------------------
## Set up external dependencies
##


# SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
add_link_libraries(${SDL2_LIBRARIES})

# SDL2_Net
find_package(SDL2_net REQUIRED)
include_directories(${SDL2_NET_INCLUDE_DIRS})
add_link_libraries(${SDL2_NET_LIBRARIES})

# zlib
find_package(ZLIB REQUIRED)

# libpng
find_package(PNG REQUIRED)
include_directories(${PNG_INCLUDE_DIRS})
add_link_libraries(${PNG_LIBRARIES})

# FluidSynth
find_package(FluidSynth REQUIRED)
include_directories(${FLUIDSYNTH_INCLUDE_DIRS})
add_link_libraries(${FLUIDSYNTH_LIBRARIES})

# OpenGL
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})
add_link_libraries(${OPENGL_LIBRARIES})

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
	m_misc.c
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
	r_sky.c
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
	i_cpu_posix.c
	i_main.c
	i_png.c
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

# src/zone
add_sources(zone
	z_zone.c
)

##------------------------------------------------------------------------------
## Build target
##

include_directories(${SOURCE_ROOT_DIR})
add_executable(doom64ex ${SOURCES})
target_link_libraries(doom64ex ${LIBRARIES} m)

##------------------------------------------------------------------------------
## Install target
##

