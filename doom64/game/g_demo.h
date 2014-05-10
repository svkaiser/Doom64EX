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

#ifndef __G_DEMO_H__
#define __G_DEMO_H__

#define DEMOMARKER      0x80

dboolean G_CheckDemoStatus(void);

void G_RecordDemo(const char* name);
void G_PlayDemo(const char* name);
void G_ReadDemoTiccmd(ticcmd_t* cmd);
void G_WriteDemoTiccmd(ticcmd_t* cmd);

extern char             demoname[256];  // name of demo lump
extern dboolean         demorecording;  // currently recording a demo
extern dboolean         demoplayback;   // currently playing a demo
extern dboolean         netdemo;
extern byte*            demobuffer;
extern byte*            demo_p;
extern byte*            demoend;
extern dboolean         singledemo;
extern dboolean         endDemo;        // signal recorder to stop on next tick
extern dboolean         iwadDemo;       // hide hud, end playback after one level

#endif
