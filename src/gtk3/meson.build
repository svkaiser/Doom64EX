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

gnome = import('gnome')

##------------------------------------------------------------------------------
## Dependencies
##
nui_gtk3_dep = [
  dependency('gtk+-3.0'),
  dependency('glib-2.0'),
  dependency('gobject-2.0')
]

##------------------------------------------------------------------------------
## GReources
##
nui_gtk3_rsc = gnome.compile_resources(
  'nui_gtk3_rsc', 'resource/nui-gtk3.gresource.xml',
  source_dir : 'resource',
  c_name : 'nui_gtk3_rsc'
)

##------------------------------------------------------------------------------
## Sources
##
nui_gtk3_src = [
  'console-window.vala',
  'rom-dialog.vala',
  'main.vala',
  nui_gtk3_rsc
]

##------------------------------------------------------------------------------
## Library
##
nui_gtk3_lib = static_library(
  'nui_gtk3',
  sources : nui_gtk3_src,
  dependencies : nui_gtk3_dep
)

##------------------------------------------------------------------------------
## Install
##
nui_gtk3_dep = declare_dependency(
  link_whole : nui_gtk3_lib
)
