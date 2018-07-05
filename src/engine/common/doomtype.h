// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2007-2012 Samuel Villarreal
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#include <prelude.hh>

#ifdef _WIN32
#include "SDL_config.h"
#endif

typedef bool            dboolean;
typedef unsigned short  word;  // TODO: Rename every instance of 'word' to 'uint16'.
                               // 'Word' isn't the correct term unless you're writing for 16-bit systems, which we're not.
typedef unsigned int    dword; // TODO: ditto, but 'dword' -> 'uint32'

#include <limits.h>
#define D_MININT INT_MIN
#define D_MAXINT INT_MAX
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef BETWEEN
#define BETWEEN(l,u,x) ((l)>(x)?(l):(x)>(u)?(u):(x))
#endif

#ifdef _WIN32

#define DIR_SEPARATOR '\\'
#define PATH_SEPARATOR ';'

#else

#define DIR_SEPARATOR '/'
#define PATH_SEPARATOR ':'

#endif


