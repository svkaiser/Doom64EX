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


#ifndef __M_CHEAT__
#define __M_CHEAT__

#include "doomdef.h"
#include "g_game.h"
#include "doomstat.h"

//
// CHEAT SEQUENCE PACKAGE
//

void M_CheatProcess(player_t* plyr, event_t* ev);
void M_ParseNetCheat(int player, int type, char *buff);

void M_CheatGod(player_t* player, char dat[4]);
void M_CheatClip(player_t* player, char dat[4]);
void M_CheatKfa(player_t* player, char dat[4]);
void M_CheatGiveWeapon(player_t* player, char dat[4]);
void M_CheatArtifacts(player_t* player, char dat[4]);
void M_CheatBoyISuck(player_t* player, char dat[4]);
void M_CheatGiveKey(player_t* player, char dat[4]);

extern int        amCheating;

#endif
