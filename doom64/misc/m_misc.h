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


#ifndef __M_MISC__
#define __M_MISC__


#include "doomtype.h"
#include "m_fixed.h"
#include "r_local.h"

//
// MISC
//

#ifndef O_BINARY
#define O_BINARY 0
#endif

extern  int    myargc;
extern  char**    myargv;

// Returns the position of the given parameter
// in the arg list (0 if not found).
int M_CheckParm(char* check);

// Bounding box coordinate storage.
enum {
    BOXTOP,
    BOXBOTTOM,
    BOXLEFT,
    BOXRIGHT
};    // bbox coordinates

// Bounding box functions.
void M_ClearBox(fixed_t*    box);

void
M_AddToBox
(fixed_t*    box,
 fixed_t    x,
 fixed_t    y);

dboolean M_WriteFile(char const* name, void* source, int length);
int M_ReadFile(char const* name, byte** buffer);
void M_NormalizeSlashes(char *str);
int M_FileExists(char *filename);
long M_FileLength(FILE *handle);
dboolean M_WriteTextFile(char const* name, char* source, int length);
void M_ScreenShot(void);
int M_CacheThumbNail(byte** data);
void M_LoadDefaults(void);
void M_SaveDefaults(void);

//
// DEFAULTS
//
extern int        DualMouse;

extern int      viewwidth;
extern int      viewheight;

extern char*    chat_macros[];

//extern dboolean HighSound;

#endif
