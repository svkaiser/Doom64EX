// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 1997 Midway Home Entertainment, Inc
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

#ifndef D3DR_SKY_H
#define D3DR_SKY_H

#include "p_setup.h"

extern skydef_t*    sky;
extern int          skypicnum;
extern int          skybackdropnum;
extern int          thunderCounter;
extern int          lightningCounter;
extern int          thundertic;
extern dboolean     skyfadeback;

// Needed to store the number of the dummy sky flat.
// Used for rendering, as well as tracking projectiles etc.
extern int          skyflatnum;

extern byte*        fireBuffer;
extern dPalette_t   firePal16[256];
extern int          fireLump;

void R_SkyTicker(void);
void R_DrawSky(void);
void R_InitFire(void);

#endif