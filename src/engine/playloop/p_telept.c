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
// DESCRIPTION:
//    Teleportation.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "s_sound.h"
#include "p_local.h"
#include "sounds.h"
#include "tables.h"
#include "doomstat.h"


//
// TELEPORTATION
//

//
// P_Telefrag
//

static void P_Telefrag(mobj_t *thing, fixed_t x, fixed_t y) {
    int     delta;
    int     size;
    mobj_t* m;

    for(m = mobjhead.next; m != &mobjhead; m = m->next) {
        if(!(m->flags & MF_SHOOTABLE)) {
            continue;
        }

        size = m->radius + thing->radius + 4*FRACUNIT;

        delta = m->x - x;
        if(delta < -size || delta > size) {
            continue;
        }

        delta = m->y - y;
        if(delta < -size || delta > size) {
            continue;
        }

        P_DamageMobj(m, thing, thing, 10000);
        m->flags &= ~(MF_SOLID|MF_SHOOTABLE);
    }
}

//
// EV_Teleport
//

int EV_Teleport(line_t* line, int side, mobj_t* thing) {
    int         tag;
    mobj_t*     m;
    mobj_t*     fog;
    angle_t     an;
    fixed_t     oldx;
    fixed_t     oldy;
    fixed_t     oldz;

    // don't teleport missiles
    if(thing->flags & MF_MISSILE) {
        return 0;
    }

    // Don't teleport if hit back of line, so you can get out of teleporter.
    if(side == 1) {
        return 0;
    }

    tag = line->tag;
    for(m = mobjhead.next; m != &mobjhead; m = m->next) {
        // not a teleportman
        if(m->type != MT_DEST_TELEPORT) {
            continue;
        }

        // not matching the tid
        if(m->tid != tag) {
            continue;
        }

        // no use teleporting if the thing has no room
        if(m->ceilingz - m->floorz < m->height) {
            continue;
        }

        oldx = thing->x;
        oldy = thing->y;
        oldz = thing->z + (thing->height >> 1);

        if(thing->player) {
            P_Telefrag(thing, m->x, m->y);
        }

        if(!P_TeleportMove(thing, m->x, m->y)) {
            return 0;
        }

        thing->z = thing->floorz;

        if(thing->player) {
            thing->player->viewz = thing->z + thing->player->viewheight;
        }

        // spawn teleport fog at source and destination
        fog = P_SpawnMobj(oldx, oldy, oldz, MT_TELEPORTFOG);

        S_StartSound(fog, sfx_telept);

        an = m->angle >> ANGLETOFINESHIFT;
        fog = P_SpawnMobj(m->x + 20 * finecosine[an], m->y + 20 * finesine[an],
                          thing->z + (thing->height >> 1), MT_TELEPORTFOG);

        // emit sound, where?
        S_StartSound(fog, sfx_telept);

        // don't move for a bit
        if(thing->player) {
            thing->reactiontime = 9;    // [d64] changed to 9
        }

        thing->angle = m->angle;
        thing->momx = thing->momy = thing->momz = 0;

        return 1;
    }

    return 0;
}

//
// EV_SilentTeleport
//

int EV_SilentTeleport(line_t* line, mobj_t* thing) {
    int         tag;
    mobj_t*     m;
    fixed_t     oldx;
    fixed_t     oldy;
    fixed_t     oldz;

    tag = line->tag;
    for(m = mobjhead.next; m != &mobjhead; m = m->next) {
        // not a teleportman
        if(m->type != MT_DEST_TELEPORT) {
            continue;
        }

        // not matching the tid
        if(m->tid != tag) {
            continue;
        }

        oldx = thing->x;
        oldy = thing->y;
        oldz = thing->z;

        if(thing->player) {
            P_Telefrag(thing, m->x, m->y);
        }

        // don't bother checking for position, just move it
        P_TeleportMove(thing, m->x, m->y);

        if(thing->player) {
            thing->player->viewz = thing->z + thing->player->viewheight;
        }

        thing->angle = m->angle;
        thing->z = m->z;
        thing->momx = thing->momy = thing->momz = 0;

        return 1;
    }

    return 0;
}

