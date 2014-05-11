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
//      Moving object handling. Spawn functions.
//
//-----------------------------------------------------------------------------

#include "i_system.h"
#include "z_zone.h"
#include "m_fixed.h"
#include "m_random.h"
#include "doomdef.h"
#include "p_local.h"
#include "sounds.h"
#include "st_stuff.h"
#include "s_sound.h"
#include "doomstat.h"
#include "info.h"
#include "tables.h"
#include "r_local.h"
#include "r_sky.h"
#include "m_misc.h"
#include "con_console.h"
#include "m_password.h"

mapthing_t* spawnlist;
int         numspawnlist;

void G_PlayerReborn(int player);
mobj_t* P_SpawnMapThing(mapthing_t* mthing);
void P_CreateFadeThinker(mobj_t* mobj, line_t* line);
void P_CreateFadeOutThinker(mobj_t* mobj, line_t* line);


//
// P_SetMobjState
// Returns true if the mobj is still present.
//

dboolean P_SetMobjState(mobj_t* mobj, statenum_t state) {
    state_t* st;

    do {
        if(!mobj->state || state == S_000) {
            mobj->state = (state_t *)S_000;
            P_RemoveMobj(mobj);
            return false;
        }

        st = &states[state];
        mobj->frame = st->frame;

        mobj->state = st;
        mobj->tics = st->tics;
        mobj->sprite = st->sprite;

        // Modified handling.
        // Call action functions when the state is set

        if(st->action.acp1) {
            st->action.acp1(mobj);
        }
        else {
            mobj->mobjfunc = NULL;    // [d64]
        }

        state = st->nextstate;

    }
    while(!mobj->tics);

    return true;
}

//
// P_SetTarget
//
// This function is used to keep track of pointer references to mobj thinkers.
// In Doom, objects such as lost souls could sometimes be removed despite
// their still being referenced. In Boom, 'target' mobj fields were tested
// during each gametic, and any objects pointed to by them would be prevented
// from being removed. But this was incomplete, and was slow (every mobj was
// checked during every gametic). Now, we keep a count of the number of
// references, and delay removal until the count is 0.
//
//

void P_SetTarget(mobj_t **mop, mobj_t *targ) {
    // If there was a target already, decrease its refcount
    if(*mop) {
        Z_Touch(*mop); // validate pointer
        (*mop)->refcount--;
    }

    // Set new target and if non-NULL, increase its counter
    if((*mop = targ)) {
        targ->refcount++;
    }
}


//
// P_ExplodeMissile
//

void P_ExplodeMissile(mobj_t* mo) {
    if(!P_SetMobjState(mo, mobjinfo[mo->type].deathstate)) {
        return;
    }

    mo->momx = mo->momy = mo->momz = 0;

    mo->tics -= P_Random(pr_explode) & 3;

    if(mo->tics < 1) {
        mo->tics = 1;
    }

    mo->flags &= ~MF_MISSILE;

    if(mo->info->deathsound) {
        S_StopSound(mo, 0);
        S_StartSound(mo, mo->info->deathsound);
    }
}

//
// P_MissileHit
//

void P_MissileHit(mobj_t* mo) {
    int     damage;
    mobj_t* missilething;

    missilething = (mobj_t*)mo->extradata;
    damage = 0;

    if(missilething) {
        damage = ((P_Random(pr_damage) & 7) + 1) * mo->info->damage;
        P_DamageMobj(missilething, mo, mo->target, damage);

        if(mo->type == MT_PROJ_RECTFIRE) {
            if(missilething->player && missilething->info->mass) {
                missilething->momz += ((1500 / missilething->info->mass) * FRACUNIT);
            }
        }
    }

    if(mo->type == MT_PROJ_DART) {
        if(missilething && !(missilething->flags & MF_NOBLOOD)) {
            P_SpawnBlood(mo->x, mo->y, mo->z, damage);
        }
        else {
            S_StartSound(mo, sfx_darthit);
            P_SpawnPuff(mo->x, mo->y, mo->z);
        }
    }

    P_ExplodeMissile(mo);
}

//
// P_SkullBash
// A move in P_MobjThinker caused a flying skull to hit
// another thing or a wall
//

void P_SkullBash(mobj_t* mo) {
    int     damage;
    mobj_t* skullthing;

    skullthing = (mobj_t*)mo->extradata;

    if(skullthing) {
        damage = ((P_Random(pr_skullfly) & 7) + 1) * mo->info->damage;
        P_DamageMobj(skullthing, mo, mo, damage);
    }

    mo->flags &= ~MF_SKULLFLY;
    mo->momx = mo->momy = mo->momz = 0;
    P_SetMobjState(mo, mo->info->spawnstate);
}


//
// P_XYMovement
//

void P_XYMovement(mobj_t* mo) {
    fixed_t     ptryx;
    fixed_t     ptryy;
    fixed_t     xmove;
    fixed_t     ymove;

    //
    // [d64] don't clamp momx/momy
    //

    //
    // [d64] round off momentum bits
    //
    xmove = (mo->momx & ~7);
    ymove = (mo->momy & ~7);

    do {
        // [d64] fixed bug with fast projectiles going through walls
        if((xmove > MAXMOVE || ymove > MAXMOVE)
                || (xmove < -MAXMOVE || ymove < -MAXMOVE)) {
            ptryx = mo->x + xmove/2;
            ptryy = mo->y + ymove/2;
            xmove >>= 1;
            ymove >>= 1;
        }
        else {
            ptryx = mo->x + xmove;
            ptryy = mo->y + ymove;
            xmove = ymove = 0;
        }

        if(!P_TryMove(mo, ptryx, ptryy)) {
            if(mo->flags & MF_SKULLFLY) {
                mo->extradata = (mobj_t*)blockthing;
                mo->mobjfunc = P_SkullBash;
            }

            if(mo->flags & MF_MISSILE) {
                // explode a missile
                if(tmhitline) {
                    if((tmhitline->backsector &&
                        tmhitline->backsector->ceilingpic == skyflatnum) ||
                        sides[tmhitline->sidenum[0]].midtexture == 1) {
                        // Hack to prevent missiles exploding
                        // against the sky.
                        // Does not handle sky floors.
                        mo->mobjfunc = P_RemoveMobj;
                        return;
                    }
                }

                mo->mobjfunc = P_MissileHit;
                mo->extradata = (mobj_t*)blockthing;
                return;
            }
            else {
                mo->momx = mo->momy = 0;
                return;
            }
        }
    }
    while(xmove || ymove);

    if(mo->flags & (MF_MISSILE | MF_SKULLFLY)) {
        return;    // no friction for missiles ever
    }

    if(mo->z > mo->floorz && (!(mo->blockflag & BF_MOBJSTAND))) {
        return;    // no friction when airborne
    }

    if(mo->flags & MF_CORPSE) {
        // do not stop sliding
        //  if halfway off a step with some momentum
        if(mo->momx > FRACUNIT/4
                || mo->momx < -FRACUNIT/4
                || mo->momy > FRACUNIT/4
                || mo->momy < -FRACUNIT/4) {
            if(mo->floorz != mo->subsector->sector->floorheight) {
                return;
            }
        }
    }

    if(mo->momx > -STOPSPEED && mo->momx < STOPSPEED
            && mo->momy > -STOPSPEED && mo->momy < STOPSPEED) {
        // if in a walking frame, stop moving
        mo->momx = 0;
        mo->momy = 0;
    }
    else {
        mo->momx = FixedMul(mo->momx, FRICTION);
        mo->momy = FixedMul(mo->momy, FRICTION);
    }
}

//
// P_ZMovement
//
void P_ZMovement(mobj_t* mo, dboolean checkmissile) {
    fixed_t     dist;
    fixed_t     delta;

    // adjust height
    mo->z += mo->momz;

    if(mo->flags & MF_FLOAT && mo->target) {
        // float down towards target if too close
        //if(!(mo->flags & MF_INFLOAT))
        {
            dist = P_AproxDistance(mo->x - mo->target->x,
                                   mo->y - mo->target->y);

            delta = (mo->target->z + (mo->height>>1)) - mo->z;

            if(delta < 0 && dist < -(delta*3)) {
                mo->z -= FLOATSPEED;
            }
            else if(delta > 0 && dist < (delta*3)) {
                mo->z += FLOATSPEED;
            }
        }

    }

    // clip movement
    if(mo->z <= mo->floorz) {
        // hit the floor

        if(mo->momz < 0) {
            mo->momz = 0;
        }

        mo->z = mo->floorz;

        if(checkmissile) {
            if((mo->flags & MF_MISSILE)
                    && !(mo->flags & MF_NOCLIP)
                    && !(mo->type == MT_PROJ_RECTFIRE)) {
                mo->mobjfunc = P_ExplodeMissile;
                return;
            }
        }
    }
    else if((mo->flags & MF_GRAVITY)) {
        if(mo->momz == 0) {
            mo->momz = -GRAVITY;
        }
        else {
            mo->momz -= GRAVITY;
        }
    }

    if(mo->z + mo->height > mo->ceilingz) {
        // hit the ceiling
        if(mo->momz > 0) {
            mo->momz = 0;
        }

        mo->z = mo->ceilingz - mo->height;

        if(!checkmissile) {
            return;
        }

        if(mo->flags & MF_MISSILE &&
                mo->subsector->sector->ceilingpic == skyflatnum) {
            mo->mobjfunc = P_RemoveMobj;
            return;
        }

        if((mo->flags & MF_MISSILE)
                && !(mo->flags & MF_NOCLIP)) {
            mo->mobjfunc = P_ExplodeMissile;
            return;
        }
    }
}

//
// P_NightmareRespawn
//
void P_NightmareRespawn(mobj_t* mobj) {
    fixed_t             x;
    fixed_t             y;
    fixed_t             z;
    mobj_t*             mo;
    mapthing_t*         mthing;
    line_t              junk;

    mobj->movecount++;

    if(mobj->movecount < 12*TICRATE) {
        return;
    }

    if(leveltime & 31) {
        return;
    }

    if(P_Random(pr_respawn) > 4) {
        return;
    }

    x = INT2F(mobj->spawnpoint.x);
    y = INT2F(mobj->spawnpoint.y);

    // somthing is occupying it's position?
    if(!P_CheckPosition(mobj, x, y)) {
        return;    // no respawn
    }

    // 20120301 villsa - don't respawn in insta-kill sector
    if(R_PointInSubsector(x, y)->sector->special == 666) {
        mobj->flags &= ~MF_COUNTKILL;   // don't bother checking for respawns again
        return;
    }

    // spawn the new monster
    mthing = &mobj->spawnpoint;

    // spawn it
    if(mobj->info->flags & MF_SPAWNCEILING) {
        z = ONCEILINGZ;
    }
    else {
        z = ONFLOORZ;
    }

    dmemset(&junk, 0, sizeof(line_t));

    // inherit attributes from deceased one
    mo = P_SpawnMobj(x, y, z, mobj->type);
    mo->spawnpoint = mobj->spawnpoint;
    mo->angle = ANG45 * (mthing->angle / 45);

    // 20120212 villsa - fix for respawning spectures
    if(mo->type != MT_DEMON2) {
        mo->alpha = 0;
        P_CreateFadeThinker(mo, &junk);
    }
    else {
        mo->alpha = 0x30;
    }

    // initiate spawn sound
    S_StartSound(mo, sfx_spawn);

    if(mthing->options & MTF_AMBUSH) {
        mo->flags |= MF_AMBUSH;
    }

    mo->reactiontime = 18;

    // remove the old monster
    P_CreateFadeOutThinker(mobj, &junk);
}

//
// P_RespawnSpecials
//

void P_RespawnSpecials(mobj_t* special) {
    if(special->alpha != 80) {
        special->alpha = 80;
    }

    // wait at least 30 seconds
    if(leveltime - special->reactiontime < 30*TICRATE) {
        return;
    }

    special->reactiontime = 0;
    special->mobjfunc = NULL;

    P_FadeMobj(special, 8, 0xff, (special->flags | MF_SPECIAL));
    S_StartSound(special, sfx_spawn);
}

//
// P_OnMobjZ
//

dboolean P_OnMobjZ(mobj_t* mobj) {
    mobj_t* onmo;

    if(mobj->blockflag & BF_MOBJPASS) {
        if(!(onmo = P_CheckOnMobj(mobj))) {
            if(mobj->player && mobj->blockflag & BF_MOBJSTAND) {
                mobj->blockflag &= ~BF_MOBJSTAND;
            }
            return false;
        }
        else {
            if(mobj->player) {
                if(onmo->z + onmo->height - mobj->z <= 24*FRACUNIT) {
                    mobj->player->viewheight -= onmo->z + onmo->height - mobj->z;
                    mobj->player->deltaviewheight = (VIEWHEIGHT - mobj->player->viewheight) >> 3;
                    mobj->z = onmo->z + onmo->height;
                    mobj->blockflag |= BF_MOBJSTAND;
                    mobj->player->onground = 1;
                    mobj->momz = 0;
                }
                else {
                    mobj->momz = 0;
                }
            }
        }
        return true;
    }

    return false;
}


//
// P_MobjThinker
//

void P_MobjThinker(mobj_t* mobj) {
    blockthing = NULL;

    // momentum movement
    if(mobj->momx || mobj->momy) {
        P_XYMovement(mobj);
    }

    if(mobj->mobjfunc && mobj->mobjfunc != P_RespawnSpecials) {
        return;
    }

    if((mobj->z != mobj->floorz) || mobj->momz || blockthing) {
        if(!P_OnMobjZ(mobj)) {
            P_ZMovement(mobj, true);
        }
    }

    if(mobj->mobjfunc) {
        return;
    }

    // cycle through states,
    // calling action functions at transitions
    if(mobj->tics != -1) {
        mobj->tics--;

        // you can cycle through multiple states in a tic
        if(!mobj->tics) {
            if(!mobj->state) {
                return;
            }

            if(!P_SetMobjState(mobj, mobj->state->nextstate)) {
                return;    // freed itself
            }
        }
    }
    else {
        // check for nightmare respawn
        if(!(mobj->flags & MF_COUNTKILL)) {
            return;
        }

        if(!respawnmonsters) {
            return;
        }

        mobj->momx = mobj->momy = mobj->momz = 0;
        P_NightmareRespawn(mobj);
    }
}


//
// P_SpawnMobj
//

mobj_t* P_SpawnMobj(fixed_t x, fixed_t y, fixed_t z, mobjtype_t type) {
    mobj_t*     mobj;
    state_t*    st;
    mobjinfo_t* info;

    mobj = Z_Malloc(sizeof(*mobj), PU_LEVEL, NULL);
    dmemset(mobj, 0, sizeof(*mobj));
    info = &mobjinfo[type];

    mobj->type      = type;
    mobj->info      = info;
    mobj->x         = x;
    mobj->y         = y;
    mobj->pitch     = 0;
    mobj->radius    = info->radius;
    mobj->height    = info->height;
    mobj->flags     = info->flags;
    mobj->health    = info->spawnhealth;
    mobj->alpha     = info->alpha;
    mobj->blockflag = 0;
    mobj->frame_x   = mobj->x;
    mobj->frame_y   = mobj->y;
    mobj->mobjfunc  = NULL;
    mobj->extradata = NULL;
    mobj->target    = NULL;
    mobj->tracer    = NULL;
    mobj->refcount  = 0;    // init reference counter to 0

    if(mobj->flags & MF_SOLID &&
            compatflags & COMPATF_MOBJPASS &&
            !(mobj->flags & (MF_NOCLIP|MF_SPECIAL))) {
        mobj->blockflag |= BF_MOBJPASS;
    }

    if(gameskill != sk_nightmare) {
        mobj->reactiontime = info->reactiontime;
    }

    // [kex] 12/26/11 - don't increment stats when loading a savegame
    if(gameaction != ga_loadgame) {
        if(mobj->flags & MF_COUNTKILL) {
            totalkills++;
        }

        if(mobj->flags & MF_COUNTITEM) {
            totalitems++;
        }
    }

    // do not set the state with P_SetMobjState,
    // because action routines can not be called yet

    st              = &states[info->spawnstate];
    mobj->state     = st;
    mobj->tics      = st->tics;
    mobj->sprite    = st->sprite;
    mobj->frame     = st->frame;

    P_SetThingPosition(mobj);   // set subsector and/or block links

    mobj->floorz    = mobj->subsector->sector->floorheight;
    mobj->ceilingz  = mobj->subsector->sector->ceilingheight;

    if(z == ONFLOORZ) {
        mobj->z = mobj->frame_z = mobj->floorz;
    }
    else if(z == ONCEILINGZ) {
        mobj->z = mobj->frame_z = (mobj->ceilingz - mobj->info->height);
    }
    else {
        mobj->z = mobj->frame_z = z;
    }

    P_LinkMobj(mobj);       // add to list

    return mobj;
}

//
// P_SafeRemoveMobj
//

void P_SafeRemoveMobj(mobj_t* mobj) {
    if(!mobj->refcount) {
        P_UnlinkMobj(mobj); // unlink from mobj list
        Z_Free(mobj);       // free block
    }
}


//
// P_RemoveMobj
//

void P_RemoveMobj(mobj_t* mobj) {
    if(respawnspecials
            && ((mobj->flags & MF_SPECIAL)
                && !(mobj->flags & MF_DROPPED)
                && (mobj->type != MT_ITEM_INVULSPHERE)
                && (mobj->type != MT_ITEM_INVISSPHERE))) {
        // TODO: Don't require mobjfunc
        mobj->reactiontime = leveltime;
        mobj->flags &= ~MF_SPECIAL;
        mobj->alpha = 80;
        mobj->mobjfunc = P_RespawnSpecials;
        return;
    }

    // [kex] set targets to null and update reference id
    P_SetTarget(&mobj->target, NULL);
    P_SetTarget(&mobj->tracer, NULL);

    S_RemoveOrigin(mobj);       // unlink from sound channels
    P_UnsetThingPosition(mobj); // unlink from sector and block lists

    // [kex] set callback to remove mobj
    mobj->mobjfunc = P_SafeRemoveMobj;
}


//
// P_SpawnPlayer
// Called when a player is spawned on the level.
// Most of the player structure stays unchanged
//  between levels.
//
void P_SpawnPlayer(mapthing_t* mthing) {
    player_t*   p;
    fixed_t     x;
    fixed_t     y;
    mobj_t*     mobj;
    int         i;

    if(mthing->type == 0) {
        return;
    }

    // not playing?
    if(!playeringame[mthing->type-1]) {
        return;
    }

    p = &players[mthing->type-1];

    if(p->playerstate == PST_REBORN) {
        G_PlayerReborn(mthing->type-1);
    }

    if(!mthing->options) {
        I_Error("P_SpawnPlayer: attempt to spawn player at unavailable start point");
    }

    x = INT2F(mthing->x);
    y = INT2F(mthing->y);

    mobj = P_SpawnMobj(x, y, ONFLOORZ, MT_PLAYER);

    mobj->angle         = ANG45 * (mthing->angle/45);
    mobj->player        = p;
    mobj->health        = p->health;
    mobj->tid           = mthing->tid;
    mobj->z             = mobj->z + INT2F(mthing->z);

    p->mo               = mobj;
    p->playerstate      = PST_LIVE;
    p->refire           = 0;
    p->message          = NULL;
    p->messagepic       = -1;
    p->damagecount      = 0;
    p->bonuscount       = 0;
    p->bfgcount         = 0;
    p->viewheight       = VIEWHEIGHT;
    p->recoilpitch      = 0;
    p->palette          = mthing->type-1;
    p->cameratarget     = p->mo;

    // setup gun psprite
    P_SetupPsprites(p);

    // give all cards in death match mode
    if(deathmatch) {
        for(i = 0; i < NUMCARDS; i++) {
            p->cards[i] = true;
        }
    }

    ST_ClearMessage();
    ST_UpdateFlash();

    if(doPassword) {
        M_DecodePassword(0);
    }

    doPassword = false;
}

//
// T_MobjFadeThinker
//

void T_MobjFadeThinker(mobjfade_t* mobjfade) {
    mobj_t *mobj = mobjfade->mobj;

    if(mobj->alpha == mobjfade->destAlpha) {
        P_SetTarget(&mobjfade->mobj, NULL);
        P_RemoveThinker(&mobjfade->thinker);
        return;
    }

    mobj->alpha += mobjfade->amount;

    if(mobjfade->amount <= 0) {
        if(mobjfade->destAlpha < mobj->alpha) {
            return;
        }

        mobj->alpha = mobjfade->destAlpha;

        if(mobjfade->destAlpha == 0) {
            P_RemoveMobj(mobj);
        }

        P_SetTarget(&mobjfade->mobj, NULL);
        P_RemoveThinker(&mobjfade->thinker);
        return;
    }

    if(mobj->alpha < mobjfade->destAlpha) {
        return;
    }

    mobj->alpha = mobjfade->destAlpha;
    if(mobjfade->flagReserve) {
        mobj->flags |= mobjfade->flagReserve;
    }

    P_SetTarget(&mobjfade->mobj, NULL);
    P_RemoveThinker(&mobjfade->thinker);
}

//
// P_FadeMobj
// Generic routine for fading in/out mobjs
//

void P_FadeMobj(mobj_t* mobj, int amount, int alpha, int flags) {
    mobjfade_t *mobjfade;

    mobjfade = Z_Calloc(sizeof(*mobjfade), PU_LEVSPEC, 0);
    P_AddThinker(&mobjfade->thinker);
    mobjfade->thinker.function.acp1 = (actionf_p1)T_MobjFadeThinker;
    P_SetTarget(&mobjfade->mobj, mobj);
    mobjfade->amount = amount;
    mobjfade->destAlpha = alpha;
    mobjfade->flagReserve = flags;
}

//
// P_CreateFadeThinker
//

void P_CreateFadeThinker(mobj_t* mobj, line_t* line) {
    int flags = 0;

    if(mobj->flags & MF_SHOOTABLE) {
        flags |= MF_SHOOTABLE;
        mobj->flags &= ~MF_SHOOTABLE;
    }
    if(mobj->flags & MF_SPECIAL) {
        flags |= MF_SPECIAL;
        mobj->flags &= ~MF_SPECIAL;
    }

    P_FadeMobj(mobj, 8, mobj->info->alpha, flags);
}

//
// P_CreateFadeOutThinker
//

void P_CreateFadeOutThinker(mobj_t* mobj, line_t* line) {
    int flags;

    if(mobj->flags & MF_SHOOTABLE) {
        flags |= MF_SHOOTABLE;
        mobj->flags &= ~MF_SHOOTABLE;
    }
    if(mobj->flags & MF_SPECIAL) {
        flags |= MF_SPECIAL;
        mobj->flags &= ~MF_SPECIAL;
    }

    P_FadeMobj(mobj, -8, 0, flags);
}

//
// EV_SpawnMobjTemplate
//

int EV_SpawnMobjTemplate(line_t* line) {
    mobj_t*    mobj;
    int i;
    dboolean ok = false;
    mapthing_t* mthing;

    for(i = 0; i < numspawnlist; i++) {
        mthing = &spawnlist[i];

        // find matching tid
        if(mthing->tid != line->tag) {
            continue;
        }

        // setup spawning
        if(!(mobj = P_SpawnMapThing(mthing))) {
            continue;
        }

        // somthing is occupying it's position?
        if(mobj->flags & MF_SOLID && (!P_CheckPosition(mobj, mobj->x, mobj->y))) {
            P_RemoveMobj(mobj);
            continue;
        }

        mobj->reactiontime = 18;

        S_StartSound(mobj, sfx_spawn);

        if(mobj->type != MT_DEMON2) {
            mobj->alpha = 0;
            P_CreateFadeThinker(mobj, line);
        }
        else {
            mobj->alpha = 0x30;
        }

        ok = true;
    }

    return ok;
}

//
// EV_FadeOutMobj
//

int EV_FadeOutMobj(line_t* line) {
    mobj_t* mo;
    dboolean ok = false;

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        // not matching the tid

        if(mo->tid != line->tag) {
            continue;
        }

        // don't remove teleportmans

        if(mo->type == MT_DEST_TELEPORT) {
            continue;
        }

        if(mo->flags & MF_SPECIAL) {
            mo->flags &= ~MF_SPECIAL;
        }

        if(mo->flags & MF_TRIGDEATH) {
            mo->flags = 0;
        }

        if(netgame && respawnspecials) {
            if(mo->mobjfunc == P_RespawnSpecials) {
                mo->mobjfunc = NULL;
                mo->alpha = 0xff;
            }
        }

        P_CreateFadeOutThinker(mo, line);
        ok = true;
    }

    return ok;
}

//
// P_SpawnMapThing
//
// The fields of the mapthing should
// already be in host byte order.
//

mobj_t* P_SpawnMapThing(mapthing_t* mthing) {
    int                 i;
    int                 bit;
    mobj_t*             mobj;
    fixed_t             x;
    fixed_t             y;
    fixed_t             z;

    // count deathmatch start positions

    if(mthing->type == 11) {
        if(deathmatch_p < &deathmatchstarts[10]) {
            dmemcpy(deathmatch_p, mthing, sizeof(*mthing));
            deathmatch_p++;
        }
        return NULL;
    }

    // check for players specially

    if(mthing->type <= 4 && mthing->type > 0) {
        // save spots for respawning in network games
        playerstarts[mthing->type-1] = *mthing;
        return NULL;
    }

    // check for apropriate skill level

    if(!(netgame) && (mthing->options & MTF_MULTI)) {
        return NULL;
    }

    if(gameskill == sk_baby) {
        bit = 1;
    }
    else if(gameskill == sk_nightmare) {
        bit = 4;
    }
    else {
        bit = 1 << (gameskill - 1);
    }

    if(!(mthing->options & bit)) {
        return NULL;
    }

    // find which type to spawn
    for(i = 0; i < NUMMOBJTYPES; i++) {
        if(mthing->type == mobjinfo[i].doomednum) {
            break;
        }
    }

    if(i >= NUMMOBJTYPES)
        I_Error("P_SpawnMapThing: Unknown type %i at (%i, %i)",
                mthing->type,
                mthing->x, mthing->y);

    // don't spawn keycards and players in deathmatch

    if(deathmatch && mobjinfo[i].flags & MF_NOTDMATCH) {
        return NULL;
    }

    // don't spawn any monsters if -nomonsters

    if(nomonsters
            && (i == MT_SKULL
                || (mobjinfo[i].flags & MF_COUNTKILL))) {
        return NULL;
    }

    if(mthing->options & MTF_SPAWN) {
        mthing->options &= ~MTF_SPAWN;
        dmemcpy(&spawnlist[numspawnlist++], mthing, sizeof(mapthing_t));

        return NULL;
    }

    // spawn it

    x = INT2F(mthing->x);
    y = INT2F(mthing->y);

    if(mobjinfo[i].flags & MF_SPAWNCEILING) {
        z = ONCEILINGZ;
    }
    else {
        z = ONFLOORZ;
    }

    mobj = P_SpawnMobj(x, y, z, i);
    mobj->z += INT2F(mthing->z);
    mobj->spawnpoint = *mthing;
    mobj->tid = mthing->tid;

    if(mobj->flags & MF_SOLID &&
            compatflags & COMPATF_MOBJPASS &&
            !(mobj->flags & (MF_NOCLIP|MF_SPECIAL))) {
        mobj->blockflag |= BF_MOBJPASS;
    }

    //
    // [d64] check if spawn is valid
    //
    if(mobj->flags & MF_SOLID && !mobj->player) {
        if(!P_CheckPosition(mobj, mobj->x, mobj->y)) {
            P_RemoveMobj(mobj);
            return NULL;
        }
    }

    if(mobj->tics > 0) {
        mobj->tics = 1 + (P_Random(pr_spawnthing) % mobj->tics);
    }

    mobj->angle = ANG45 * (mthing->angle/45);
    if(mthing->options & MTF_AMBUSH) {
        mobj->flags |= MF_AMBUSH;
    }

    if(mthing->options & MTF_ONDEATH) {
        mobj->flags |= MF_TRIGDEATH;
    }

    if(mthing->options & MTF_ONTOUCH) {
        mobj->flags |= MF_TRIGTOUCH;
    }

    if(mthing->options & MTF_NOINFIGHTING) {
        mobj->flags |= MF_NOINFIGHTING;
    }

    // [kex] check for nightmare flag
    if(mthing->options & MTF_NIGHTMARE) {
        mobj->health *= 2;
        mobj->flags |= MF_NIGHTMARE;
    }

    if(mthing->options & MTF_SECRET) {
        mobj->flags |= MF_COUNTSECRET;
        totalsecret++;
    }

    // At least set BF_MIDPOINTONLY if no flags exist..
    if(mobj->flags == 0) {
        mobj->blockflag |= BF_MIDPOINTONLY;
    }

    return mobj;
}

//
// GAME SPAWN FUNCTIONS
//


//
// P_SpawnPuff
//
extern fixed_t attackrange;

void P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z) {
    mobj_t* th;

    z += P_RandomShift(pr_spawnpuff, 10);

    th = P_SpawnMobj(x, y, z, MT_SMOKE_SMALL);
    th->momz = FRACUNIT;
    th->tics -= P_Random(pr_spawnpuff) & 3;

    if(th->tics < 1) {
        th->tics = 1;
    }

    // don't make punches spark on the wall
    if(attackrange == MELEERANGE) {
        P_SetMobjState(th, S_490);
    }
}

//
// P_SpawnBlood
//
void P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, int damage) {
    mobj_t* th;
    int i = 0;

    for(i = 0; i < 3; i++) {
        z += P_RandomShift(pr_spawnblood, 11);
        x += P_RandomShift(pr_spawnblood, 12);
        y += P_RandomShift(pr_spawnblood, 12);

        th = P_SpawnMobj(x, y, z, MT_BLOOD);
        th->momz = FRACUNIT*2;
        th->tics -= (P_Random(pr_spawnblood) & 1);

        if(th->tics < 1) {
            th->tics = 1;
        }

        if(damage < 9 && !(damage < 13)) {
            P_SetMobjState(th, S_495);
        }
        else if(damage < 9) {
            P_SetMobjState(th, S_496);
        }
    }
}


//
// P_SpawnPlayerMissile
// Tries to aim at a nearby monster
//

void P_SpawnPlayerMissile(mobj_t* source, mobjtype_t type) {
    mobj_t*     th;
    angle_t     an;
    fixed_t     x;
    fixed_t     y;
    fixed_t     z;
    fixed_t     slope;
    fixed_t     missileheight;
    int         offset;
    fixed_t     frac;

    // [d64] adjust offset and height based on missile
    if(type == MT_PROJ_ROCKET) {
        missileheight = (42*FRACUNIT);
        offset = 30;
    }
    else if(type == MT_PROJ_PLASMA) {
        missileheight = (32*FRACUNIT);
        offset = 40;
    }
    else if(type == MT_PROJ_BFG) {
        missileheight = (32*FRACUNIT);
        offset = 30;
    }
    else {
        missileheight = (32*FRACUNIT);
        offset = 0;
    }

    // see which target is to be aimed at
    an = source->angle;

    slope = P_AimLineAttack(source, an, missileheight, ATTACKRANGE);

    if(!linetarget) {
        an += 1<<26;
        slope = P_AimLineAttack(source, an, missileheight, ATTACKRANGE);

        if(!linetarget) {
            an -= 2<<26;
            slope = P_AimLineAttack(source, an, missileheight, ATTACKRANGE);
        }
    }

    if(!linetarget) {
        an = source->angle;
    }

    x = source->x;
    y = source->y;
    z = source->z + missileheight;

    th = P_SpawnMobj(x, y, z, type);

    if(th->info->seesound) {
        S_StartSound(th, th->info->seesound);
    }

    P_SetTarget(&th->target, source);
    th->angle = an;

    // [kex] adjust velocity based on viewpitch
    if(!linetarget) {
        frac = FixedMul(th->info->speed, dcos(source->pitch));
    }
    else {
        frac = th->info->speed;
    }

    th->momx = FixedMul(frac, dcos(an));
    th->momy = FixedMul(frac, dsin(an));
    th->momz = FixedMul(th->info->speed, slope);

    x = source->x + (offset * F2INT(dcos(an)));
    y = source->y + (offset * F2INT(dsin(an)));

    if(!P_TryMove(th, x, y)) {
        P_ExplodeMissile(th);
    }
}

//
// P_SpawnMissile
//

mobj_t* P_SpawnMissile(mobj_t* source, mobj_t* dest, mobjtype_t type,
                       fixed_t xoffs, fixed_t yoffs, fixed_t heightoffs, dboolean aim) {
    mobj_t* th;
    angle_t an;
    int dist;
    int speed;
    fixed_t x;
    fixed_t y;
    fixed_t z;

    x = source->x + xoffs;
    y = source->y + yoffs;
    z = source->z + (heightoffs*FRACUNIT);

    th = P_SpawnMobj(x, y, z, type);

    if(th->info->seesound) {
        S_StartSound(th, th->info->seesound);
    }

    P_SetTarget(&th->target, source);        // where it came from
    if(aim && dest) {
        an = R_PointToAngle2(x, y, dest->x, dest->y);
    }
    else {
        an = source->angle;
    }

    // fuzzy player
    if(dest) {
        if(dest->flags & MF_SHADOW) {
            an += P_RandomShift(pr_shadow, 20);
        }
    }

    speed = th->info->speed;

    // [kex] nightmare missiles move faster
    if(source && source->flags & MF_NIGHTMARE) {
        th->flags |= MF_NIGHTMARE;
        speed *= 2;
    }

    th->angle = an;
    th->momx = FixedMul(speed, dcos(th->angle));
    th->momy = FixedMul(speed, dsin(th->angle));

    if(dest) {
        dist = P_AproxDistance(dest->x - x, dest->y - y);
        dist = dist / speed;

        if(dist < 1) {
            dist = 1;
        }

        if(th->type != MT_PROJ_RECT) {
            th->momz = ((dest->z+(dest->height/2)) - z) / dist;
        }

        if(!P_TryMove(th, th->x, th->y)) {
            P_ExplodeMissile(th);
        }
    }

    return th;
}

//
// P_SpawnDartMissile
//

void P_SpawnDartMissile(int tid, int type, mobj_t *target) {
    mobj_t* mo;
    mobj_t* th;

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        // not a dart projector
        if(mo->type != MT_DEST_PROJECTILE) {
            continue;
        }

        // not matching the tid
        if(mo->tid != tid) {
            continue;
        }

        if(type == MT_PROJ_TRACER) {
            th = P_SpawnMissile(mo, target, type,
                                FixedMul(mo->radius, dcos(mo->angle)),
                                FixedMul(mo->radius, dsin(mo->angle)),
                                (mo->height<<1)/FRACUNIT, true);

            th->x = (th->x + th->momx);
            th->y = (th->y + th->momy);
            P_SetTarget(&th->tracer, target);
        }
        else {
            th = P_SpawnMissile(mo, NULL, type,
                                FixedMul(mo->radius, dcos(mo->angle)),
                                FixedMul(mo->radius, dsin(mo->angle)),
                                0, false);
        }

        P_SetTarget(&th->target, mo);        // where it came from
    }
}
