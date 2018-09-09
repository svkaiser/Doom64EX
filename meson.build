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
  default_options : [ 'cpp_std=c++14' ]
)

cpp = meson.get_compiler('cpp')

##------------------------------------------------------------------------------
## Dependencies
##

# GLBinding
glbinding_dep = dependency('glbinding', required : false)

if not glbinding_dep.found()
  # GLBinding doesn't have a .pc on Ubuntu
  glbinding_dep = cpp.find_library('glbinding')
endif

# Fluidsynth
fluid_dep = dependency('fluidsynth')

##------------------------------------------------------------------------------
## Target: doom64ex.pk3 (data)
##
zip_prog = find_program('zip')
if zip_prog.found()
else
  warning('`zip` program not found. Can\'t generate `doom64ex.pk3`')
endif

##------------------------------------------------------------------------------
## Target: doom64ex (engine)
##
subdir('src/engine')
