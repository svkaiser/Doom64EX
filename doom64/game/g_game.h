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

#ifndef __G_GAME__
#define __G_GAME__

#include "doomdef.h"
#include "d_event.h"

extern dboolean sendpause;

//
// GAME
//

void G_Init(void);
void G_ReloadDefaults(void);
void G_SaveDefaults(void);
void G_DeathMatchSpawnPlayer(int playernum);
void G_InitNew(skill_t skill, int map);
void G_DeferedInitNew(skill_t skill, int map);
void G_LoadGame(const char* name);
void G_DoLoadGame(void);
void G_SaveGame(int slot, const char* description);
void G_DoSaveGame(void);
void G_CompleteLevel(void);
void G_ExitLevel(void);
void G_SecretExitLevel(int map);
void G_Ticker(void);
void G_ScreenShot(void);
void G_RunTitleMap(void);
void G_RunGame(void);
void G_RegisterCvars(void);

dboolean G_Responder(event_t* ev);

#endif
