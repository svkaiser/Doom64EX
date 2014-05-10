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

#ifndef _R_THINGS_H_
#define _R_THINGS_H_

#include "doomtype.h"
#include "doomdef.h"
#include "t_bsp.h"
#include "d_player.h"
#include "gl_main.h"

typedef struct {
    mobj_t* spr;
    fixed_t dist;
    float   x;
    float   y;
    float   z;
} visspritelist_t;

void R_InitSprites(char** namelist);
void R_AddSprites(subsector_t *sub);
void R_SetupSprites(void);
void R_ClearSprites(void);
void R_RenderPlayerSprites(player_t *player);
void R_DrawThingBBox(void);

extern spritedef_t    *spriteinfo;
extern int            numsprites;

#endif