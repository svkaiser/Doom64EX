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
//        Weapon sprite animation, weapon objects.
//        Action functions for weapons.
//
//-----------------------------------------------------------------------------

#include <math.h>

#include "doomdef.h"
#include "d_event.h"
#include "m_fixed.h"
#include "m_random.h"
#include "p_local.h"
#include "s_sound.h"
#include "z_zone.h"
#include "r_local.h"
#include "doomstat.h"
#include "sounds.h"
#include "p_pspr.h"
#include "m_misc.h"


#define LOWERSPEED                FRACUNIT*7
#define RAISESPEED                FRACUNIT*7

#define WEAPONBOTTOM            128*FRACUNIT
#define WEAPONTOP                32*FRACUNIT

#define RECOILPITCH                0x2AA8000    //villsa


// plasma cells for a bfg attack
#define BFGCELLS                40

weaponinfo_t    weaponinfo[NUMWEAPONS] = {
    { am_noammo,    S_708, S_707, S_705, S_709, S_000 },    // chainsaw
    { am_noammo,    S_714, S_713, S_712, S_715, S_000 },    // fist
    { am_clip,        S_722, S_721, S_720, S_723, S_728 },    // pistol
    { am_shell,        S_731, S_730, S_729, S_732, S_738 },    // shotgun
    { am_shell,        S_741, S_740, S_739, S_742, S_752 },    // super shotgun
    { am_clip,        S_755, S_754, S_753, S_756, S_759 },    // chaingun
    { am_misl,        S_763, S_762, S_761, S_764, S_767 },    // rocket launcher
    { am_cell,        S_773, S_772, S_771, S_775, S_000 },    // plasma gun
    { am_cell,        S_783, S_782, S_781, S_784, S_788 },    // bfg
    { am_cell,        S_793, S_792, S_791, S_794, S_796 }        // laser rifle
};

static int laserCells = 1;

void A_FadeAlpha(mobj_t *mobj);


//
// P_SetPsprite
//
static void P_SetPsprite(player_t *player, int position, statenum_t stnum) {
    pspdef_t *psp = &player->psprites[position];

    do {
        state_t *state;

        if(!stnum) {
            // object removed itself
            psp->state = NULL;
            break;
        }

        state = &states[stnum];
        psp->state = state;
        psp->tics = state->tics;        // could be 0

        // Call action routine.
        // Modified handling.
        if(state->action.acp2) {
            state->action.acp2(player, psp);
            if(!psp->state) {
                break;
            }
        }
        stnum = psp->state->nextstate;
    }
    while(!psp->tics);     // an initial state of 0 could cycle through
}


//
// P_BringUpWeapon
// Starts bringing the pending weapon up
// from the bottom of the screen.
// Uses player
//

static dboolean pls_buzzing = false;    // [kex] for keeping track when the buzzing is playing

void P_BringUpWeapon(player_t* player) {
    statenum_t    newstate;

    if(player->pendingweapon == wp_nochange) {
        player->pendingweapon = player->readyweapon;
    }

    if(player->pendingweapon == wp_chainsaw) {
        S_StartSound(player->mo, sfx_sawup);
    }
    else if(player->pendingweapon == wp_plasma) {
        pls_buzzing = true;
        S_StartSound(player->mo, sfx_electric);
    }

    //
    // [kex] hack for making sure the buzzing sound is stopped
    // when plasma rifle is not equipped
    //
    if(pls_buzzing && player->pendingweapon != wp_plasma) {
        pls_buzzing = false;
        S_StopSound(NULL, sfx_electric);
    }

    newstate = weaponinfo[player->pendingweapon].upstate;

    player->pendingweapon = wp_nochange;
    player->psprites[ps_weapon].sx = FRACUNIT;
    player->psprites[ps_weapon].sy = WEAPONBOTTOM;

    player->psprites[ps_weapon].alpha = 0xff;
    player->psprites[ps_flash].alpha = 0xff;

    P_SetPsprite(player, ps_weapon, newstate);
}

//
// P_DropWeapon
// Player died, so put the weapon away.
//
void P_DropWeapon(player_t* player) {
    P_SetPsprite(player,
                 ps_weapon,
                 weaponinfo[player->readyweapon].downstate);
}

//
// P_CheckAmmo
// Returns true if there is enough ammo to shoot.
// If not, selects the next weapon to use.
//

dboolean P_CheckAmmo(player_t* player) {
    ammotype_t ammo;
    int count;

    ammo = weaponinfo[player->readyweapon].ammo;

    // Minimal amount for one shot varies.
    if(player->readyweapon == wp_bfg) {
        count = BFGCELLS;
    }
    else if(player->readyweapon == wp_supershotgun) {
        count = 2;    // Double barrel.
    }
    else if(player->readyweapon == wp_laser) {
        count = laserCells;
    }
    else {
        count = 1;    // Regular.
    }

    // Some do not need ammunition anyway.
    // Return if current ammunition sufficient.
    if(ammo == am_noammo || player->ammo[ammo] >= count) {
        return true;
    }

    // Out of ammo, pick a weapon to change to.
    // Preferences are set here.
    do {
        if(player->weaponowned[wp_plasma]
                && player->ammo[am_cell]) {
            player->pendingweapon = wp_plasma;
        }
        else if(player->weaponowned[wp_supershotgun]
                && player->ammo[am_shell] > 2) {
            player->pendingweapon = wp_supershotgun;
        }
        else if(player->weaponowned[wp_chaingun]
                && player->ammo[am_clip]) {
            player->pendingweapon = wp_chaingun;
        }
        else if(player->weaponowned[wp_shotgun]
                && player->ammo[am_shell]) {
            player->pendingweapon = wp_shotgun;
        }
        else if(player->ammo[am_clip]) {
            player->pendingweapon = wp_pistol;
        }
        else if(player->weaponowned[wp_chainsaw]) {
            player->pendingweapon = wp_chainsaw;
        }
        else if(player->weaponowned[wp_missile]
                && player->ammo[am_misl]) {
            player->pendingweapon = wp_missile;
        }
        else if(player->weaponowned[wp_bfg]
                && player->ammo[am_cell] > 40) {
            player->pendingweapon = wp_bfg;
        }
        else if(player->weaponowned[wp_laser]
                && player->ammo[am_cell] > laserCells) {
            player->pendingweapon = wp_laser;
        }
        else {
            // If everything fails.
            player->pendingweapon = wp_fist;
        }

    }
    while(player->pendingweapon == wp_nochange);

    // Now set appropriate weapon overlay.
    P_SetPsprite(player, ps_weapon, weaponinfo[player->readyweapon].downstate);

    return false;
}


//
// P_FireWeapon.
//
void P_FireWeapon(player_t* player) {
    statenum_t    newstate;
    pspdef_t*    psp;

    if(!P_CheckAmmo(player)) {
        return;
    }

    psp = &player->psprites[ps_weapon];

    P_SetMobjState(player->mo, S_006);
    newstate = weaponinfo[player->readyweapon].atkstate;
    if(player->refire && player->readyweapon == wp_pistol) {
        newstate++;
    }
    P_SetPsprite(player, ps_weapon, newstate);
    P_NoiseAlert(player->mo, player->mo);
    psp->sx = FRACUNIT;        //villsa
    psp->sy = WEAPONTOP;
}

//
// A_WeaponReady
// The player can fire the weapon
// or change to another weapon at this time.
// Follows after getting weapon up,
// or after previous attack/fire sequence.
//

void A_WeaponReady(player_t* player, pspdef_t* psp) {
    statenum_t    newstate;
    int         angle;

    // get out of attack state
    if(player->mo->state == &states[S_006]
            || player->mo->state == &states[S_007]) {
        P_SetMobjState(player->mo, S_001);
    }

    // check for change
    //    if player is dead, put the weapon away
    if(player->pendingweapon != wp_nochange || !player->health) {
        // change weapon
        //    (pending weapon should allready be validated)
        newstate = weaponinfo[player->readyweapon].downstate;
        P_SetPsprite(player, ps_weapon, newstate);
        return;
    }

    // check for fire
    //    the missile launcher and bfg do not auto fire
    if(player->cmd.buttons & BT_ATTACK) {
        if(!player->attackdown
                || (player->readyweapon != wp_missile
                    && player->readyweapon != wp_bfg)) {
            player->attackdown = true;
            P_FireWeapon(player);
            return;
        }
    }
    else {
        player->attackdown = false;
    }

    // bob the weapon based on movement speed
    angle = (128*leveltime)&FINEMASK;
    psp->sx = FRACUNIT + FixedMul(player->bob, finecosine[angle]);
    angle &= FINEANGLES/2-1;
    psp->sy = WEAPONTOP + FixedMul(player->bob, finesine[angle]);
}


//
// A_ReFire
// The player can re-fire the weapon
// without lowering it entirely.
//
void A_ReFire(player_t* player, pspdef_t* psp) {

    // check for fire
    //    (if a weaponchange is pending, let it go through instead)
    if((player->cmd.buttons & BT_ATTACK)
            && player->pendingweapon == wp_nochange
            && player->health) {
        player->refire++;
        P_FireWeapon(player);
    }
    else {
        player->refire = 0;
        P_CheckAmmo(player);
    }
}

//
// A_CheckReload
//

void A_CheckReload(player_t* player, pspdef_t* psp) {
    P_CheckAmmo(player);
}



//
// A_Lower
// Lowers current weapon,
//    and changes weapon at bottom.
//
void A_Lower(player_t* player, pspdef_t* psp) {
    psp->sy += LOWERSPEED;

    // Is already down.
    if(psp->sy < WEAPONBOTTOM) {
        return;
    }

    // Player is dead.
    if(player->playerstate == PST_DEAD) {
        psp->sy = WEAPONBOTTOM;

        // don't bring weapon back up
        return;
    }

    //
    // [d64] stop plasma buzz
    //
    if(player->readyweapon == wp_plasma) {
        pls_buzzing = false;
        S_StopSound(NULL, sfx_electric);
    }

    // The old weapon has been lowered off the screen,
    // so change the weapon and start raising it
    if(!player->health) {
        // Player is dead, so keep the weapon off screen.
        P_SetPsprite(player,  ps_weapon, S_000);
        return;
    }

    P_SetPsprite(player, ps_flash, S_000);  //villsa

    player->readyweapon = player->pendingweapon;
    P_BringUpWeapon(player);
}


//
// A_Raise
//
void A_Raise(player_t* player, pspdef_t* psp) {
    statenum_t    newstate;

    psp->sy -= RAISESPEED;
    if(psp->sy > WEAPONTOP) {
        return;
    }

    psp->sy = WEAPONTOP;

    // The weapon has been raised all the way,
    //    so change to the ready state.
    newstate = weaponinfo[player->readyweapon].readystate;
    P_SetPsprite(player, ps_weapon, newstate);
}



//
// A_GunFlash
//
void A_GunFlash(player_t* player, pspdef_t* psp) {
    P_SetMobjState(player->mo, S_007);

    // [d64] set alpha on flash frame
    if(player->readyweapon != wp_bfg) {
        player->psprites[ps_flash].alpha = 100;
    }

    P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate);
}



//
// WEAPON ATTACKS
//


//
// A_Punch
//

void A_Punch(player_t* player, pspdef_t* psp) {
    angle_t     angle;
    int         damage;
    int         slope = 0;

    damage = ((P_Random(pr_punch) & 7) + 1) * 3;

    if(player->powers[pw_strength]) {
        damage *= 10;
    }

    angle = player->mo->angle;
    angle += P_RandomShift(pr_punchangle, 18);

    slope = P_AimLineAttack(player->mo, angle, 0, MELEERANGE);

    P_LineAttack(player->mo, angle, MELEERANGE, slope, damage);

    // turn to face target
    if(linetarget) {
        S_StartSound(player->mo, sfx_punch);
        player->mo->angle = R_PointToAngle2(player->mo->x, player->mo->y,
                                            linetarget->x, linetarget->y);
    }
}


//
// A_Saw
//

void A_Saw(player_t* player, pspdef_t* psp) {
    angle_t     angle;
    int         damage;
    int         slope = 0;

    damage = ((P_Random(pr_saw) & 7) + 1) * 3;
    angle = player->mo->angle;
    angle += P_RandomShift(pr_saw, 18);

    // use meleerange + 1 se the puff doesn't skip the flash
    slope = P_AimLineAttack(player->mo, angle, 0, MELEERANGE+1);

    P_LineAttack(player->mo, angle, MELEERANGE + 1, slope, damage);

    if(!linetarget) {
        S_StartSound(player->mo, sfx_saw1);
        return;
    }

    S_StartSound(player->mo, sfx_saw2);

    // turn to face target
    angle = R_PointToAngle2(player->mo->x, player->mo->y, linetarget->x, linetarget->y);
    if(angle - player->mo->angle > ANG180) {
        if(angle - player->mo->angle < -ANG90/20) {
            player->mo->angle = angle + ANG90/21;
        }
        else {
            player->mo->angle -= ANG90/20;
        }
    }
    else {
        if(angle - player->mo->angle > ANG90/20) {
            player->mo->angle = angle - ANG90/21;
        }
        else {
            player->mo->angle += ANG90/20;
        }
    }

    player->mo->flags |= MF_JUSTATTACKED;
}


//
// A_ChainSawReady
//

void A_ChainSawReady(player_t* player, pspdef_t* psp) {
    S_StartSound(player->mo, sfx_sawidle);
    A_WeaponReady(player, psp);
}


//
// A_FireMissile
//

void A_FireMissile(player_t* player, pspdef_t* psp) {
    player->ammo[weaponinfo[player->readyweapon].ammo]--;
    P_SpawnPlayerMissile(player->mo, MT_PROJ_ROCKET);

    player->recoilpitch = RECOILPITCH;

    if(player->onground) {
        P_Thrust(player, player->mo->angle + ANG180, FRACUNIT);
    }
}


//
// A_FireBFG
//
void A_FireBFG(player_t* player, pspdef_t* psp) {
    player->ammo[weaponinfo[player->readyweapon].ammo] -= BFGCELLS;
    P_SpawnPlayerMissile(player->mo, MT_PROJ_BFG);
}


//
// A_PlasmaAnimate
//

static int pls_animpic = 0;

void A_PlasmaAnimate(player_t *player, pspdef_t *psp) {
    P_SetPsprite(player, ps_flash, S_778 + pls_animpic);

    if(++pls_animpic >= 3) {
        pls_animpic = 0;
    }
}


//
// A_FirePlasma
//
void A_FirePlasma(player_t* player, pspdef_t* psp) {
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite(player, ps_flash, S_000);
    P_SpawnPlayerMissile(player->mo, MT_PROJ_PLASMA);
}



//
// P_BulletSlope
//
// Sets a slope so a near miss is at aproximately
// the height of the intended target
//

fixed_t bulletslope;

void P_BulletSlope(mobj_t* mo) {
    angle_t an;

    // see which target is to be aimed at
    an = mo->angle;
    bulletslope = P_AimLineAttack(mo, an, 0, ATTACKRANGE);

    if(!linetarget) {
        an += 1<<26;
        bulletslope = P_AimLineAttack(mo, an, 0, ATTACKRANGE);

        if(!linetarget) {
            an -= 2<<26;
            bulletslope = P_AimLineAttack(mo, an, 0, ATTACKRANGE);
        }
    }
}


//
// P_GunShot
//
void P_GunShot(mobj_t* mo, dboolean accurate) {
    angle_t     angle;
    int         damage;

    damage = ((P_Random(pr_gunshot)&3)<<2)+4;
    angle = mo->angle;

    if(!accurate) {
        angle += P_RandomShift(pr_misfire, 18);
    }

    P_LineAttack(mo, angle, MISSILERANGE, bulletslope, damage);
}


//
// A_FirePistol
//
void A_FirePistol(player_t* player, pspdef_t* psp) {
    S_StartSound(player->mo, sfx_pistol);

    P_SetMobjState(player->mo, S_007);
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite(player,
                 ps_flash,
                 weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope(player->mo);
    P_GunShot(player->mo, !player->refire);
}


//
// A_FireShotgun
//
void A_FireShotgun(player_t* player, pspdef_t* psp) {
    int i;

    S_StartSound(player->mo, sfx_shotgun);
    P_SetMobjState(player->mo, S_007);

    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    P_SetPsprite(player,
                 ps_flash,
                 weaponinfo[player->readyweapon].flashstate);

    P_BulletSlope(player->mo);

    player->recoilpitch = RECOILPITCH;

    for(i = 0; i < 7; i++) {
        P_GunShot(player->mo, false);
    }
}



//
// A_FireShotgun2
//

void A_FireShotgun2(player_t* player, pspdef_t* psp) {
    int         i;
    angle_t     angle;
    int         damage;

    S_StartSound(player->mo, sfx_sht2fire);
    P_SetMobjState(player->mo, S_007);
    player->ammo[weaponinfo[player->readyweapon].ammo] -= 2;

    P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate);
    P_BulletSlope(player->mo);

    player->recoilpitch = RECOILPITCH;

    if(player->onground) {
        P_Thrust(player, player->mo->angle + ANG180, FRACUNIT);
    }

    for(i = 0; i < 20; i++) {
        damage = 5 * (P_Random(pr_shotgun) % 3 + 1);
        angle = player->mo->angle;
        angle += P_RandomShift(pr_shotgun, 19);
        P_LineAttack(player->mo, angle, MISSILERANGE, bulletslope +
                     P_RandomShift(pr_shotgun, 5), damage);
    }
}

//
// A_FireCGun
//

void A_FireCGun(player_t* player, pspdef_t* psp) {
    int ammo = player->ammo[weaponinfo[player->readyweapon].ammo];
    int rand;

    if(!ammo) {
        return;
    }

    S_StartSound(player->mo, sfx_pistol);

    P_SetMobjState(player->mo, S_007);
    player->ammo[weaponinfo[player->readyweapon].ammo]--;

    // randomize sx
    rand = (((P_Random(pr_chaingun) & 1) << 1) - 1);
    psp->sx = (rand * FRACUNIT);

    // randomize sy
    rand = ((((ammo - 1) & 1) << 1) - 1);
    psp->sy = WEAPONTOP - (rand * (2*FRACUNIT));

    player->psprites[ps_flash].alpha = 160;

    P_SetPsprite(player, ps_flash,
                 weaponinfo[player->readyweapon].flashstate + psp->state - &states[S_756]);

    player->recoilpitch = RECOILPITCH;

    P_BulletSlope(player->mo);
    P_GunShot(player->mo, !player->refire);
}

//
// A_BFGFlash
//

void A_BFGFlash(mobj_t* actor) {
    if(actor->target) {
        if(actor->target->player) {
            actor->target->player->bfgcount = 100;
        }
    }

    actor->alpha = 170;
}

//
// A_BFGSpray
// Spawn a BFG explosion on every monster in view
//

void A_BFGSpray(mobj_t* mo) {
    int         i;
    int         j;
    int         damage;
    angle_t     an;

    // offset angles from its attack angle
    for(i = 0; i < 40; i++) {
        an = mo->angle - ANG90/2 + ANG90/40*i;

        // mo->target is the originator (player) of the missile

        //
        // [kex] add 1 to distance so autoaim can be forced
        //
        P_AimLineAttack(mo->target, an, 0, ATTACKRANGE + 1);

        if(!linetarget) {
            continue;
        }

        //
        // [d64] shift linetarget height by 1 instead of 2
        //
        P_SpawnMobj(linetarget->x, linetarget->y,
                    linetarget->z + (linetarget->height >> 1), MT_BFGSPREAD);

        damage = 0;
        for(j = 0; j < 15; j++) {
            damage += (P_Random(pr_bfg) & 7) + 1;
        }

        P_DamageMobj(linetarget, mo->target, mo->target, damage);
    }

    A_FadeAlpha(mo);
}


//
// A_BFGsound
//

void A_BFGsound(player_t* player, pspdef_t* psp) {
    S_StartSound(player->mo, sfx_bfg);
}

//
// A_LoadShotgun2
//

void A_LoadShotgun2(player_t* player, pspdef_t* psp) {
    S_StartSound(player->mo, sfx_sht2load1);
}

//
// A_CloseShotgun2
//

void A_CloseShotgun2(player_t* player, pspdef_t* psp) {
    S_StartSound(player->mo, sfx_sht2load2);
}

//
// P_LaserPointOnSide
//

d_inline static fixed_t P_LaserPointOnSide(fixed_t x, fixed_t y, node_t* node) {
    fixed_t    dx;
    fixed_t    dy;
    fixed_t    left;
    fixed_t    right;

    dx = (x - node->x);
    dy = (y - node->y);

    left =  F2INT(node->dy) * F2INT(dx);
    right = F2INT(dy) * F2INT(node->dx);

    return left - right;
}

//
// P_LaserCrossBSP
//
// Traverses through BSP nodes and places a laser marker on each
// partition line that it hits
//

static void P_LaserCrossBSP(int bspnum, laser_t* laser) {
    node_t* node;
    int ds1;
    int ds2;
    int s1;
    int s2;
    int dist;
    int frac;
    mobj_t* marker;
    laser_t* childlaser = NULL;
    fixed_t x;
    fixed_t y;
    fixed_t z;

    while(!(bspnum & NF_SUBSECTOR)) {
        node = &nodes[bspnum];

        // traverse nodes
        ds1 = P_LaserPointOnSide(laser->x1, laser->y1, node);
        ds2 = P_LaserPointOnSide(laser->x2, laser->y2, node);

        s1 = (ds1 < 0);
        s2 = (ds2 < 0);

        // did the two laser points cross the node?
        if(s1 == s2) {
            bspnum = node->children[s1];
            continue;
        }

        // allocate a new child laser
        childlaser = (laser_t*)Z_Calloc(sizeof(laser_t), PU_LEVSPEC, NULL);
        childlaser->x1 = laser->x1;
        childlaser->y1 = laser->y1;
        childlaser->z1 = laser->z1;
        childlaser->x2 = laser->x2;
        childlaser->y2 = laser->y2;
        childlaser->z2 = laser->z2;
        childlaser->slopex = laser->slopex;
        childlaser->slopey = laser->slopey;
        childlaser->slopez = laser->slopez;
        childlaser->distmax = laser->distmax;
        childlaser->next = laser->next;
        childlaser->angle = laser->angle;

        // get the intercepting point of the laser and node
        frac = FixedDiv(ds1, ds1 - ds2);

        x = (F2INT(laser->x2 - laser->x1) * frac) + laser->x1;
        y = (F2INT(laser->y2 - laser->y1) * frac) + laser->y1;
        z = (F2INT(laser->z2 - laser->z1) * frac) + laser->z1;

        // update endpoint of current laser to intercept point
        laser->x2 = x;
        laser->y2 = y;
        laser->z2 = z;

        // childlaser begins at intercept point
        childlaser->x1 = x;
        childlaser->y1 = y;
        childlaser->z1 = z;

        // update distmax
        dist = F2INT(laser->distmax * frac);

        childlaser->distmax = laser->distmax - dist;
        laser->distmax = dist;

        // point to child laser
        laser->next = childlaser;

        // traverse child nodes
        P_LaserCrossBSP(node->children[s1], laser);

        laser = childlaser;
        bspnum = node->children[s2];
    }

    // subsector was hit, spawn a marker between the two laser points
    x = (laser->x1 + laser->x2) >> 1;
    y = (laser->y1 + laser->y2) >> 1;
    z = (laser->z1 + laser->z2) >> 1;

    marker = P_SpawnMobj(x, y, z, MT_LASERMARKER);

    // have marker point to which laser it belongs to
    marker->extradata = (laser_t*)laser;
    laser->marker = marker;
}

//
// T_LaserThinker
//

void T_LaserThinker(laserthinker_t* laserthinker) {
    laser_t* laser = laserthinker->laser;

    laser->dist += 64;

    // laser reached its destination?
    if(laser->dist >= laser->distmax) {
        // reached the end?
        if(!laser->next) {
            P_RemoveThinker(&laserthinker->thinker);

            // fade out the laser puff
            P_FadeMobj(laserthinker->dest, -24, 0, 0);
        }
        else {
            laserthinker->laser = laser->next;    // advance to next laser point
        }

        // remove marker and free laser
        P_RemoveMobj(laser->marker);
        Z_Free(laser);
    }
    else {
        // update laser's location
        laser->x1 += laser->slopex;
        laser->y1 += laser->slopey;
        laser->z1 += laser->slopez;
    }
}

//
// A_FireLaser
//

fixed_t laserhit_x;
fixed_t laserhit_y;
fixed_t laserhit_z;

void A_FireLaser(player_t *player, pspdef_t *psp) {
    angle_t         angleoffs;
    angle_t         spread;
    mobj_t          *mobj;
    int             lasercount;
    int             i;
    fixed_t         slope;
    fixed_t         x;
    fixed_t         y;
    fixed_t         z;
    byte            type;
    laser_t         *laser[3];
    laserthinker_t  *laserthinker[3];
    //fixed_t         laserfrac;

    mobj = player->mo;

    lasercount  = 0;
    spread      = 0;
    angleoffs   = 0;

    // the original used a lookup table with the values "0, 1, 1, 2, 1, 2, 2, 3"
    // this is an alternative to using that table
    type = (byte)(((player->artifacts & 3) != 0) +
                  ((player->artifacts & 3) == 3) + ((player->artifacts & 4) != 0));

    // setup laser type
    switch(type) {
    case 1:        // Rapid fire / single shot
        psp->tics = 5;
        lasercount = 1;
        angleoffs = mobj->angle;
        break;
    case 2:        // Rapid fire / double shot
        psp->tics = 4;
        lasercount = 2;
        angleoffs = mobj->angle + 0xFF4A0000;
        spread = 0x16C0000;
        break;
    case 3:        // Spread shot
        psp->tics = 4;
        lasercount = 3;
        spread = 0x2220000 + (0x2220000 * (player->refire & 3));
        angleoffs = mobj->angle - spread;
        break;
    default:    // Normal shot
        lasercount = 1;
        angleoffs = mobj->angle;
        break;
    }

    if(lasercount <= 0) {
        return;
    }

    laserCells = lasercount;

    // setup laser beams
    for(i = 0; i < lasercount; i++) {
        int    hitdice = 0;
        int    damage = 0;

        slope = P_AimLineAttack(mobj, angleoffs, LASERAIMHEIGHT, LASERRANGE);

        player->ammo[weaponinfo[player->readyweapon].ammo]--;

        //
        // [kex] 1/2/12 the old code is just plain bad. the original behavior was
        // to simply call P_AimLineAttack and use the intercept fraction to
        // determine where the tail end of the laser will land. this is
        // optimal for consoles but leads to a lot of issues when working with
        // plane hit detection and auto aiming. P_LineAttack will be called normally
        // and instead of spawning puffs or blood, the xyz values are stored so the
        // tail end of the laser can be setup properly here
        //

        // (unused) adjust aim fraction which will be used to determine
        // the endpoint of the laser
        /*if(aimfrac)
            laserfrac = (aimfrac << (FRACBITS - 4)) - (4 << FRACBITS);
        else
            laserfrac = 0x800;*/

        hitdice = (P_Random(pr_laser) & 7);
        damage = (((hitdice << 2) + hitdice) << 1) + 10;

        P_LineAttack(mobj, angleoffs, LASERRANGE, slope, damage);

        // setup laser
        laser[i] = Z_Malloc(sizeof(*laser[i]), PU_LEVSPEC, 0);

        // setup laser head point
        laser[i]->x1 = mobj->x + FixedMul(LASERDISTANCE, dcos(mobj->angle));
        laser[i]->y1 = mobj->y + FixedMul(LASERDISTANCE, dsin(mobj->angle));
        laser[i]->z1 = (mobj->z + LASERAIMHEIGHT);

        // setup laser tail point
        /*
        laser[i]->x2 = (FixedMul(dcos(angleoffs), laserfrac) + mobj->x);
        laser[i]->y2 = (FixedMul(dsin(angleoffs), laserfrac) + mobj->y);
        laser[i]->z2 = (FixedMul(slope, laserfrac) + (mobj->z + LASERAIMHEIGHT));
        */
        laser[i]->x2 = laserhit_x;
        laser[i]->y2 = laserhit_y;
        laser[i]->z2 = laserhit_z;

        // setup movement slope
        laser[i]->slopex = (dcos(angleoffs) << 5);
        laser[i]->slopey = (dsin(angleoffs) << 5);
        laser[i]->slopez = slope ? (slope << 5) : 0;

        // setup distance info
        x = (laser[i]->x1 - laser[i]->x2) >> FRACBITS;
        y = (laser[i]->y1 - laser[i]->y2) >> FRACBITS;
        z = (laser[i]->z1 - laser[i]->z2) >> FRACBITS;

        laser[i]->dist = 0;
        laser[i]->distmax = (int)sqrt((x * x) + (y * y) + (z * z));

        laser[i]->next = NULL;
        laser[i]->marker = NULL;

        laser[i]->angle = angleoffs;

        x = laser[i]->x2;
        y = laser[i]->y2;
        z = laser[i]->z2;

        P_LaserCrossBSP(numnodes - 1, laser[i]);

        laserthinker[i] = Z_Malloc(sizeof(*laserthinker[i]), PU_LEVSPEC, 0);
        P_AddThinker(&laserthinker[i]->thinker);

        laserthinker[i]->thinker.function.acp1 = (actionf_p1)T_LaserThinker;
        laserthinker[i]->dest = P_SpawnMobj(x, y, z, MT_PROJ_LASER);
        laserthinker[i]->laser = laser[i];

        /*if(linetarget)
        {
            int    hitdice = 0;
            int    damage = 0;

            hitdice = (P_Random(pr_laser) & 7);
            damage = (((hitdice << 2) + hitdice) << 1) + 10;
            P_DamageMobj(linetarget, mobj, mobj, damage);
        }
        else
            angleoffs += spread;*/

        if(!linetarget) {
            angleoffs += spread;
        }
    }

    S_StartSound(player->mo, sfx_laser);
    P_SetMobjState(player->mo, S_007);
    P_SetPsprite(player, ps_flash, weaponinfo[player->readyweapon].flashstate);
}


//
// P_SetupPsprites
// Called at start of level for each player.
//

void P_SetupPsprites(player_t* player) {
    int i;

    // remove all psprites
    for(i=0 ; i<NUMPSPRITES ; i++) {
        player->psprites[i].state = NULL;
    }

    // spawn the gun
    player->pendingweapon = player->readyweapon;
    P_BringUpWeapon(player);
}




//
// P_MovePsprites
// Called every tic by player thinking routine.
//

void P_MovePsprites(player_t* player) {
    int         i;
    pspdef_t*    psp;
    state_t*    state;

    psp = &player->psprites[ps_weapon];

    for(i = 0; i < NUMPSPRITES; i++, psp++) {
        // a null state means not active
        state = psp->state;
        if(state) {
            // drop tic count and possibly change state

            // a -1 tic count never changes
            if(psp->tics != -1) {
                psp->tics--;
                if(!psp->tics) {
                    P_SetPsprite(player, i, psp->state->nextstate);
                }
            }
        }
    }

    player->psprites[ps_flash].sx = player->psprites[ps_weapon].sx;
    player->psprites[ps_flash].sy = player->psprites[ps_weapon].sy;
}


