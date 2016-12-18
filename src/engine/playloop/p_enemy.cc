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
//
// DESCRIPTION:
//    Enemy thinking, AI.
//    Action Pointer Functions
//    that are associated with states/frames.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>

#include "m_random.h"
#include "m_fixed.h"
#include "i_system.h"
#include "doomdef.h"
#include "p_local.h"
#include "p_macros.h"
#include "s_sound.h"
#include "g_game.h"
#include "doomstat.h"
#include "r_local.h"
#include "sounds.h"
#include "tables.h"
#include "info.h"
#include "z_zone.h"


typedef int dirtype_t;
enum {
    DI_EAST,
    DI_NORTHEAST,
    DI_NORTH,
    DI_NORTHWEST,
    DI_WEST,
    DI_SOUTHWEST,
    DI_SOUTH,
    DI_SOUTHEAST,
    DI_NODIR,
    NUMDIRS

};


//
// P_NewChaseDir related LUT.
//
dirtype_t opposite[] = {
    DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
    DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST,
    DI_NODIR
};

dirtype_t diags[] = {
    DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};

extern "C" {

void A_Fall(mobj_t *actor);


//
// ENEMY THINKING
// Enemies are allways spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//


//
// P_RecursiveSound
//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//

mobj_t* soundtarget;

void P_RecursiveSound(sector_t* sec, int soundblocks) {
    int        i;
    line_t*    check;
    sector_t*    other;

    // wake up all monsters in this sector
    if(sec->validcount == validcount && sec->soundtraversed <= soundblocks+1) {
        return;    // already flooded
    }

    sec->validcount     = validcount;
    sec->soundtraversed = soundblocks+1;

    P_SetTarget(&sec->soundtarget, soundtarget);

    for(i = 0; i < sec->linecount; i++) {
        check = sec->lines[i];
        if(!(check->flags & ML_TWOSIDED)) {
            continue;
        }

        P_LineOpening(check);

        if(openrange <= 0) {
            continue;    // closed door
        }

        if(sides[check->sidenum[0] ].sector == sec) {
            other = sides[check->sidenum[1]].sector;
        }
        else {
            other = sides[check->sidenum[0]].sector;
        }

        if(check->flags & ML_SOUNDBLOCK) {
            if(!soundblocks) {
                P_RecursiveSound(other, 1);
            }
        }
        else {
            P_RecursiveSound(other, soundblocks);
        }
    }
}



//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//

void P_NoiseAlert(mobj_t* target, mobj_t* emmiter) {
    soundtarget = target;
    D_IncValidCount();
    P_RecursiveSound(emmiter->subsector->sector, 0);
}




//
// P_CheckMeleeRange
//

dboolean P_CheckMeleeRange(mobj_t* actor) {
    mobj_t*    pl;
    fixed_t    dist;

    if(!(actor->flags & MF_SEETARGET)) {
        return false;
    }

    if(!actor->target) {
        return false;
    }

    pl = actor->target;
    dist = P_AproxDistance(pl->x - actor->x, pl->y - actor->y);

    if(dist >= MELEERANGE) {
        return false;
    }

    return true;
}

//
// P_CheckMissileRange
//

dboolean P_CheckMissileRange(mobj_t* actor) {
    fixed_t    dist;
    int        idist;

    if(!(actor->flags & MF_SEETARGET)) {
        return false;
    }

    if(actor->flags & MF_JUSTHIT) {
        // the target just hit the enemy,
        // so fight back!
        actor->flags &= ~MF_JUSTHIT;
        return true;
    }

    if(actor->reactiontime) {
        return false;    // do not attack yet
    }

    // OPTIMIZE: get this from a global checksight
    dist = P_AproxDistance(actor->x-actor->target->x,
                           actor->y-actor->target->y) - 64 * FRACUNIT;

    if(!actor->info->meleestate) {
        dist -= 128*FRACUNIT;    // no melee attack, so fire more
    }

    idist = F2INT(dist);

    if(actor->type == MT_SKULL) {
        idist >>= 1;
    }

    if(idist > 200) {
        idist = 200;
    }

    if(P_Random(pr_missrange) < idist) {
        return false;
    }

    return true;
}

//
// P_MissileAttack
//

enum {
    DP_STRAIGHT,
    DP_LEFT,
    DP_RIGHT
} dirproj_e;

static mobj_t *P_MissileAttack(mobj_t *actor, int direction) {
    angle_t angle = 0;
    fixed_t deltax = 0;
    fixed_t deltay = 0;
    fixed_t deltaz = 0;
    fixed_t offs = 0;
    int type = 0;
    dboolean aim = false;
    mobj_t *mo;

    if(direction == DP_LEFT) {
        angle = actor->angle + ANG45;
    }
    else if(direction == DP_RIGHT) {
        angle = actor->angle - ANG45;
    }
    else {
        angle = actor->angle;
    }

    angle >>= ANGLETOFINESHIFT;

    switch(actor->type) {
    case MT_MANCUBUS:
        offs = 50;
        deltaz = 69;
        type = MT_PROJ_FATSO;
        aim = true;
        break;
    case MT_IMP1:
        offs = 0;
        deltaz = 64;
        type = MT_PROJ_IMP1;
        aim = true;
        break;
    case MT_IMP2:
        offs = 0;
        deltaz = 64;
        type = MT_PROJ_IMP2;
        aim = true;
        break;
    case MT_BABY:
        offs = 20;
        deltaz = 28;
        type = MT_PROJ_BABY;
        aim = false;
        break;
    case MT_CACODEMON:
        offs = 0;
        deltaz = 46;
        type = MT_PROJ_HEAD;
        aim = true;
        break;
    case MT_CYBORG:
    case MT_CYBORG_TITLE:
        offs = 45;
        deltaz = 88;
        type = MT_PROJ_ROCKET;
        aim = true;
        break;
    case MT_BRUISER1:
        offs = 0;
        deltaz = 48;
        type = MT_PROJ_BRUISER2;
        aim = true;
        break;
    case MT_BRUISER2:
        offs = 0;
        deltaz = 48;
        type = MT_PROJ_BRUISER1;
        aim = true;
        break;
    }

    deltax = FixedMul(offs*FRACUNIT, finecosine[angle]);
    deltay = FixedMul(offs*FRACUNIT, finesine[angle]);

    mo = P_SpawnMissile(actor, actor->target, type, deltax, deltay, deltaz, aim);

    return mo;
}

//
// T_MobjExplode
//

void T_MobjExplode(void *data) {
    mobjexp_t *mexp = (mobjexp_t *) data;
    fixed_t x;
    fixed_t y;
    fixed_t z;
    mobj_t* exp;
    mobj_t* mobj = mexp->mobj;

    if(mexp->delay-- > 0) {
        return;
    }

    mexp->delay = mexp->delaymax;

    if(mobj->state != (state_t *)S_000) {
        x = (P_RandomShift(pr_mobjexplode, 14) + mobj->x);
        y = (P_RandomShift(pr_mobjexplode, 14) + mobj->y);
        z = (P_RandomShift(pr_mobjexplode, 14) + mobj->z);

        exp = P_SpawnMobj(x, y, z + (mobj->height << 1), MT_EXPLOSION2);

        if(!(mexp->lifetime & 1)) {
            S_StartSound(exp, sfx_explode);
        }
    }

    if(!mexp->lifetime--) {
        P_SetTarget(&mexp->mobj, NULL);
        P_RemoveThinker(&mexp->thinker);
    }
}


//
// P_Move
// Move in the current direction,
// returns false if the move is blocked.
//

fixed_t xspeed[8] = {FRACUNIT,47000,0,-47000,-FRACUNIT,-47000,0,47000};
fixed_t yspeed[8] = {0,47000,FRACUNIT,47000,0,-47000,-FRACUNIT,-47000};

dboolean P_Move(mobj_t* actor) {
    fixed_t    tryx;
    fixed_t    tryy;
    line_t*    ld;
    dboolean   try_ok;
    dboolean   good;

    if(actor->movedir == DI_NODIR) {
        return false;
    }

    if(actor->flags & MF_GRAVITY) {
        if(actor->floorz != actor->z) {
            return false;
        }
    }

    tryx = actor->x + actor->info->speed * xspeed[actor->movedir];
    tryy = actor->y + actor->info->speed * yspeed[actor->movedir];

    try_ok = P_TryMove(actor, tryx, tryy);

    if(!try_ok) {
        // open any specials
        if(actor->flags & MF_FLOAT && floatok) {
            // must adjust height
            if(actor->z < tmfloorz) {
                actor->z += FLOATSPEED;
            }
            else {
                actor->z -= FLOATSPEED;
            }

            //actor->flags |= MF_INFLOAT;
            return true;
        }

        if(!numspechit) {
            return false;
        }

        actor->movedir = DI_NODIR;
        good = false;
        while(numspechit--) {
            ld = spechit[numspechit];
            // if the special is not a door that can be opened,
            // return false
            if(ld->special & MLU_USE) {
                if(P_UseSpecialLine(actor, ld, 0)) {
                    good = true;
                }
            }
        }
        return good;
    }
    //else
    //actor->flags &= ~MF_INFLOAT;

    return true;
}


//
// P_TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//

dboolean P_TryWalk(mobj_t* actor) {
    if(!P_Move(actor)) {
        return false;
    }

    actor->movecount = P_Random(pr_trywalk) & 7;
    return true;
}

//
// P_NewChaseDir
//

void P_NewChaseDir(mobj_t* actor) {
    fixed_t        deltax;
    fixed_t        deltay;
    dirtype_t    d[3];
    int            tdir;
    dirtype_t    olddir;
    dirtype_t    turnaround;

    if(!actor->target) {
        I_Error("P_NewChaseDir: called with no target");
    }

    olddir = actor->movedir;
    turnaround=opposite[olddir];

    deltax = actor->target->x - actor->x;
    deltay = actor->target->y - actor->y;

    if(deltax > 10*FRACUNIT) {
        d[1] = DI_EAST;
    }
    else if(deltax < -10*FRACUNIT) {
        d[1] = DI_WEST;
    }
    else {
        d[1] = DI_NODIR;
    }

    if(deltay < -10*FRACUNIT) {
        d[2] = DI_SOUTH;
    }
    else if(deltay > 10*FRACUNIT) {
        d[2] = DI_NORTH;
    }
    else {
        d[2] = DI_NODIR;
    }

    // try direct route
    if(d[1] != DI_NODIR && d[2] != DI_NODIR) {
        actor->movedir = diags[((deltay < 0) << 1) + (deltax > 0)];
        if(actor->movedir != turnaround && P_TryWalk(actor)) {
            return;
        }
    }

    // try other directions
    if(P_Random(pr_newchase) > 200 ||  D_abs(deltay) > D_abs(deltax)) {
        tdir = d[1];
        d[1] = d[2];
        d[2] = tdir;
    }

    if(d[1] == turnaround) {
        d[1] = DI_NODIR;
    }
    if(d[2] == turnaround) {
        d[2] = DI_NODIR;
    }

    if(d[1] != DI_NODIR) {
        actor->movedir = d[1];
        if(P_TryWalk(actor)) {
            return;    // either moved forward or attacked
        }
    }

    if(d[2]!=DI_NODIR) {
        actor->movedir = d[2];
        if(P_TryWalk(actor)) {
            return;
        }
    }

    // there is no direct path to the player,
    // so pick another direction.
    if(olddir!=DI_NODIR) {
        actor->movedir = olddir;
        if(P_TryWalk(actor)) {
            return;
        }
    }

    // randomly determine direction of search
    if(P_Random(pr_newchasedir) & 1) {
        for(tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++) {
            if(tdir != turnaround) {
                actor->movedir = tdir;
                if(P_TryWalk(actor)) {
                    return;
                }
            }
        }
    }
    else {
        for(tdir = DI_SOUTHEAST; tdir != (DI_EAST-1); tdir--) {
            if(tdir != turnaround) {
                actor->movedir = tdir;
                if(P_TryWalk(actor)) {
                    return;
                }
            }
        }
    }

    if(turnaround !=  DI_NODIR) {
        actor->movedir = turnaround;
        if(P_TryWalk(actor)) {
            return;
        }
    }

    actor->movedir = DI_NODIR;    // can not move
}



//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//

dboolean P_LookForPlayers(mobj_t* actor, dboolean allaround) {
    angle_t     an;
    fixed_t     dist;

    if(!actor->target || actor->target->health <= 0 ||
            !(actor->flags & MF_SEETARGET)) {
        if(actor->type > MT_PLAYERBOT3) { // for monsters
            int index = 0;

            // find other players
            if(netgame) {
                int i;
                int num = 0;
                int cur = 0;

                for(i = 0; i < MAXPLAYERS; i++) {
                    if(playeringame[i]) {
                        ++num;
                        if(actor->target == players[i].mo) {
                            cur = i;
                        }
                    }
                }

                index = ((cur + 1) % num);

                // don't bother with dead targets
                if(players[index].mo->health <= 0) {
                    return false;
                }
            }

            P_SetTarget(&actor->target, players[index].mo);
        }

        else {  // special case for player bots
            fixed_t dist2 = D_MAXINT;
            mobj_t* mobj;

            for(mobj = mobjhead.next; mobj != &mobjhead; mobj = mobj->next) {
                if(!(mobj->flags & MF_COUNTKILL) || mobj->type <= MT_PLAYERBOT3 ||
                        mobj->health <= 0 || mobj == actor) {
                    continue;
                }

                // find a killable target as close as possible
                dist = P_AproxDistance(mobj->x - actor->x, mobj->y - actor->y);
                if(!(dist < dist2)) {
                    continue;
                }

                P_SetTarget(&actor->target, mobj);
                dist2 = dist;
            }
        }

        return false;
    }

    if(!actor->subsector->sector->soundtarget) {
        if(!allaround) {
            an = R_PointToAngle2(actor->x, actor->y,
                                 actor->target->x, actor->target->y) - actor->angle;

            if(an > ANG90 && an < ANG270) {
                dist = P_AproxDistance(actor->target->x - actor->x,
                                       actor->target->y - actor->y);

                // if real close, react anyway
                if(dist > MELEERANGE) {
                    return false;    // behind back
                }
            }
        }
    }

    return true;
}

//
// ACTION ROUTINES
//

//
// A_Look
// Stay in state until a player is sighted.
//

void A_Look(mobj_t* actor) {
    mobj_t*    targ;

    if(!P_LookForPlayers(actor, false)) {
        actor->threshold = 0;    // any shot will wake up
        targ = actor->subsector->sector->soundtarget;

        if(!targ) {
            return;
        }

        if(!(targ->flags & MF_SHOOTABLE)) {
            return;
        }

        if(actor->flags & MF_AMBUSH) {
            return;
        }

        P_SetTarget(&actor->target, targ);
    }

    // go into chase state
    if(actor->info->seesound) {
        int sound;

        switch(actor->info->seesound) {
        case sfx_possit1:
        case sfx_possit2:
        case sfx_possit3:
            sound = sfx_possit1 + (P_Random(pr_see) & 1);
            break;

        case sfx_impsit1:
        case sfx_impsit2:
            sound = sfx_impsit1+ (P_Random(pr_see) & 1);
            break;

        default:
            sound = actor->info->seesound;
            break;
        }

        if(actor->type == MT_RESURRECTOR || actor->type == MT_CYBORG) {
            // full volume
            S_StartSound(NULL, sound);
        }
        else {
            S_StartSound(actor, sound);
        }
    }

    P_SetMobjState(actor, actor->info->seestate);
}


//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase(mobj_t* actor) {
    int delta;

    if(actor->reactiontime) {
        actor->reactiontime--;
    }


    // modify target threshold
    if(actor->threshold) {
        if(!actor->target || actor->target->health <= 0) {
            actor->threshold = 0;
        }
        else {
            actor->threshold--;
        }
    }

    // turn towards movement direction if not there yet
    if(actor->movedir < 8) {
        actor->angle &= (7<<29);
        delta = actor->angle - (actor->movedir << 29);

        if(delta > 0) {
            actor->angle -= ANG90/2;
        }
        else if(delta < 0) {
            actor->angle += ANG90/2;
        }
    }

    if(!actor->target || !(actor->target->flags & MF_SHOOTABLE)) {
        // look for a new target
        if(P_LookForPlayers(actor,true)) {
            return;    // got a new target
        }

        P_SetMobjState(actor, actor->info->spawnstate);
        return;
    }

    // do not attack twice in a row
    if(actor->flags & MF_JUSTATTACKED) {
        actor->flags &= ~MF_JUSTATTACKED;
        if(gameskill != sk_nightmare && !fastparm) {
            P_NewChaseDir(actor);
        }
        return;
    }

    // check for melee attack
    if(actor->info->meleestate && P_CheckMeleeRange(actor)) {
        if(actor->info->attacksound) {
            S_StartSound(actor, actor->info->attacksound);
        }

        P_SetMobjState(actor, actor->info->meleestate);
        return;
    }

    // check for missile attack
    if(actor->info->missilestate) {
        if(gameskill < sk_nightmare && !fastparm && actor->movecount) {
            goto nomissile;
        }

        if(!P_CheckMissileRange(actor)) {
            goto nomissile;
        }

        P_SetMobjState(actor, actor->info->missilestate);
        actor->flags |= MF_JUSTATTACKED;
        return;
    }

nomissile:
    // possibly choose another target
    if(netgame && !actor->threshold && !(actor->flags & MF_SEETARGET)) {
        if(P_LookForPlayers(actor,true)) {
            return;    // got a new target
        }
    }

    // chase towards player
    if(--actor->movecount<0 || !P_Move(actor)) {
        P_NewChaseDir(actor);
    }

    // make active sound
    if(actor->info->activesound && P_Random(pr_see) < 3) {
        S_StartSound(actor, actor->info->activesound);
    }
}


//
// A_FaceTarget
//
void A_FaceTarget(mobj_t* actor) {
    if(!actor->target) {
        return;
    }

    actor->flags &= ~MF_AMBUSH;

    actor->angle = R_PointToAngle2(actor->x, actor->y, actor->target->x, actor->target->y);

    if(actor->target->flags & MF_SHADOW) {
        actor->angle += P_RandomShift(pr_facetarget, 21);
    }
}

//
// A_Tracer
//

#define    TRACEANGLE 0x10000000;

void A_Tracer(mobj_t* actor) {
    angle_t    exact;
    fixed_t    dist;
    fixed_t    slope;
    mobj_t*    dest;
    mobj_t*    th;

    th = P_SpawnMobj(actor->x - actor->momx,
                     actor->y - actor->momy, actor->z, MT_SMOKE_RED);

    th->momz = FRACUNIT;
    th->tics -= P_Random(pr_tracer) & 3;
    if(th->tics < 1) {
        th->tics = 1;
    }

    if(actor->threshold-- < -100) {
        return;
    }

    // adjust direction
    dest = actor->tracer;

    if(!dest || dest->health <= 0) {
        return;
    }

    // change angle
    exact = R_PointToAngle2(actor->x, actor->y, dest->x, dest->y);

    if(exact != actor->angle) {
        if(exact - actor->angle > 0x80000000) {
            actor->angle -= TRACEANGLE;
            if(exact - actor->angle < 0x80000000) {
                actor->angle = exact;
            }
        }
        else {
            actor->angle += TRACEANGLE;
            if(exact - actor->angle > 0x80000000) {
                actor->angle = exact;
            }
        }
    }

    exact = actor->angle >> ANGLETOFINESHIFT;
    actor->momx = FixedMul(actor->info->speed, finecosine[exact]);
    actor->momy = FixedMul(actor->info->speed, finesine[exact]);

    // change slope
    dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
    dist = dist / actor->info->speed;

    if(dist < 1) {
        dist = 1;
    }

    slope = ((dest->height << 2) - dest->height);
    if(slope < 0) {
        slope = (((dest->height << 2) - dest->height) + 3);
    }

    slope = (dest->z + (slope >> 2) - actor->z) / dist;

    if(slope < actor->momz) {
        actor->momz -= (FRACUNIT >> 2);
    }
    else {
        actor->momz += (FRACUNIT >> 2);
    }
}

//
// A_OnDeathTrigger
//

void A_OnDeathTrigger(mobj_t* actor) {
    mobj_t* mo;

    if(!(actor->flags & MF_TRIGDEATH)) {
        return;
    }

    A_Fall(actor);

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        if(mo->player) {
            continue;
        }

        if(mo != actor && mo->tid == actor->tid &&
                mo->flags & MF_SHOOTABLE && mo->health > 0) {
            return;
        }
    }

    P_QueueSpecial(actor);
}


//
// A_PosAttack
//

void A_PosAttack(mobj_t* actor) {
    int     angle;
    int     damage;
    int     hitdice;
    int     slope;

    if(!actor->target) {
        return;
    }

    S_StartSound(actor, sfx_pistol);
    A_FaceTarget(actor);

    angle = actor->angle;
    slope = P_AimLineAttack(actor, angle, 0, MISSILERANGE);

    angle += P_RandomShift(pr_posattack, 20);
    hitdice = (P_Random(pr_posattack) & 7);
    damage = ((hitdice<<2) - hitdice) + 3;
    P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
}

//
// A_SPosAttack
//

void A_SPosAttack(mobj_t* actor) {
    int     i;
    int     angle;
    int     bangle;
    int     damage;
    int     slope;

    if(!actor->target) {
        return;
    }

    S_StartSound(actor, sfx_shotgun);
    A_FaceTarget(actor);
    bangle = actor->angle;
    slope = P_AimLineAttack(actor, bangle, 0, MISSILERANGE);

    for(i = 0; i < 3; i++) {
        angle = bangle + P_RandomShift(pr_sposattack, 20);
        damage = ((P_Random(pr_sposattack) % 5) * 3) + 3;
        P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
    }
}

//
// A_PlayAttack
//

void A_PlayAttack(mobj_t* actor) {
    int    angle;
    int    damage;
    int    hitdice;
    int    slope;

    if(!actor->target) {
        return;
    }

    S_StartSound(actor, sfx_pistol);
    A_FaceTarget(actor);

    angle = actor->angle;
    slope = P_AimLineAttack(actor, angle, 0, MISSILERANGE);

    angle += P_RandomShift(pr_playattack, 20);
    hitdice = (P_Random(pr_playattack)%5);
    damage = ((hitdice << 2) - hitdice) + 3;
    P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
}

//
// A_BspiFaceTarget
//

void A_BspiFaceTarget(mobj_t* actor) {
    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    actor->reactiontime = 5;
}

//
// A_BspiAttack
//

void A_BspiAttack(mobj_t *actor) {
    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    P_MissileAttack(actor, DP_LEFT);
    P_MissileAttack(actor, DP_RIGHT);
}

//
// A_SpidRefire
//

void A_SpidRefire(mobj_t* actor) {
    A_FaceTarget(actor);

    if(P_Random(pr_spidrefire) < 10) {
        return;
    }

    if((!actor->target || actor->target->health <= 0) || !(actor->flags & MF_SEETARGET)) {
        P_SetMobjState(actor, actor->info->seestate);
        actor->reactiontime = 5;
        return;
    }

    if(!actor->reactiontime--) {
        P_SetMobjState(actor, actor->info->missilestate);
        actor->reactiontime = 5;
    }
}

//
// A_TroopMelee
//

void A_TroopMelee(mobj_t* actor) {
    int    damage;
    int hitdice;

    if(!actor->target) {
        return;
    }

    if(P_CheckMeleeRange(actor)) {
        S_StartSound(actor, sfx_scratch);
        hitdice = (P_Random(pr_troopattack) & 7);
        damage = ((hitdice << 2) - hitdice) + 3;
        P_DamageMobj(actor->target, actor, actor, damage);
    }
}

//
// A_TroopAttack
//

void A_TroopAttack(mobj_t* actor) {
    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);

    // launch a missile
    P_MissileAttack(actor, DP_STRAIGHT);
}

//
// A_SargAttack
//

void A_SargAttack(mobj_t* actor) {
    int    damage;
    int    hitdice;

    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    if(P_CheckMeleeRange(actor)) {
        hitdice = (P_Random(pr_sargattack) & 7);
        damage = ((hitdice << 2) + 4);
        P_DamageMobj(actor->target, actor, actor, damage);
    }
}

//
// A_HeadAttack
//

void A_HeadAttack(mobj_t* actor) {
    int    damage;
    int hitdice;

    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    if(P_CheckMeleeRange(actor)) {
        hitdice = (P_Random(pr_headattack) & 7);
        damage = (hitdice << 3) + 8;
        P_DamageMobj(actor->target, actor, actor, damage);
        return;
    }

    // launch a missile
    P_MissileAttack(actor, DP_STRAIGHT);
}

//
// A_CyberAttack
//

void A_CyberAttack(mobj_t* actor) {
    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    P_MissileAttack(actor, DP_LEFT);
}

//
// A_CyberDeathEvent
//

void A_CyberDeathEvent(mobj_t* actor) {
    mobjexp_t *exp;

    exp = (mobjexp_t*) Z_Calloc(sizeof(*exp), PU_LEVSPEC, 0);
    P_AddThinker(&exp->thinker);

    exp->thinker.function.acp1 = (actionf_p1)T_MobjExplode;
    exp->delaymax = 4;
    exp->delay = 0;
    exp->lifetime = 12;
    P_SetTarget(&exp->mobj, actor);

    if(actor->info->deathsound) {
        S_StartSound(NULL, actor->info->deathsound);
    }
}

//
// A_BruisAttack
//

void A_BruisAttack(mobj_t* actor) {
    int    damage;
    int hitdice;

    if(!actor->target) {
        return;
    }

    if(P_CheckMeleeRange(actor)) {
        S_StartSound(actor, sfx_scratch);
        hitdice = (P_Random(pr_bruisattack) & 7);
        damage = ((hitdice << 2) - hitdice) + 11;
        P_DamageMobj(actor->target, actor, actor, damage);
        return;
    }

    // launch a missile
    P_MissileAttack(actor, DP_STRAIGHT);
}


//
// A_RectChase
//

void A_RectChase(mobj_t* actor) {
    if(!actor->target) {
        A_Chase(actor);
        return;
    }
    if(actor->target->health <= 0) {
        A_Chase(actor);
        return;
    }
    if(!(P_AproxDistance(actor->target->x-actor->x,
                         actor->target->y-actor->y) < (600*FRACUNIT))) {
        A_Chase(actor);
        return;
    }

    A_FaceTarget(actor);
    S_StartSound(actor, actor->info->attacksound);
    P_SetMobjState(actor, actor->info->meleestate);

}

//
// A_RectMissile
//

void A_RectMissile(mobj_t* actor) {
    mobj_t* mo;
    int count = 0;
    angle_t an = 0;
    fixed_t x = 0;
    fixed_t y = 0;

    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        // not a rect projectile
        if(mo->type != MT_PROJ_RECT) {
            continue;
        }

        count++;
    }

    if(!(count < 9)) {
        return;
    }

    // Arm 1

    an = (actor->angle-ANG90) >> ANGLETOFINESHIFT;
    x = FixedMul(68*FRACUNIT, finecosine[an]);
    y = FixedMul(68*FRACUNIT, finesine[an]);
    mo = P_SpawnMobj(actor->x + x, actor->y + y, actor->z + 68*FRACUNIT, MT_PROJ_RECT);
    P_SetTarget(&mo->target, actor);
    P_SetTarget(&mo->tracer, actor->target);
    mo->threshold = 5;
    an = (actor->angle + ANG270);
    mo->angle = an;
    an >>= ANGLETOFINESHIFT;
    mo->momx = FixedMul(mo->info->speed, finecosine[an]);
    mo->momy = FixedMul(mo->info->speed, finesine[an]);

    // Arm2

    an = (actor->angle-ANG90) >> ANGLETOFINESHIFT;
    x = FixedMul(50*FRACUNIT, finecosine[an]);
    y = FixedMul(50*FRACUNIT, finesine[an]);
    mo = P_SpawnMobj(actor->x + x, actor->y + y, actor->z + 139*FRACUNIT, MT_PROJ_RECT);
    P_SetTarget(&mo->target, actor);
    P_SetTarget(&mo->tracer, actor->target);
    mo->threshold = 1;
    an = (actor->angle + ANG270);
    mo->angle = an;
    an >>= ANGLETOFINESHIFT;
    mo->momx = FixedMul(mo->info->speed, finecosine[an]);
    mo->momy = FixedMul(mo->info->speed, finesine[an]);

    // Arm3

    an = (actor->angle+ANG90) >> ANGLETOFINESHIFT;
    x = FixedMul(68*FRACUNIT, finecosine[an]);
    y = FixedMul(68*FRACUNIT, finesine[an]);
    mo = P_SpawnMobj(actor->x + x, actor->y + y, actor->z + 68*FRACUNIT, MT_PROJ_RECT);
    P_SetTarget(&mo->target, actor);
    P_SetTarget(&mo->tracer, actor->target);
    mo->threshold = 5;
    an = (actor->angle - ANG270);
    mo->angle = an;
    an >>= ANGLETOFINESHIFT;
    mo->momx = FixedMul(mo->info->speed, finecosine[an]);
    mo->momy = FixedMul(mo->info->speed, finesine[an]);

    // Arm4

    an = (actor->angle+ANG90) >> ANGLETOFINESHIFT;
    x = FixedMul(50*FRACUNIT, finecosine[an]);
    y = FixedMul(50*FRACUNIT, finesine[an]);
    mo = P_SpawnMobj(actor->x + x, actor->y + y, actor->z + 139*FRACUNIT, MT_PROJ_RECT);
    P_SetTarget(&mo->target, actor);
    P_SetTarget(&mo->tracer, actor->target);
    mo->threshold = 1;
    an = (actor->angle - ANG270);
    mo->angle = an;
    an >>= ANGLETOFINESHIFT;
    mo->momx = FixedMul(mo->info->speed, finecosine[an]);
    mo->momy = FixedMul(mo->info->speed, finesine[an]);
}

//
// A_RectTracer
//

void A_RectTracer(mobj_t* actor) {
    if(actor->threshold < 0) {
        A_Tracer(actor);
    }
    else {
        actor->threshold--;
    }
}

//
// A_RectGroundFire
//

void A_RectGroundFire(mobj_t* actor) {
    mobj_t* mo;
    angle_t an;

    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);

    mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_PROJ_RECTFIRE);
    P_SetTarget(&mo->target, actor);
    an = actor->angle + R_PointToAngle2(actor->x, actor->y, mo->target->x, mo->target->y);

    mo->angle = an;
    mo->angle >>= ANGLETOFINESHIFT;
    mo->momx = FixedMul(mo->info->speed, finecosine[mo->angle]);
    mo->momy = FixedMul(mo->info->speed, finesine[mo->angle]);
    mo->angle = an;

    mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_PROJ_RECTFIRE);
    P_SetTarget(&mo->target, actor);
    mo->angle = an - ANG45;
    mo->angle >>= ANGLETOFINESHIFT;
    mo->momx = FixedMul(mo->info->speed, finecosine[mo->angle]);
    mo->momy = FixedMul(mo->info->speed, finesine[mo->angle]);
    mo->angle = an - ANG45;

    mo = P_SpawnMobj(actor->x, actor->y, actor->z, MT_PROJ_RECTFIRE);
    P_SetTarget(&mo->target, actor);
    mo->angle = an + ANG45;
    mo->angle >>= ANGLETOFINESHIFT;
    mo->momx = FixedMul(mo->info->speed, finecosine[mo->angle]);
    mo->momy = FixedMul(mo->info->speed, finesine[mo->angle]);
    mo->angle = an + ANG45;

    S_StartSound(mo, mo->info->seesound);
}

//
// A_MoveGroundFire
//

void A_MoveGroundFire(mobj_t* fire) {
    mobj_t* mo;

    if(fire->z > fire->floorz && fire->momz > 0) {
        fire->z = fire->floorz;
    }

    mo = P_SpawnMobj(fire->x, fire->y, fire->floorz, MT_PROP_FIRE);
    P_FadeMobj(mo, -8, 0, 0);
}

//
// A_RectDeathEvent
//

void A_RectDeathEvent(mobj_t* actor) {
    mobjexp_t *exp;

    exp = (mobjexp_t*) Z_Calloc(sizeof(*exp), PU_LEVSPEC, 0);
    P_AddThinker(&exp->thinker);

    exp->thinker.function.acp1 = (actionf_p1)T_MobjExplode;
    exp->delaymax = 3;
    exp->delay = 0;
    exp->lifetime = 32;
    P_SetTarget(&exp->mobj, actor);

    if(actor->info->deathsound) {
        S_StartSound(NULL, actor->info->deathsound);
    }
}

// A_FatRaise
//
// Mancubus attack,
// firing three missiles (bruisers)
// in three different directions?
// Doesn't look like it.
//

void A_FatRaise(mobj_t *actor) {
    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    S_StartSound(actor, sfx_fattatk);
}

//
// A_FatAttack1
//

#define    FATSPREAD    0x10000000

void A_FatAttack1(mobj_t* actor) {
    mobj_t *mo;
    angle_t an;

    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    P_MissileAttack(actor, DP_RIGHT);
    mo = P_MissileAttack(actor, DP_LEFT);

    mo->angle += FATSPREAD;
    an = mo->angle >> ANGLETOFINESHIFT;

    mo->momx = FixedMul(mo->info->speed, finecosine[an]);
    mo->momy = FixedMul(mo->info->speed, finesine[an]);
}

//
// A_FatAttack2
//

void A_FatAttack2(mobj_t* actor) {
    mobj_t *mo;
    angle_t an;

    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    P_MissileAttack(actor, DP_LEFT);
    mo = P_MissileAttack(actor, DP_RIGHT);

    mo->angle -= FATSPREAD;
    an = mo->angle >> ANGLETOFINESHIFT;

    mo->momx = FixedMul(mo->info->speed, finecosine[an]);
    mo->momy = FixedMul(mo->info->speed, finesine[an]);
}

//
// A_FatAttack3
//

void A_FatAttack3(mobj_t* actor) {
    mobj_t *mo;
    angle_t an;

    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    mo = P_MissileAttack(actor, DP_RIGHT);
    mo->angle -= 0x4000000;
    an = mo->angle >> ANGLETOFINESHIFT;

    mo->momx = FixedMul(mo->info->speed, finecosine[an]);
    mo->momy = FixedMul(mo->info->speed, finesine[an]);

    mo = P_MissileAttack(actor, DP_LEFT);
    mo->angle += 0x4000000;
    an = mo->angle >> ANGLETOFINESHIFT;

    mo->momx = FixedMul(mo->info->speed, finecosine[an]);
    mo->momy = FixedMul(mo->info->speed, finesine[an]);
}


//
// A_SkullAttack
// Fly at the player like a missile.
//
#define    SKULLSPEED      (40*FRACUNIT)

void A_SkullAttack(mobj_t* actor) {
    mobj_t*        dest;
    angle_t        an;
    int            dist;

    if(!actor->target) {
        return;
    }

    dest = actor->target;
    actor->flags |= MF_SKULLFLY;

    S_StartSound(actor, actor->info->attacksound);
    A_FaceTarget(actor);

    an = actor->angle >> ANGLETOFINESHIFT;
    actor->momx = FixedMul(SKULLSPEED, finecosine[an]);
    actor->momy = FixedMul(SKULLSPEED, finesine[an]);
    dist = P_AproxDistance(dest->x - actor->x, dest->y - actor->y);
    dist = dist / SKULLSPEED;

    if(dist < 1) {
        dist = 1;
    }

    actor->momz = (dest->z + (dest->height >> 1) - actor->z) / dist;
}

//
// A_SkullSetAlpha
//

void A_SkullSetAlpha(mobj_t* actor) {
    actor->alpha >>= 2;
}

//
// PIT_PainCheckLine
//

static dboolean PIT_PainCheckLine(intercept_t *in) {
    if(!in->d.line->backsector) {
        return false;
    }

    return true;
}

//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//

void A_PainShootSkull(mobj_t* actor, angle_t angle) {
    fixed_t     x;
    fixed_t     y;
    fixed_t     z;
    mobj_t*     newmobj;
    angle_t     an;
    int         prestep;
    int         count;

    // count total number of skull currently on the level
    count = 0;

    for(newmobj = mobjhead.next; newmobj != &mobjhead; newmobj = newmobj->next) {
        if(newmobj->type == MT_SKULL) {
            count++;
        }
    }

    //
    // if there are all ready 17 skulls on the level,
    // don't spit another one
    // 20120212 villsa - new compatibility flag to disable limit
    //
    if(compatflags & COMPATF_LIMITPAIN && count > 0x11) {
        return;
    }


    an = angle >> ANGLETOFINESHIFT;

    prestep = 4*FRACUNIT + 3 * (actor->info->radius + mobjinfo[MT_SKULL].radius) / 2;

    x = actor->x + FixedMul(prestep, finecosine[an]);
    y = actor->y + FixedMul(prestep, finesine[an]);
    z = actor->z + 16*FRACUNIT;

    newmobj = P_SpawnMobj(x, y, z, MT_SKULL);

    // Check for movements

    if((!P_TryMove(newmobj, newmobj->x, newmobj->y)) ||
            (!P_PathTraverse(actor->x, actor->y, newmobj->x, newmobj->y, PT_ADDLINES, PIT_PainCheckLine))) {
        // kill it immediately

        P_DamageMobj(newmobj, actor, actor, 10000);
        P_RadiusAttack(newmobj, newmobj, 128);
        return;
    }

    P_SetTarget(&newmobj->target, actor->target);
    P_SetMobjState(newmobj, newmobj->info->missilestate);
    A_SkullAttack(newmobj);
}


//
// A_PainAttack
// Spawn a lost soul and launch it at the target
//

void A_PainAttack(mobj_t* actor) {
    if(!actor->target) {
        return;
    }

    A_FaceTarget(actor);
    A_PainShootSkull(actor, actor->angle+0x15550000);
    A_PainShootSkull(actor, actor->angle-0x15550000);
}

//
// A_PainDie
//

void A_PainDie(mobj_t* actor) {
    A_Fall(actor);
    A_PainShootSkull(actor, actor->angle+ANG90);
    A_PainShootSkull(actor, actor->angle+ANG180);
    A_PainShootSkull(actor, actor->angle+ANG270);

    A_OnDeathTrigger(actor);
}

//
// A_PainDeathEvent
//

void A_PainDeathEvent(mobj_t* actor) {
    actor->alpha -= 0x3F;
}

//
// A_FadeOut
//

void A_FadeOut(mobj_t* actor) {
    P_FadeMobj(actor, -8, 0x30, 0);
}

//
// A_FadeIn
//

void A_FadeIn(mobj_t* actor) {
    P_FadeMobj(actor, 8, 0xff, 0);
}

//
// A_Scream
//

void A_Scream(mobj_t* actor) {
    int sound;

    switch(actor->info->deathsound) {
    case 0:
        return;

    case sfx_posdie1:
    case sfx_posdie2:
    case sfx_posdie3:
        sound = sfx_posdie1 + (P_Random(pr_scream) & 1);
        break;

    case sfx_impdth1:
    case sfx_impdth2:
        sound = sfx_impdth1 + (P_Random(pr_scream) & 1);
        break;

    default:
        sound = actor->info->deathsound;
        break;
    }

    S_StartSound(actor, sound);
}

//
// A_XScream
//

void A_XScream(mobj_t* actor) {
    S_StartSound(actor, sfx_slop);
}

//
// A_Pain
//

void A_Pain(mobj_t* actor) {
    if(actor->info->painsound) {
        if(actor->type == MT_RESURRECTOR) {
            S_StartSound(NULL, actor->info->painsound);
        }
        else {
            S_StartSound(actor, actor->info->painsound);
        }
    }
}

//
// A_Fall
//

void A_Fall(mobj_t *actor) {
    // actor is on ground, it can be walked over
    actor->flags &= ~MF_SOLID;
    actor->blockflag |= BF_MIDPOINTONLY;
}


//
// A_Explode
//

void A_Explode(mobj_t* thingy) {
    P_RadiusAttack(thingy, thingy->target, 128);
}

//
// A_BarrelExplode
//

void A_BarrelExplode(mobj_t* actor) {
    mobj_t* exp;

    S_StartSound(actor, sfx_explode);
    exp = P_SpawnMobj(actor->x+FRACUNIT, actor->y+FRACUNIT, actor->z+(actor->height<<1), MT_EXPLOSION1);
    A_Explode(actor);

    A_OnDeathTrigger(actor);
}

//
// A_Hoof
//

void A_Hoof(mobj_t* mo) {
    S_StartSound(mo, sfx_cybhoof);
    A_Chase(mo);
}

//
// A_Metal
//

void A_Metal(mobj_t* mo) {
    S_StartSound(mo, sfx_metal);
    A_Chase(mo);
}

//
// A_BabyMetal
//

void A_BabyMetal(mobj_t* mo) {
    S_StartSound(mo, sfx_bspistomp);
    A_Chase(mo);
}

//
// A_PlayerScream
//

void A_PlayerScream(mobj_t* mo) {
    int    sound = sfx_plrdie;
    S_StartSound(mo, sound);
}

//
// A_SpawnSmoke
//

void A_SpawnSmoke(mobj_t *mobj) {
    mobj_t *smoke = P_SpawnMobj(mobj->x, mobj->y, mobj->z, MT_SMOKE_GRAY);
    smoke->momz = FRACUNIT;
}

//
// A_MissileSetAlpha
//

void A_MissileSetAlpha(mobj_t* actor) {
    actor->alpha >>= 1;
}

//
// A_FadeAlpha
//

void A_FadeAlpha(mobj_t *mobj) {
    int fade = 0;

    fade = ((mobj->alpha << 2) - mobj->alpha);
    if(!(fade >= 0)) {
        fade = fade + 3;
    }

    mobj->alpha = (fade >> 2);
}

//
// A_TargetCamera
//

void A_TargetCamera(mobj_t* actor) {
    mobj_t* mo;

    actor->threshold = D_MAXINT;

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        if(actor->tid+1 == mo->tid) {
            P_SetTarget(&actor->target, mo);
            P_SetMobjState(actor, actor->info->missilestate);
            return;
        }
    }
}

} // extern "C"
