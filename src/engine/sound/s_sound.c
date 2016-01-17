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
//
// DESCRIPTION: In-game Sound behavior
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "sounds.h"
#include "s_sound.h"
#include "z_zone.h"
#include "m_fixed.h"
#include "m_random.h"
#include "w_wad.h"
#include "doomdef.h"
#include "p_local.h"
#include "doomstat.h"
#include "tables.h"
#include "r_local.h"
#include "m_misc.h"
#include "p_setup.h"
#include "i_audio.h"
#include "con_console.h"

// Adjustable by menu.
#define NORM_VOLUME     127
#define NORM_SEP        128

// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST (1700<<FRACBITS)

// Distance to origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).

#define S_MAX_DIST (NORM_VOLUME * (S_CLIPPING_DIST >> FRACBITS))    // [d64]
#define S_CLOSE_DIST (200 << FRACBITS)
#define S_ATTENUATOR ((S_CLIPPING_DIST - S_CLOSE_DIST) >> FRACBITS)

#define S_PITCH_PERTURB         1
#define S_STEREO_SWING          (96*0x10000)

// percent attenuation from front to back
#define S_IFRACVOL              30

static dboolean nosound = false;
static dboolean nomusic = false;
static int lastmusic = 0;

CVAR_CMD(s_sfxvol, 80)  {
    if(cvar->value < 0.0f) {
        return;
    }
    S_SetSoundVolume(cvar->value);
}
CVAR_CMD(s_musvol, 80)  {
    if(cvar->value < 0.0f) {
        return;
    }
    S_SetMusicVolume(cvar->value);
}
CVAR_CMD(s_gain, 1)     {
    if(cvar->value < 0.0f) {
        return;
    }
    S_SetGainOutput(cvar->value);
}

//
// Internals.
//
int S_AdjustSoundParams(fixed_t x, fixed_t y, int* vol, int* sep);

//
// S_Init
//
// Initializes sound stuff, including volume
// and the sequencer
//

void S_Init(void) {
    if(M_CheckParm("-nosound")) {
        nosound = true;
        CON_DPrintf("Sounds disabled\n");
    }

    if(M_CheckParm("-nomusic")) {
        nomusic = true;
        CON_DPrintf("Music disabled\n");
    }

    if(nosound && nomusic) {
        return;
    }

    I_InitSequencer();

    S_SetMusicVolume(s_musvol.value);
    S_SetSoundVolume(s_sfxvol.value);
    S_SetGainOutput(s_gain.value);
}

//
// S_SetSoundVolume
//

void S_SetSoundVolume(float volume) {
    I_SetSoundVolume(volume);
}

//
// S_SetMusicVolume
//

void S_SetMusicVolume(float volume) {
    I_SetMusicVolume(volume);
}

//
// S_SetGainOutput
//

void S_SetGainOutput(float db) {
    I_SetGain(db);
}

//
// S_StartMusic
//

void S_StartMusic(int mnum) {
    if(nomusic) {
        return;
    }

    if(mnum <= -1) {
        return;
    }

    I_StartMusic(mnum);
    lastmusic = mnum;
}

//
// S_StopMusic
//

void S_StopMusic(void) {
    I_StopSound(NULL, lastmusic);
    lastmusic = 0;
}

//
// S_ResetSound
//

void S_ResetSound(void) {
    int i;

    if(nosound && nomusic) {
        return;
    }

    I_ResetSound();

    // villsa 12282013 - make sure we clear all sound sources
    // during level transition
    for(i = 0; i < I_GetMaxChannels(); i++) {
        I_RemoveSoundSource(i);
    }
}

//
// S_PauseSound
//

void S_PauseSound(void) {
    if(nosound && nomusic) {
        return;
    }

    I_PauseSound();
}

//
// S_ResumeSound
//

void S_ResumeSound(void) {
    if(nosound && nomusic) {
        return;
    }

    I_ResumeSound();
}

//
// S_StopSound
//

void S_StopSound(mobj_t* origin, int sfx_id) {
    I_StopSound((sndsrc_t*)origin, sfx_id);
}

//
// S_GetActiveSounds
//

int S_GetActiveSounds(void) {
    return I_GetVoiceCount();
}

//
// S_RemoveOrigin
//

void S_RemoveOrigin(mobj_t* origin) {
    int     channels;
    mobj_t* source;
    int     i;

    channels = I_GetMaxChannels();

    for(i = 0; i < channels; i++) {
        source = (mobj_t*)I_GetSoundSource(i);
        if(origin == source) {
            I_RemoveSoundSource(i);
        }
    }
}

//
// S_UpdateSounds
//

void S_UpdateSounds(void) {
    int     i;
    int     audible;
    int     volume;
    int     sep;
    mobj_t* source;
    int     channels;

    channels = I_GetMaxChannels();

    for(i = 0; i < channels; i++) {
        source = (mobj_t*)I_GetSoundSource(i);

        if(source == NULL) {
            continue;
        }

        // initialize parameters
        volume = NORM_VOLUME;
        sep = NORM_SEP;

        // check non-local sounds for distance clipping
        // or modify their params
        if(source == players[consoleplayer].mo &&
                players[consoleplayer].cameratarget == players[consoleplayer].mo) {
            audible = 1;
            sep = NORM_SEP;
        }
        else {
            audible = S_AdjustSoundParams(source->x, source->y, &volume, &sep);
        }

        if(audible) {
            I_UpdateChannel(i, volume, sep);
        }
    }
}

//
// S_StartSound
//

void S_StartSound(mobj_t* origin, int sfx_id) {
    int volume;
    int sep;
    int reverb;

    if(nosound) {
        return;
    }

    if(origin && origin != players[consoleplayer].cameratarget) {
        if(!S_AdjustSoundParams(origin->x, origin->y, &volume, &sep)) {
            return;
        }
    }
    else {
        sep = NORM_SEP;
        volume = NORM_VOLUME;
    }

    reverb = 0;

    if(origin) {
        subsector_t* subsector;

        subsector = R_PointInSubsector(origin->x, origin->y);

        if(subsector->sector->flags & MS_REVERB) {
            reverb = 16;
        }
        else if(subsector->sector->flags & MS_REVERBHEAVY) {
            reverb = 32;
        }
    }

    // Assigns the handle to one of the channels in the mix/output buffer.
    I_StartSound(sfx_id, (sndsrc_t*)origin, volume, sep, reverb);
}

//
// S_AdjustSoundParams
//
// Changes volume, stereo-separation, and pitch variables
// from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//

int S_AdjustSoundParams(fixed_t x, fixed_t y, int* vol, int* sep) {
    fixed_t     approx_dist;
    angle_t     angle;
    mobj_t*        listener;
    player_t*    player;

    player = &players[consoleplayer];

    listener = player->cameratarget;

    // calculate the distance to sound origin
    //  and clip it if necessary

    // From _GG1_ p.428. Appox. eucledian distance fast.
    approx_dist = P_AproxDistance(listener->x - x, listener->y - y);

    if(approx_dist > S_CLIPPING_DIST) {
        return 0;
    }

    if(listener->x != x || listener->y != y) {
        // angle of source to listener
        angle = R_PointToAngle2(listener->x, listener->y, x, y);

        if(angle <= listener->angle) {
            angle += 0xffffffff;
        }
        angle -= listener->angle;

        // stereo separation
        *sep = (NORM_VOLUME + 1) - (FixedMul(S_STEREO_SWING, dsin(angle)) >> FRACBITS);
    }
    else {
        *sep = NORM_SEP;
    }

    // volume calculation
    if(approx_dist < S_CLOSE_DIST) {
        *vol = NORM_VOLUME;
    }
    else {
        // distance effect
        approx_dist >>= FRACBITS;
        *vol = (((-approx_dist << 7) + (approx_dist)) + S_MAX_DIST) / S_ATTENUATOR;
    }

    return (*vol > 0);
}


//
// S_RegisterCvars
//

CVAR_EXTERNAL(s_soundfont);
CVAR_EXTERNAL(s_driver);

void S_RegisterCvars(void) {
    CON_CvarRegister(&s_sfxvol);
    CON_CvarRegister(&s_musvol);
    CON_CvarRegister(&s_gain);
    CON_CvarRegister(&s_soundfont);
    CON_CvarRegister(&s_driver);
}




