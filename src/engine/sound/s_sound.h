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


#ifndef __S_SOUND__
#define __S_SOUND__


#ifdef __GNUG__
#pragma interface
#endif

#include "p_mobj.h"
#include "sounds.h"

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer
//
void S_Init(void);

void S_SetSoundVolume(float volume);
void S_SetMusicVolume(float volume);
void S_SetGainOutput(float db);

void S_ResetSound(void);
void S_PauseSound(void);
void S_ResumeSound(void);

//
// Start sound for thing at <origin>
//  using <sound_id> from sounds.h
//
void S_StartSound(mobj_t* origin, int sound_id);

void S_UpdateSounds(void);
void S_RemoveOrigin(mobj_t* origin);

// Will start a sound at a given volume.
void S_StartSoundAtVolume(mobj_t* origin, int sound_id, int volume);

// Stop sound for thing at <origin>
void S_StopSound(mobj_t* origin, int sfx_id);

int S_GetActiveSounds(void);


// Start music using <music_id> from sounds.h
void S_StartMusic(int mnum);
void S_StopMusic(void);

void S_RegisterCvars(void);


#endif
