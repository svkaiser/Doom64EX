// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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

#ifndef __I_SWAP_H__
#define __I_SWAP_H__

#include "SDL_endian.h"
#include "doomtype.h"

#define I_SwapLE16(x)   SDL_SwapLE16(x)
#define I_SwapLE32(x)   SDL_SwapLE32(x)
#define I_SwapBE16(x)   SDL_SwapBE16(x)
#define I_SwapBE32(x)   SDL_SwapBE32(x)

#define SHORT(x)        ((signed short)I_SwapLE16(x))
#define LONG(x)         ((signed long)I_SwapLE32(x))

// Defines for checking the endianness of the system.

#if SDL_BYTEORDER == SYS_LIL_ENDIAN
#define SYS_LITTLE_ENDIAN
#elif SDL_BYTEORDER == SYS_BIG_ENDIAN
#define SYS_BIG_ENDIAN
#endif

#endif // __I_SWAP_H__