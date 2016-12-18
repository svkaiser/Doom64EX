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

#ifndef _R_DRAWLIST_H_
#define _R_DRAWLIST_H_

#include "doomtype.h"
#include "doomdef.h"
#include "gl_main.h"
#include "r_things.h"

typedef enum {
    DLF_GLOW        = 0x1,
    DLF_WATER1      = 0x2,
    DLF_CEILING     = 0x4,
    DLF_MIRRORS     = 0x8,
    DLF_MIRRORT     = 0x10,
    DLF_WATER2      = 0x20
} drawlistflag_e;

typedef enum {
    DLT_WALL,
    DLT_FLAT,
    DLT_SPRITE,
    DLT_AMAP,
    NUMDRAWLISTS
} drawlisttag_e;

typedef struct {
    void        *data;
    dboolean(*callback)(void*, vtx_t*);
    dtexture    texid;
    int         flags;
    int         params;
} vtxlist_t;

typedef struct {
    vtxlist_t   *list;
    int         index;
    int         max;
} drawlist_t;

extern drawlist_t drawlist[NUMDRAWLISTS];

#define MAXDLDRAWCOUNT  0x10000
extern vtx_t drawVertex[MAXDLDRAWCOUNT];

dboolean DL_ProcessWalls(vtxlist_t* vl, int* drawcount);
dboolean DL_ProcessLeafs(vtxlist_t* vl, int* drawcount);
dboolean DL_ProcessSprites(vtxlist_t* vl, int* drawcount);

vtxlist_t *DL_AddVertexList(drawlist_t *dl);
int DL_GetDrawListSize(int tag);
void DL_BeginDrawList(dboolean t, dboolean a);
void DL_ProcessDrawList(int tag, dboolean(*procfunc)(vtxlist_t*, int*));
void DL_RenderDrawList(void);
void DL_Init(void);

#endif

