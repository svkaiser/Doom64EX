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

#ifndef __D_PLAYER__
#define __D_PLAYER__

// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "p_pspr.h"
// In addition, the player is just a special
// case of the generic moving object/actor.
#include "p_mobj.h"
// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "d_ticcmd.h"

#ifdef __GNUG__
#pragma interface
#endif

//
// Player states.
//
typedef enum {
    PST_LIVE,   // Playing or camping.
    PST_DEAD,   // Dead on the ground, view follows killer.
    PST_REBORN  // Ready to restart/respawn???

} playerstate_t;


//
// Player internal flags, for cheats and debug.
//
typedef enum {
    CF_NOCLIP       = 1,    // No clipping, walk through barriers.
    CF_GODMODE      = 2,    // No damage, no health loss.
    CF_UNDYING      = 4,    // Take damage but not die
    CF_SPECTATOR    = 8,    // Specator mode
    CF_CHASECAM     = 16,   // Chase cam mode
    CF_FLOATCAM     = 32,   // Floating camera
    CF_LOCKCAM      = 64    // view camera is locked by an line action

} cheat_t;

typedef enum {
    ART_FAST    = 0,
    ART_DOUBLE,
    ART_TRIPLE

} artifacts_t;


//
// Extended player object info: player_t
//
typedef struct player_s {
    mobj_t*         mo;
    playerstate_t   playerstate;
    ticcmd_t        cmd;

    // Determine POV,
    //  including viewpoint bobbing during movement.
    // Focal origin above r.z
    fixed_t         viewz;
    // Base height above floor for viewz.
    fixed_t         viewheight;
    // Bob/squat speed.
    fixed_t         deltaviewheight;
    // bounded/scaled total momentum.
    fixed_t         bob;

    // [d64] extra pitch for recoil/knockback
    angle_t         recoilpitch;

    // This is only used between levels,
    // mo->health is used during levels.
    int             health;
    int             armorpoints;
    // Armor type is 0-2.
    int             armortype;

    // Power ups. invinc and invis are tic counters.
    int             powers[NUMPOWERS];
    dboolean        cards[NUMCARDS];

    // [kex] for hud when trying to open a locked door
    dboolean        tryopen[NUMCARDS];

    // [d64] laser artifact flags
    int             artifacts;

    dboolean        backpack;

    // Frags, kills of other players.
    int             frags[MAXPLAYERS];
    weapontype_t    readyweapon;

    // Is wp_nochange if not changing.
    weapontype_t    pendingweapon;

    dboolean        weaponowned[NUMWEAPONS];
    int             ammo[NUMAMMO];
    int             maxammo[NUMAMMO];

    // True if button down last tic.
    int             attackdown;
    int             usedown;

    // [kex] true if jump button down last tic
    int             jumpdown;

    // Bit flags, for cheats and debug.
    // See cheat_t, above.
    int             cheats;

    // Refired shots are less accurate.
    int             refire;

    // For intermission stats.
    int             killcount;
    int             itemcount;
    int             secretcount;

    // Hint messages.
    char*           message;

    // [d64] tic for how long message should stay on hud...not used in d64ex
    // int          messagetic;

    // For screen flashing (red or bright).
    int             damagecount;
    int             bonuscount;
    int             bfgcount;

    // Who did damage (NULL for floors/ceilings).
    mobj_t*         attacker;

    // [kex] which mobj to use as the view camera (defaults to player->mo)
    mobj_t*         cameratarget;

    // Overlay view sprites (gun, etc).
    pspdef_t        psprites[NUMPSPRITES];

    // Translation palettes for multiplayer
    int             palette;

    // [d64] Track if player is on the ground or not
    dboolean        onground;

    // [kex] allow autoaim?
    dboolean        autoaim;

    // [kex] display pic as message instead of text
    int             messagepic;

} player_t;

#endif
