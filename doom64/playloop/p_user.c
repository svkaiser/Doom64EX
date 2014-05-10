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
//      Player related stuff.
//      Bobbing POV/weapon, movement.
//      Pending weapon.
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "d_event.h"
#include "m_fixed.h"
#include "p_local.h"
#include "doomstat.h"
#include "m_random.h"
#include "tables.h"
#include "info.h"
#include "d_englsh.h"
#include "s_sound.h"
#include "sounds.h"
#include "r_local.h"
#include "st_stuff.h"

//
// Movement.
//

// 16 pixels of bob
#define MAXBOB          0x100000
#define MAXLOOKPITCH    0x3effffff
#define MAXMOCKTIME     1800
#define MAXJUMP         (8*FRACUNIT)

int deathmocktics = 0;
#define MAXMOCKTEXT     13

static char* mockstrings[MAXMOCKTEXT] = {
    MOCKPLAYER1,
    MOCKPLAYER2,
    MOCKPLAYER3,
    MOCKPLAYER4,
    MOCKPLAYER5,
    MOCKPLAYER6,
    MOCKPLAYER7,
    MOCKPLAYER8,
    MOCKPLAYER9,
    MOCKPLAYER10,
    MOCKPLAYER11,
    MOCKPLAYER12,
    MOCKPLAYER13
};


//
// P_Thrust
// Moves the given origin along a given angle.
//

void P_Thrust(player_t* player, angle_t angle, fixed_t move) {
    angle >>= ANGLETOFINESHIFT;
    player->mo->momx += FixedMul(move * 2, finecosine[angle]);
    player->mo->momy += FixedMul(move * 2, finesine[angle]);
}

//
// P_SpectateMove
//

void P_SpectateMove(player_t* player, angle_t angle, angle_t pitch, fixed_t move) {
    fixed_t frac;

    move <<= 4;

    player->mo->momx = player->mo->momy = player->mo->momz = 0;

    P_UnsetThingPosition(player->mo);

    player->mo->ceilingz = player->mo->subsector->sector->ceilingheight;
    player->mo->floorz = player->mo->subsector->sector->floorheight;

    frac = FixedMul(move, dcos(pitch));

    player->mo->x += FixedMul(frac, dcos(angle));
    player->mo->y += FixedMul(frac, dsin(angle));
    player->mo->z += FixedMul(move, dcos(D_abs(pitch - ANG90)));

    P_SetThingPosition(player->mo);
}

static mobj_t* camstatic = NULL;

//
// P_SetStaticCamera
//

void P_SetStaticCamera(player_t* player) {
    mobj_t* mo = player->mo;

    if(player->cheats & CF_LOCKCAM) {
        return;
    }

    if(!(player->cheats & CF_FLOATCAM)) {
        camstatic = P_SpawnMobj(mo->x, mo->y, player->viewz, MT_CAMERA);
        camstatic->angle = mo->angle;
        camstatic->pitch = mo->pitch;
        camstatic->player = player;

        player->cameratarget = camstatic;
        player->message = "camera detached";
        player->cheats |= CF_FLOATCAM;
    }
    else {
        camstatic = player->cameratarget;   // just to be safe
        player->cameratarget = mo;

        P_RemoveMobj(camstatic);
        camstatic = NULL;

        player->cheats &= ~(CF_CHASECAM|CF_FLOATCAM);
        player->message = "camera attached";
    }
}

//
// P_UpdateFollowCamera
//

void P_UpdateFollowCamera(player_t* player) {
    mobj_t* mo = player->mo;
    fixed_t dist;
    fixed_t height;
    fixed_t x;
    fixed_t y;

    if(player->cameratarget != camstatic) {
        return;
    }

    height = (mo->z + mo->height) + INT2F(16);
    x = mo->x + FixedMul(mo->radius + INT2F(96), dcos(mo->angle + ANG180));
    y = mo->y + FixedMul(mo->radius + INT2F(96), dsin(mo->angle + ANG180));

    camstatic->z = height;
    camstatic->angle = mo->angle;

    P_CheckChaseCamPosition(mo, camstatic, x, y);

    dist = P_AproxDistance(camstatic->x - mo->x, camstatic->y - mo->y);
    camstatic->pitch = R_PointToPitch(camstatic->z, height - 8*FRACUNIT, dist);
}

//
// P_SetFollowCamera
//

void P_SetFollowCamera(player_t* player) {
    if(player->cheats & CF_LOCKCAM) {
        return;
    }

    P_SetStaticCamera(player);

    if(player->cameratarget == camstatic) {
        player->cheats |= CF_CHASECAM;
        player->message = "chasecam on";
    }
    else {
        player->cheats &= ~CF_CHASECAM;
        player->message = "chasecam off";
        return;
    }

    P_UpdateFollowCamera(player);
}

//
// P_ClearUserCamera
//

void P_ClearUserCamera(player_t* player) {
    if(player->cheats & CF_FLOATCAM) {
        P_SetStaticCamera(player);   // clear float cam
    }
    else if(player->cheats & CF_CHASECAM) {
        P_SetFollowCamera(player);   // clear chasecam
    }
}


//
// P_CalcHeight
// Calculate the walking / running height adjustment
//

void P_CalcHeight(player_t* player) {
    int         angle;
    fixed_t     bob;

    // Regular movement bobbing
    // (needs to be calculated for gun swing
    // even if not on ground)
    // OPTIMIZE: tablify angle
    // Note: a LUT allows for effects
    //  like a ramp with low health.
    player->bob =
        FixedMul(player->mo->momx, player->mo->momx)
        + FixedMul(player->mo->momy, player->mo->momy);

    player->bob >>= 2;

    if(player->bob > MAXBOB) {
        player->bob = MAXBOB;
    }

    if(!player->onground) {
        player->viewz = player->mo->z + VIEWHEIGHT;

        if(player->viewz > player->mo->ceilingz-4*FRACUNIT) {
            player->viewz = player->mo->ceilingz-4*FRACUNIT;
        }

        player->viewz = player->mo->z + player->viewheight;
        return;
    }

    angle = (FINEANGLES / 20 * leveltime) & FINEMASK;
    bob = FixedMul(player->bob / 2, finesine[angle]);


    // move viewheight
    if(player->playerstate == PST_LIVE) {
        player->viewheight += player->deltaviewheight;

        if(player->viewheight > VIEWHEIGHT) {
            player->viewheight = VIEWHEIGHT;
            player->deltaviewheight = 0;
        }

        if(player->viewheight < VIEWHEIGHT/2) {
            player->viewheight = VIEWHEIGHT/2;
            if(player->deltaviewheight <= 0) {
                player->deltaviewheight = 1;
            }
        }

        if(player->deltaviewheight) {
            player->deltaviewheight += FRACUNIT/2;
            if(!player->deltaviewheight) {
                player->deltaviewheight = 1;
            }
        }
    }

    player->viewz = player->mo->z + player->viewheight + bob;

    if(player->viewz
            > player->mo->subsector->sector->ceilingheight-4*FRACUNIT) {
        player->viewz = player->mo->subsector->sector->ceilingheight-4*FRACUNIT;
    }
}

//
// P_MovePlayer
//

void P_MovePlayer(player_t* player) {
    ticcmd_t*   cmd;
    int         mpitch;

    cmd = &player->cmd;

    player->mo->angle += INT2F(cmd->angleturn);

    if(cmd->buttons2 & BT2_CENTER) {
        player->mo->pitch = 0;
    }
    else {
        player->mo->pitch += INT2F(cmd->pitch);
        mpitch = player->mo->pitch;
        if(mpitch >= MAXLOOKPITCH) {
            player->mo->pitch = MAXLOOKPITCH;
        }
        if(mpitch <= -MAXLOOKPITCH) {
            player->mo->pitch = -(MAXLOOKPITCH);
        }
    }

    // Do not let the player control movement if not onground.
    if(cmd->forwardmove) {
        if(player->cheats & CF_SPECTATOR) {
            P_SpectateMove(player, player->mo->angle, player->mo->pitch, cmd->forwardmove*2048);
        }
        else if(player->onground) {
            P_Thrust(player, player->mo->angle, cmd->forwardmove*2048);
        }
    }

    if(cmd->sidemove) {
        if(player->cheats & CF_SPECTATOR) {
            P_SpectateMove(player, player->mo->angle-ANG90, 0, cmd->sidemove*2048);
        }
        else if(player->onground) {
            P_Thrust(player, player->mo->angle-ANG90, cmd->sidemove*2048);
        }
    }

    if((cmd->forwardmove || cmd->sidemove) && player->mo->state == &states[S_001]) {
        P_SetMobjState(player->mo, S_002);
    }

    if(cmd->buttons2 & BT2_JUMP) {
        if(player->onground && !player->jumpdown) {
            player->mo->momz += MAXJUMP;
            player->jumpdown = true;
        }
    }
    else {
        cmd->buttons2 &= ~BT2_JUMP;
        player->jumpdown = false;
    }
}

//
// P_AdjustAngle
//

angle_t P_AdjustAngle(angle_t angle, angle_t newangle, int threshold) {
    angle_t delta;

    delta = newangle - angle;

    if(delta < (angle_t)threshold || delta > (angle_t)-threshold) {
        return newangle;
    }
    else {
        if(delta < ANG180) {
            return angle + threshold;
        }
        else {
            return angle - threshold;
        }
    }

    // no change
    return angle;
}

//
// P_DeathThink
// Fall on your face when dying.
// Decrease POV height to floor height.
//

void P_DeathThink(player_t* player) {
    angle_t angle;

    P_MovePsprites(player);

    // fall to the ground
    if(player->viewheight > 8*FRACUNIT) {
        player->viewheight -= FRACUNIT;
    }

    if(player->viewheight < 8*FRACUNIT) {
        player->viewheight = 8*FRACUNIT;
    }

    player->deltaviewheight = 0;
    player->onground = ((player->mo->floorz < player->mo->z &&
                         !(player->mo->blockflag & BF_MOBJSTAND)) ^ 1);

    P_CalcHeight(player);

    if(player->attacker && player->attacker != player->mo) {
        angle = R_PointToAngle2(player->mo->x,
                                player->mo->y,
                                player->attacker->x,
                                player->attacker->y);

        // [d64] don't decrement damagecount

        // [kex] use angle adjust routine
        player->mo->angle = P_AdjustAngle(player->mo->angle, angle, ANG5);

        if(player->mo->angle == angle) {
            if(player->damagecount) {
                player->damagecount--;
            }
        }
    }
    else if(player->damagecount) {
        player->damagecount--;
    }

    //mocking text
    if(!((gametic - deathmocktics) < MAXMOCKTIME)) {
        player->message = mockstrings[(P_Random(pr_playermock) % MAXMOCKTEXT)];
        deathmocktics = gametic;
    }

    if(player->cmd.buttons & BT_USE) {
        player->playerstate = PST_REBORN;
    }
}

//
// P_PlayerXYMovment
//

void P_PlayerXYMovment(mobj_t* mo) {
    fixed_t x = mo->momx + mo->x;
    fixed_t y = mo->momy + mo->y;

    if(mo->player->cheats & CF_SPECTATOR) {
        return;
    }

    // try to slide along a blocked move
    if(!P_PlayerMove(mo, x, y)) {
        P_SlideMove(mo);
    }

    if(mo->z > mo->floorz && (!(mo->blockflag & BF_MOBJSTAND))) {
        return;    // no friction when airborne
    }

    if(mo->flags & MF_CORPSE) {
        if(mo->floorz != mo->subsector->sector->floorheight) {
            return;
        }
    }

    if(mo->momx > -STOPSPEED && mo->momx < STOPSPEED &&
            mo->momy > -STOPSPEED && mo->momy < STOPSPEED) {
        // if in a walking frame, stop moving
        if(((unsigned)(mo->state - states)- S_002) < 4) {
            P_SetMobjState(mo, S_001);
        }

        mo->momx = 0;
        mo->momy = 0;
    }
    else {
        mo->momx = FixedMul(mo->momx, FRICTION);
        mo->momy = FixedMul(mo->momy, FRICTION);
    }
}

//
// P_PlayerZMovement
//

void P_PlayerZMovement(mobj_t* mo) {
    if(mo->player->cheats & CF_SPECTATOR) {
        return;
    }

    // check for smooth step up
    if(mo->z < mo->floorz) {
        mo->player->viewheight -= mo->floorz - mo->z;
        mo->player->deltaviewheight = (VIEWHEIGHT - mo->player->viewheight)>>2;
    }

    // adjust height
    mo->z += mo->momz;

    // clip movement
    if(mo->z <= mo->floorz) {
        if(mo->momz < 0) {
            if(mo->momz < -GRAVITY*8) {
                // Squat down.
                // Decrease viewheight for a moment
                // after hitting the ground (hard),
                // and utter appropriate sound.
                mo->player->deltaviewheight = mo->momz>>3;
                S_StartSound(mo, sfx_oof);
            }
            mo->momz = 0;
        }
        mo->z = mo->floorz;
    }
    else {
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
    }
}

//
// P_PlayerTic
//

void P_PlayerTic(mobj_t* mo) {
    state_t* st;

    if(!mo->player) {
        return;
    }

    blockthing = NULL;

    if(mo->player->cheats & CF_CHASECAM) {
        P_UpdateFollowCamera(mo->player);
    }

    if(mo->momx || mo->momy) {
        P_PlayerXYMovment(mo);
    }

    mo->player->onground = ((mo->floorz < mo->z &&
        !(mo->blockflag & BF_MOBJSTAND)) ^ 1);

    if((mo->floorz != mo->z) || mo->momz || blockthing) {
        if(!P_OnMobjZ(mo)) {
            P_PlayerZMovement(mo);
        }
    }

    if(mo->tics == -1) {
        return;
    }

    mo->tics--;

    if(!mo->tics) {
        st = &states[mo->state->nextstate];
        mo->state = st;
        mo->tics = st->tics;
        mo->frame = st->frame;
        mo->sprite = st->sprite;

        if(st->action.acp1) {
            st->action.acp1(mo);
        }
    }
}

//
// P_PlayerThink
//

void P_PlayerThink(player_t* player) {
    ticcmd_t*       cmd;
    weapontype_t    newweapon;

    P_PlayerTic(player->mo);

    // fixme: do this in the cheat code
    if(player->cheats & CF_NOCLIP) {
        player->mo->flags |= MF_NOCLIP;
    }
    else {
        player->mo->flags &= ~MF_NOCLIP;
    }

    // chain saw run forward
    cmd = &player->cmd;
    if(player->mo->flags & MF_JUSTATTACKED) {
        cmd->angleturn = 0;
        cmd->forwardmove = 0xc800/512;
        cmd->sidemove = 0;
        player->mo->flags &= ~MF_JUSTATTACKED;
    }

    // [kex] reset camera target if it doesn't exist
    if(!player->cameratarget) {
        player->cameratarget = player->mo;
    }

    if(player->playerstate == PST_DEAD && !(player->cheats & CF_UNDYING)) {
        P_DeathThink(player);
        return;
    }

    // Move around.
    // Reactiontime is used to prevent movement
    // for a bit after a teleport.
    if(player->mo->reactiontime) {
        player->mo->reactiontime--;
    }
    else {
        P_MovePlayer(player);
    }

    P_CalcHeight(player);

    if(player->mo->subsector->sector->flags &
            (MS_SECRET |
             MS_DAMAGEX5 |
             MS_DAMAGEX10 |
             MS_DAMAGEX20 |
             MS_SCROLLFLOOR)) {
        P_PlayerInSpecialSector(player);
    }

    // Check for weapon change.

    // A special event has no other buttons.
    if(cmd->buttons & BT_SPECIAL) {
        cmd->buttons = 0;
    }

    if(cmd->buttons & BT_CHANGE) {
        if(!(cmd->buttons2 & (BT2_NEXTWEAP|BT2_PREVWEAP))) {
            newweapon = (cmd->buttons & BT_WEAPONMASK) >> BT_WEAPONSHIFT;

            if(newweapon == wp_fist
                    && player->weaponowned[wp_chainsaw]
                    && (player->readyweapon != wp_chainsaw)) {
                newweapon = wp_chainsaw;
            }

            if(newweapon == wp_shotgun
                    && player->weaponowned[wp_supershotgun]
                    && player->readyweapon != wp_supershotgun) {
                newweapon = wp_supershotgun;
            }

            if(player->weaponowned[newweapon] && newweapon != player->readyweapon) {
                player->pendingweapon = newweapon;
            }
        }
        else {  // 20120211 villsa - new weapon cycle logic
            dboolean direction;
            int weapon;

            newweapon = player->pendingweapon;

            if(newweapon == wp_nochange) {
                newweapon = player->readyweapon;
            }

            direction = (cmd->buttons2 & BT2_NEXTWEAP) ? true : false;
            weapon = newweapon;

            while(1) {
                weapon += direction ? 1 : -1;

                if(weapon < wp_chainsaw) {
                    weapon = (NUMWEAPONS - 1);
                }

                if(weapon >= NUMWEAPONS) {
                    weapon = wp_chainsaw;
                }

                if(player->weaponowned[weapon]) {
                    newweapon = weapon;
                    break;
                }
            }

            player->pendingweapon = newweapon;

            ST_DisplayPendingWeapon();
        }
    }

    // check for use
    if(cmd->buttons & BT_USE) {
        if(!player->usedown) {
            P_UseLines(player, false);
            player->usedown = true;
        }
    }
    else {
        player->usedown = false;
    }

    // cycle psprites
    P_MovePsprites(player);

    // Counters, time dependend power ups.

    if(player->powers[pw_strength] >= 2) {
        player->powers[pw_strength]--;
    }

    if(player->powers[pw_invulnerability]) {
        player->powers[pw_invulnerability]--;
    }

    if(player->powers[pw_invisibility]) {
        if(player->powers[pw_invisibility] > 61 || player->powers[pw_invisibility] & 8) {
            player->mo->alpha = 0x7f;
        }
        else {
            player->mo->alpha = 0xff;
        }

        if(! --player->powers[pw_invisibility]) {
            player->mo->flags &= ~MF_SHADOW;
        }
    }

    if(player->powers[pw_infrared]) {
        player->powers[pw_infrared]--;
    }
    else {
        if(&players[displayplayer] == player) {
            if(infraredFactor) {
                infraredFactor -= 4;
                R_RefreshBrightness();
            }
        }
    }

    if(player->powers[pw_ironfeet]) {
        player->powers[pw_ironfeet]--;
    }

    if(player->damagecount) {
        player->damagecount--;
    }

    if(player->bonuscount) {
        player->bonuscount--;
    }

    if(player->bfgcount) {
        player->bfgcount -= 6;
        if(player->bfgcount < 0) {
            player->bfgcount = 0;
        }
    }

    // [d64] - recoil pitch from weapons
    if(player->recoilpitch >> ANGLETOFINESHIFT) {
        player->recoilpitch -= (player->recoilpitch >> 2);
    }
    else {
        player->recoilpitch = 0;
    }

    // [kex] check cvar for autoaim
    player->autoaim = (gameflags & GF_ALLOWAUTOAIM);
}


