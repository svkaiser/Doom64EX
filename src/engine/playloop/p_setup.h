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


#ifndef __P_SETUP__
#define __P_SETUP__


#ifdef __GNUG__
#pragma interface
#endif


// NOT called by W_Ticker. Fixme.
void P_SetupLevel(int map, int playermask, skill_t skill);

// Called by startup code.
void P_Init(void);

void P_RegisterCvars(void);

//
// [kex] mapinfo
//
mapdef_t* P_GetMapInfo(int map);
clusterdef_t* P_GetCluster(int map);

//
// [kex] sky definitions
//
typedef enum {
    SKF_CLOUD       = 0x1,
    SKF_THUNDER     = 0x2,
    SKF_FIRE        = 0x4,
    SKF_BACKGROUND  = 0x8,
    SKF_FADEBACK    = 0x10,
    SKF_VOID        = 0x20
} skyflags_e;

typedef struct {
    char        flat[9];
    int         flags;
    char        pic[9];
    char        backdrop[9];
    rcolor      fogcolor;
    rcolor      skycolor[3];
    int         fognear;
} skydef_t;

#endif
