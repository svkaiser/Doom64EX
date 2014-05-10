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


#ifndef __M_RANDOM__
#define __M_RANDOM__


#include "doomtype.h"
#include "d_keywds.h"

typedef enum {
    pr_skullfly,                // #1
    pr_damage,                  // #2
    pr_crush,                   // #3
    pr_genlift,                 // #4
    pr_killtics,                // #5
    pr_damagemobj,              // #6
    pr_painchance,              // #7
    pr_lights,                  // #8
    pr_explode,                 // #9
    pr_respawn,                 // #10
    pr_lastlook,                // #11
    pr_spawnthing,              // #12
    pr_spawnpuff,               // #13
    pr_spawnblood,              // #14
    pr_missile,                 // #15
    pr_shadow,                  // #16
    pr_plats,                   // #17
    pr_punch,                   // #18
    pr_punchangle,              // #19
    pr_saw,                     // #20
    pr_plasma,                  // #21
    pr_gunshot,                 // #22
    pr_misfire,                 // #23
    pr_shotgun,                 // #24
    pr_bfg,                     // #25
    pr_slimehurt,               // #26
    pr_dmspawn,                 // #27
    pr_missrange,               // #28
    pr_trywalk,                 // #29
    pr_newchase,                // #30
    pr_newchasedir,             // #31
    pr_see,                     // #32
    pr_facetarget,              // #33
    pr_posattack,               // #34
    pr_sposattack,              // #35
    pr_spidrefire,              // #36
    pr_troopattack,             // #37
    pr_sargattack,              // #38
    pr_headattack,              // #39
    pr_bruisattack,             // #40
    pr_tracer,                  // #41
    pr_scream,                  // #42
    pr_spawnfly,                // #43
    pr_misc,                    // #44
    pr_all_in_one,              // #45
    pr_mobjexplode,             // #46
    pr_playattack,              // #47
    pr_chaingun,                // #48
    pr_laser,                   // #49
    pr_randomtrigger,           // #50
    pr_quake,                   // #51
    pr_playermock,              // #52

    // End of new entries
    NUMPRCLASS               // MUST be last item in list
} pr_class_t;

// The random number generator's state.
typedef struct {
    unsigned int seed[NUMPRCLASS];       // Each block's random seed
    int rndindex, prndindex;             // For compatibility support
} rng_t;

extern rng_t rng;                      // The rng's state

extern unsigned int rngseed;           // The starting seed (not part of state)

// Returns a number from 0 to 255,
// from a lookup table.
extern byte rndtable[256];

int M_Random(void);

// As M_Random, but used only by the play simulation.
int P_Random(pr_class_t pr_class);

// Fix randoms for demos.
void M_ClearRandom(void);

int P_RandomShift(pr_class_t pr_class, int shift);


#endif
