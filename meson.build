##------------------------------------------------------------------------------
##
## Doom64EX meson configuration
##
## LICENSE:
##
## Copyright(C) 2018 Zohar Malamant
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

project(
  'Doom64EX', 'cpp',
  version : '3.0.0-git',
  license : 'GPL',
  default_options : [
    'cpp_std=c++14'
  ]
)

cpp = meson.get_compiler('cpp')

##------------------------------------------------------------------------------
## Options
##
program_name = get_option('program_name')

##------------------------------------------------------------------------------
## Dependencies
##

# Threads
threads_dep = dependency('threads')

# SDL2 & co.
sdl2_dep = dependency('sdl2')
sdl2_net_dep = dependency('SDL2_net')

#
zlib_dep = dependency('zlib')
png_dep = dependency('libpng')

# GLBinding
glbinding_dep = dependency('glbinding', required : false)

if not glbinding_dep.found()
  # GLBinding doesn't have a .pc on Ubuntu
  glbinding_dep = cpp.find_library('glbinding')
endif

# Fluidsynth
fluid_dep = dependency('fluidsynth', required : false)

if not fluid_dep.found()
  fluid_dep = cpp.find_library('fluidsynth')
endif

##------------------------------------------------------------------------------
## Target: doom64ex.pk3 (data)
##
zip_prog = find_program('zip', required : false, native : true)
if zip_prog.found()
  res = run_command('./script/meson_build_pk3.sh')
  message(res.stdout())

  if res.returncode() == 0
    install_data(
      join_paths(meson.current_build_dir(), 'doom64ex.pk3'),
      install_dir : 'usr/share/doom64ex'
    )
  else
    warning('Could not create doom64ex.pk3. The game will not run without this file!')
  endif
else
  warning('`zip` program not found. Can\'t generate `doom64ex.pk3`')
endif

##------------------------------------------------------------------------------
## Target: doom64ex (engine)
##
subdir('src/engine')

executable(
  program_name,
  sources : engine_src,
  dependencies : engine_dep,
  include_directories : engine_inc,
  install : true
)

##------------------------------------------------------------------------------
## Install: misc data
##
icon_sizes = ['256x256']
data_dir = join_paths(get_option('prefix'), 'share')

foreach icon_size : icon_sizes
  install_data(
    join_paths('data/icons', icon_size, 'doom64ex.png'),
    install_dir : join_paths(data_dir, 'icons/hicolor', icon_size, 'apps'),
    rename : '@0@.png'.format(program_name)
  )
endforeach

desktop_data = configuration_data()
desktop_data.set('program_name', program_name)
configure_file(
  input : 'data/doom64ex.desktop.in',
  output : '@0@.desktop'.format(program_name),
  configuration : desktop_data
)

install_data(
  join_paths(meson.current_build_dir(), '@0@.desktop'.format(program_name)),
  install_dir : join_paths(data_dir, 'applications')
)

# install_data(
#   'data/doom64ex.6',
#   install_dir : 'usr/share/man'
# )

##------------------------------------------------------------------------------
## Postinstall
##
meson.add_install_script(
  'script/post_install.py',
  data_dir
)
