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


#ifndef __W_WAD__
#define __W_WAD__


#ifdef __GNUG__
#pragma interface
#endif

#include "d_main.h"
#include "w_file.h"
#include "w_merge.h"

//
// WADFILE I/O related stuff.
//
typedef struct {
    char        name[8];
    wad_file_t* wadfile;
    int         position;
    int         size;
    int         next;
    int         index;
    void*       cache;
} lumpinfo_t;

extern lumpinfo_t* lumpinfo;
extern int numlumps;

void            W_Init(void);
wad_file_t*     W_AddFile(char *filename);
unsigned int    W_HashLumpName(const char* str);
int             W_CheckNumForName(const char* name);
int             W_GetNumForName(const char* name);
int             W_LumpLength(int lump);
void            W_ReadLump(int lump, void *dest);
void*           W_GetMapLump(int lump);
void            W_CacheMapLump(int map);
void            W_FreeMapLump(void);
int             W_MapLumpLength(int lump);
void*           W_CacheLumpNum(int lump, int tag);
void*           W_CacheLumpName(const char* name, int tag);




#endif
