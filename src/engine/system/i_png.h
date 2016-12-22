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

#ifndef __I_PNG_H__
#define __I_PNG_H__

#include "doomtype.h"

#include <imp/Image>

Image I_ReadImage(int lump, dboolean palette, dboolean nopack, double alpha, int palindex);

void *I_PNGReadData(int lump, dboolean palette, dboolean nopack, dboolean alpha,
                    int* w, int* h, int* offset, int palindex);

#endif // __I_PNG_H__
