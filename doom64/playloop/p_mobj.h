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


#ifndef __P_MOBJ__
#define __P_MOBJ__

// Basics.
#include "tables.h"
#include "info.h"
#include "m_fixed.h"
#include "d_think.h"
#include "doomdata.h"
#include "tables.h"
#include "info.h"


#ifdef __GNUG__
#pragma interface
#endif

//
// NOTES: mobj_t
//
// mobj_ts are used to tell the refresh where to draw an image,
// tell the world simulation when objects are contacted,
// and tell the sound driver how to position a sound.
//
// The refresh uses the next and prev links to follow
// lists of things in sectors as they are being drawn.
// The sprite, frame, and angle elements determine which patch_t
// is used to draw the sprite if it is visible.
// The sprite and frame values are allmost allways set
// from state_t structures.
// The statescr.exe utility generates the states.h and states.c
// files that contain the sprite/frame numbers from the
// statescr.txt source file.
// The xyz origin point represents a point at the bottom middle
// of the sprite (between the feet of a biped).
// This is the default origin position for patch_ts grabbed
// with lumpy.exe.
// A walking creature will have its z equal to the floor
// it is standing on.
//
// The sound code uses the x,y, and subsector fields
// to do stereo positioning of any sound effited by the mobj_t.
//
// The play simulation uses the blocklinks, x,y,z, radius, height
// to determine when mobj_ts are touching each other,
// touching lines in the map, or hit by trace lines (gunshots,
// lines of sight, etc).
// The mobj_t->flags element has various bit flags
// used by the simulation.
//
// Every mobj_t is linked into a single sector
// based on its origin coordinates.
// The subsector_t is found with R_PointInSubsector(x,y),
// and the sector_t can be found with subsector->sector.
// The sector links are only used by the rendering code,
// the play simulation does not care about them at all.
//
// Any mobj_t that needs to be acted upon by something else
// in the play world (block movement, be shot, etc) will also
// need to be linked into the blockmap.
// If the thing has the MF_NOBLOCK flag set, it will not use
// the block links. It can still interact with other things,
// but only as the instigator (missiles will run into other
// things, but nothing can run into a missile).
// Each block in the grid is 128*128 units, and knows about
// every line_t that it contains a piece of, and every
// interactable mobj_t that has its origin contained.
//
// A valid mobj_t is a mobj_t that has the proper subsector_t
// filled in for its xy coordinates and is linked into the
// sector from which the subsector was made, or has the
// MF_NOSECTOR flag set (the subsector_t needs to be valid
// even if MF_NOSECTOR is set), and is linked into a blockmap
// block or has the MF_NOBLOCKMAP flag set.
// Links should only be modified by the P_[Un]SetThingPosition()
// functions.
// Do not change the MF_NO? flags while a thing is valid.
//
// Any questions?
//

//
// Misc. mobj flags
//

typedef enum {
    MF_SPECIAL              = 1,            // Call P_SpecialThing when touched.
    MF_SOLID                = 2,            // Blocks.
    MF_SHOOTABLE            = 4,            // Can be hit.
    MF_NOSECTOR             = 8,            // Don't use the sector links (invisible but touchable).
    MF_NOBLOCKMAP           = 16,           // Don't use the blocklinks (inert but displayable)
    MF_AMBUSH               = 32,           // Not to be activated by sound, deaf monster.
    MF_JUSTHIT              = 64,           // Will try to attack right back.
    MF_JUSTATTACKED         = 128,          // Will take at least one step before attacking.
    MF_SPAWNCEILING         = 256,          // Hang from ceiling instead of stand on floor.
    MF_GRAVITY              = 512,          // [d64] Apply gravity (every tic),
    MF_DROPOFF              = 0x400,        // This allows jumps from high places.
    MF_PICKUP               = 0x800,        // For players, will pick up items.
    MF_NOCLIP               = 0x1000,       // Clip through walls (unused)
    MF_NIGHTMARE            = 0x2000,       // [kex] Enables nightmare mode
    MF_FLOAT                = 0x4000,       // For active floaters, e.g. cacodemons, pain elementals.
    MF_TELEPORT             = 0x8000,       // (unused)
    MF_MISSILE              = 0x10000,      // Player missiles as well as fireballs of various kinds.
    MF_DROPPED              = 0x20000,      // Dropped by a demon, not level spawned.
    MF_TRIGTOUCH            = 0x40000,      // [d64] Trigger line special on touch/pickup
    MF_NOBLOOD              = 0x80000,      // Flag: don't bleed when shot (use puff),
    MF_CORPSE               = 0x100000,     // Don't stop moving halfway off a step,
    MF_INFLOAT              = 0x200000,     // don't auto float to target's height.
    MF_COUNTKILL            = 0x400000,     // On kill, count this enemy object towards intermission kill total.
    MF_COUNTITEM            = 0x800000,     // On picking up, count this item object towards intermission item total.
    MF_SKULLFLY             = 0x1000000,    // Special handling: skull in flight.
    MF_NOTDMATCH            = 0x2000000,    // Don't spawn this object in death match mode (e.g. key cards).
    MF_SEETARGET            = 0x4000000,    // [d64] Is target visible?
    MF_COUNTSECRET          = 0x8000000,    // [d64] Count as secret when picked up (for intermissions)
    MF_RENDERLASER          = 0x10000000,   // [d64] Exclusive to MT_LASERMARKER only
    MF_TRIGDEATH            = 0x20000000,   // [d64] Trigger line special on death
    MF_SHADOW               = 0x40000000,   // temporary player invisibility powerup.
    MF_NOINFIGHTING         = 0x80000000    // [d64] Do not switch targets

} mobjflag_t;

typedef enum {
    BF_MOBJSTAND        = 1,    // Standing on top of thing?
    BF_MOBJPASS         = 2,    // Able to pass under thing?
    BF_MIDPOINTONLY     = 4     // Only check sector where mobj's midpoint is inside

} mobjblockflag_t;

// Map Object definition

struct mobj_s;
typedef void (*mobjfunc_t)(struct mobj_s *mo);

typedef struct mobj_s {
    // Info for drawing: position.
    fixed_t             x;
    fixed_t             y;
    fixed_t             z;

    // [d64] mobj tag
    int                 tid;

    // More list: links in sector (if needed)
    struct mobj_s*      snext;
    struct mobj_s*      sprev;

    //More drawing info: to determine current sprite.
    angle_t             angle;    // orientation
    angle_t             pitch;  // [kex] pitch orientation; for looking up/down
    spritenum_t         sprite;    // used to find patch_t and flip value
    int                 frame;    // might be ORed with FF_FULLBRIGHT

    // Interaction info, by BLOCKMAP.
    // Links in blocks (if needed).
    struct mobj_s*      bnext;
    struct mobj_s*      bprev;

    struct subsector_s* subsector;

    // The closest interval over all contacted Sectors.
    fixed_t             floorz;
    fixed_t             ceilingz;

    // For movement checking.
    fixed_t             radius;
    fixed_t             height;

    // Momentums, used to update position.
    fixed_t             momx;
    fixed_t             momy;
    fixed_t             momz;

    // If == validcount, already checked.
    int                 validcount;

    mobjtype_t          type;
    mobjinfo_t*         info;    // &mobjinfo[mobj->type]

    int                 tics;    // state tic counter
    state_t*            state;
    dword               flags;
    int                 health;

    // [d64] alpha value for rendering
    int                 alpha;

    // [kex] flags for mobj collision
    mobjblockflag_t     blockflag;

    // Movement direction, movement generation (zig-zagging).
    int                 movedir;    // 0-7
    int                 movecount;    // when 0, select a new dir

    // Thing being chased/attacked (or NULL),
    // also the originator for missiles.
    struct mobj_s*      target;

    // Reaction time: if non 0, don't attack yet.
    // Used by player to freeze a bit after teleporting.
    int                 reactiontime;

    // If >0, the target will be chased
    // no matter what (even if shot)
    int                 threshold;

    // Additional info record for player avatars only.
    // Only valid if type == MT_PLAYER
    struct player_s*    player;

    // For nightmare respawn.
    mapthing_t          spawnpoint;

    // Thing being chased/attacked for tracers.
    struct mobj_s*      tracer;

    // [d64] Mobj linked list: used to seperate from thinkers
    struct mobj_s*      prev;
    struct mobj_s*      next;

    // [d64] callback routine called at end of P_Tick
    mobjfunc_t          mobjfunc;

    // [d64] misc data for various actions
    void*               extradata;

    // [kex] stuff that happens in between tics
    fixed_t             frame_x;
    fixed_t             frame_y;
    fixed_t             frame_z;

    // [kex] mobj reference id
    unsigned int        refcount;

} mobj_t;

#endif
