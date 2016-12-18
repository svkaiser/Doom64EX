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
//      Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------


// Data.
#include "doomdef.h"
#include "d_englsh.h"
#include "sounds.h"
#include "doomstat.h"
#include "m_fixed.h"
#include "m_random.h"
#include "i_system.h"
#include "am_map.h"
#include "p_local.h"
#include "p_macros.h"
#include "s_sound.h"
#include "r_local.h"
#include "con_console.h"
#include "st_stuff.h"

#ifdef __GNUG__
#pragma implementation "p_inter.h"
#endif
#include "p_inter.h"

#include "tables.h"
#include "info.h"

CVAR_EXTERNAL(p_damageindicator);


// a weapon is found with two clip loads,
// a big item has five clip loads
int     maxammo[NUMAMMO] = {200, 50, 300, 50};
int     clipammo[NUMAMMO] = {10, 4, 20, 1};

int infraredFactor = 0;


//
// GET STUFF
//

//
// P_GiveAmmo
// Num is the number of clip loads,
// not the individual count (0= 1/2 clip).
// Returns false if the ammo can't be picked up at all
//

dboolean P_GiveAmmo(player_t* player, ammotype_t ammo, int num) {
    int         oldammo;

    if(ammo == am_noammo) {
        return false;
    }

    if(ammo < 0 || ammo > NUMAMMO) {
        I_Error("P_GiveAmmo: bad type %i", ammo);
    }

    if(player->ammo[ammo] == player->maxammo[ammo]) {
        return false;
    }

    if(num) {
        num *= clipammo[ammo];
    }
    else {
        num = clipammo[ammo]/2;
    }

    if(gameskill == sk_baby
            || gameskill == sk_nightmare) {
        // give double ammo in trainer mode,
        // you'll need in nightmare
        num <<= 1;
    }


    oldammo = player->ammo[ammo];
    player->ammo[ammo] += num;

    if(player->ammo[ammo] > player->maxammo[ammo]) {
        player->ammo[ammo] = player->maxammo[ammo];
    }

    // If non zero ammo,
    // don't change up weapons,
    // player was lower on purpose.
    if(oldammo) {
        return true;
    }

    // We were down to zero,
    // so select a new weapon.
    // Preferences are not user selectable.
    switch(ammo) {
    case am_clip:
        if(player->readyweapon == wp_fist) {
            if(player->weaponowned[wp_chaingun]) {
                player->pendingweapon = wp_chaingun;
            }
            else {
                player->pendingweapon = wp_pistol;
            }
        }
        break;

    case am_shell:
        if(player->readyweapon == wp_fist
                || player->readyweapon == wp_pistol) {
            if(player->weaponowned[wp_shotgun]) {
                player->pendingweapon = wp_shotgun;
            }
        }
        break;

    case am_cell:
        if(player->readyweapon == wp_fist
                || player->readyweapon == wp_pistol) {
            if(player->weaponowned[wp_plasma]) {
                player->pendingweapon = wp_plasma;
            }
        }
        break;

    case am_misl:
        if(player->readyweapon == wp_fist) {
            if(player->weaponowned[wp_missile]) {
                player->pendingweapon = wp_missile;
            }
        }
    default:
        break;
    }

    return true;
}


//
// P_GiveWeapon
// The weapon name may have a MF_DROPPED flag ored in.
//
dboolean P_GiveWeapon(player_t* player, mobj_t *item, weapontype_t weapon, dboolean dropped) {
    dboolean    gaveammo;
    dboolean    gaveweapon;

    if(netgame && (deathmatch!=2) && !dropped) {
        if(item && item->flags & MF_TRIGTOUCH) {
            P_SpawnMobj(item->x, item->y, item->z, item->type);
            return true;
        }

        // leave placed weapons forever on net games
        if(player->weaponowned[weapon]) {
            return false;
        }

        player->bonuscount += BONUSADD;
        player->weaponowned[weapon] = true;

        if(deathmatch) {
            P_GiveAmmo(player, weaponinfo[weapon].ammo, 5);
        }
        else {
            P_GiveAmmo(player, weaponinfo[weapon].ammo, 2);
        }
        player->pendingweapon = weapon;

        return false;
    }

    if(weaponinfo[weapon].ammo != am_noammo) {
        // give one clip with a dropped weapon,
        // two clips with a found weapon
        gaveammo = P_GiveAmmo(player, weaponinfo[weapon].ammo, dropped ? 1 : 2);
    }
    else {
        gaveammo = false;
    }

    if(player->weaponowned[weapon]) {
        gaveweapon = false;
    }
    else {
        gaveweapon = true;
        player->weaponowned[weapon] = true;
        player->pendingweapon = weapon;
    }

    return (gaveweapon || gaveammo);
}



//
// P_GiveBody
// Returns false if the body isn't needed at all
//
dboolean P_GiveBody(player_t* player, int num) {
    if(player->health >= MAXHEALTH) {
        return false;
    }

    player->health += num;
    if(player->health > MAXHEALTH) {
        player->health = MAXHEALTH;
    }
    player->mo->health = player->health;

    return true;
}



//
// P_GiveArmor
// Returns false if the armor is worse
// than the current armor.
//
dboolean P_GiveArmor(player_t* player, int armortype) {
    int         hits;

    hits = armortype*100;
    if(player->armorpoints >= hits) {
        return false;    // don't pick up
    }

    player->armortype = armortype;
    player->armorpoints = hits;

    return true;
}


//
// P_GiveCard
//
static dboolean P_GiveCard(player_t* player, mobj_t *item, card_t card) {
    if(netgame && (item && item->flags & MF_TRIGTOUCH)) {
        P_SpawnMobj(item->x, item->y, item->z, item->type);
        return true;
    }

    if(player->cards[card]) {
        return false;
    }

    player->bonuscount = BONUSADD;
    player->cards[card] = 1;

    switch(card) {
    case it_bluecard:
        player->message = GOTBLUECARD;
        player->messagepic = 25;
        break;
    case it_yellowcard:
        player->message = GOTYELWCARD;
        player->messagepic = 26;
        break;
    case it_redcard:
        player->message = GOTREDCARD;
        player->messagepic = 27;
        break;
    case it_blueskull:
        player->message = GOTBLUESKUL;
        player->messagepic = 28;
        break;
    case it_yellowskull:
        player->message = GOTYELWSKUL;
        player->messagepic = 29;
        break;
    case it_redskull:
        player->message = GOTREDSKULL;
        player->messagepic = 30;
        break;
    }

    if(netgame) {
        return false;
    }

    return true;
}


//
// P_GivePower
//
dboolean P_GivePower(player_t* player, int power) {
    if(power == pw_invulnerability) {
        player->powers[power] = INVULNTICS;
        return true;
    }

    if(power == pw_invisibility) {
        player->powers[power] = INVISTICS;
        player->mo->flags |= MF_SHADOW;
        return true;
    }

    if(power == pw_infrared) {
        player->powers[power] = INFRATICS;

        if(&players[displayplayer] == player) {
            infraredFactor = 300;
            R_RefreshBrightness();
        }

        return true;
    }

    if(power == pw_ironfeet) {
        player->powers[power] = IRONTICS;
        return true;
    }

    if(power == pw_strength) {
        P_GiveBody(player, 100);
        player->powers[power] = STRTICS;
        return true;
    }

    if(player->powers[power]) {
        return false;    // already got it
    }

    player->powers[power] = 1;
    return true;
}


//
// P_TouchSpecialThing
//
void P_TouchSpecialThing(mobj_t* special, mobj_t* toucher) {
    player_t*   player;
    fixed_t     delta;
    int         sound;
    int            i = 0;

    delta = special->z - toucher->z;

    if(delta > toucher->height
            || delta < -8*FRACUNIT) {
        // out of reach
        return;
    }


    sound = sfx_itemup;
    player = toucher->player;

    // Dead thing touching.
    // Can happen with a sliding player corpse.
    if(toucher->health <= 0) {
        return;
    }

    // Identify by sprite.
    switch(special->sprite) {
    // armor
    case SPR_ARM1:
        if(!P_GiveArmor(player, 1)) {
            return;
        }
        player->message = GOTARMOR;
        player->messagepic = 23;
        break;

    case SPR_ARM2:
        if(!P_GiveArmor(player, 2)) {
            return;
        }
        player->message = GOTMEGA;
        player->messagepic = 24;
        break;

    // bonus items
    case SPR_BON1:
        player->health+=2;               // can go over 100%
        if(player->health > 200) {
            player->health = 200;
        }
        player->mo->health = player->health;
        player->message = GOTHTHBONUS;
        player->messagepic = 3;
        break;

    case SPR_BON2:
        player->armorpoints+=2;          // can go over 100%
        if(player->armorpoints > 200) {
            player->armorpoints = 200;
        }
        if(!player->armortype) {
            player->armortype = 1;
        }
        player->message = GOTARMBONUS;
        player->messagepic = 4;
        break;

    case SPR_SOUL:
        player->health += 100;
        if(player->health > 200) {
            player->health = 200;
        }
        player->mo->health = player->health;
        player->message = GOTSUPER;
        player->messagepic = 5;
        sound = sfx_powerup;
        break;

    case SPR_MEGA:
        player->health = 200;
        player->mo->health = player->health;
        P_GiveArmor(player,2);
        player->message = GOTMSPHERE;
        player->messagepic = 6;
        sound = sfx_powerup;
        break;

    // cards
    // leave cards for everyone
    case SPR_BKEY:
        if(!(P_GiveCard(player, special, it_bluecard))) {
            return;
        }
        break;

    case SPR_YKEY:
        if(!(P_GiveCard(player, special, it_yellowcard))) {
            return;
        }
        break;

    case SPR_RKEY:
        if(!(P_GiveCard(player, special, it_redcard))) {
            return;
        }
        break;

    case SPR_BSKU:
        if(!(P_GiveCard(player, special, it_blueskull))) {
            return;
        }
        break;

    case SPR_YSKU:
        if(!(P_GiveCard(player, special, it_yellowskull))) {
            return;
        }
        break;

    case SPR_RSKU:
        if(!(P_GiveCard(player, special, it_redskull))) {
            return;
        }
        break;

    // medikits, heals
    case SPR_STIM:
        if(!P_GiveBody(player, 10)) {
            return;
        }
        player->message = GOTSTIM;
        player->messagepic = 31;
        break;

    case SPR_MEDI:
        if(!P_GiveBody(player, 25)) {
            return;
        }

        if(player->health < 25) {
            player->message = GOTMEDINEED;
            player->messagepic = 32;
        }
        else {
            player->message = GOTMEDIKIT;
            player->messagepic = 33;
        }
        break;


    // power ups
    case SPR_PINV:
        if(!P_GivePower(player, pw_invulnerability)) {
            return;
        }
        player->message = GOTINVUL;
        player->messagepic = 34;
        sound = sfx_powerup;
        break;

    case SPR_PSTR:
        if(!P_GivePower(player, pw_strength)) {
            return;
        }
        player->message = GOTBERSERK;
        player->messagepic = 35;
        if(player->readyweapon != wp_fist) {
            player->pendingweapon = wp_fist;
        }
        sound = sfx_powerup;
        break;

    case SPR_PINS:
        if(!P_GivePower(player, pw_invisibility)) {
            return;
        }
        player->message = GOTINVIS;
        player->messagepic = 36;
        sound = sfx_powerup;
        break;

    case SPR_SUIT:
        if(!P_GivePower(player, pw_ironfeet)) {
            return;
        }
        player->message = GOTSUIT;
        player->messagepic = 37;
        sound = sfx_powerup;
        break;

    case SPR_PMAP:
        if(!P_GivePower(player, pw_allmap)) {
            return;
        }
        player->message = GOTMAP;
        player->messagepic = 38;
        sound = sfx_powerup;
        break;

    case SPR_PVIS:
        if(!P_GivePower(player, pw_infrared)) {
            return;
        }
        player->message = GOTVISOR;
        player->messagepic = 39;
        sound = sfx_powerup;
        break;

    // ammo
    case SPR_CLIP:
        if(special->flags & MF_DROPPED) {
            if(!P_GiveAmmo(player,am_clip,0)) {
                return;
            }
        }
        else {
            if(!P_GiveAmmo(player,am_clip,1)) {
                return;
            }
        }
        player->message = GOTCLIP;
        player->messagepic = 7;
        break;

    case SPR_AMMO:
        if(!P_GiveAmmo(player, am_clip,5)) {
            return;
        }
        player->message = GOTCLIPBOX;
        player->messagepic = 8;
        break;

    case SPR_RCKT:
        if(!P_GiveAmmo(player, am_misl,1)) {
            return;
        }
        player->message = GOTROCKET;
        player->messagepic = 9;
        break;

    case SPR_BROK:
        if(!P_GiveAmmo(player, am_misl,5)) {
            return;
        }
        player->message = GOTROCKBOX;
        player->messagepic = 10;
        break;

    case SPR_CELL:
        if(!P_GiveAmmo(player, am_cell,1)) {
            return;
        }
        player->message = GOTCELL;
        player->messagepic = 11;
        break;

    case SPR_CELP:
        if(!P_GiveAmmo(player, am_cell,5)) {
            return;
        }
        player->message = GOTCELLBOX;
        player->messagepic = 12;
        break;

    case SPR_SHEL:
        if(!P_GiveAmmo(player, am_shell,1)) {
            return;
        }
        player->message = (gameskill == sk_baby)?GOTSHELLS2:GOTSHELLS;    //villsa
        player->messagepic = 13;
        break;

    case SPR_SBOX:
        if(!P_GiveAmmo(player, am_shell,5)) {
            return;
        }
        player->message = GOTSHELLBOX;
        player->messagepic = 14;
        break;

    case SPR_BPAK:
        if(!player->backpack) {
            for(i = 0; i < NUMAMMO; i++) {
                player->maxammo[i] *= 2;
            }

            player->backpack = true;
        }
        for(i = 0; i < NUMAMMO; i++) {
            P_GiveAmmo(player, i, 1);
        }

        player->message = GOTBACKPACK;
        player->messagepic = 15;
        break;

    // weapons
    case SPR_BFUG:
        if(!P_GiveWeapon(player, special, wp_bfg, false)) {
            return;
        }
        player->message = GOTBFG9000;
        player->messagepic = 16;
        sound = sfx_sgcock;
        break;

    case SPR_MGUN:
        if(!P_GiveWeapon(player, special, wp_chaingun, special->flags&MF_DROPPED)) {
            return;
        }
        player->message = GOTCHAINGUN;
        player->messagepic = 17;
        sound = sfx_sgcock;
        break;

    case SPR_CSAW:
        if(!P_GiveWeapon(player, special, wp_chainsaw, false)) {
            return;
        }
        player->message = GOTCHAINSAW;
        player->messagepic = 18;
        sound = sfx_sgcock;
        break;

    case SPR_LAUN:
        if(!P_GiveWeapon(player, special, wp_missile, false)) {
            return;
        }
        player->message = GOTLAUNCHER;
        player->messagepic = 19;
        sound = sfx_sgcock;
        break;

    case SPR_PLSM:
        if(!P_GiveWeapon(player, special, wp_plasma, false)) {
            return;
        }
        player->message = GOTPLASMA;
        player->messagepic = 20;
        sound = sfx_sgcock;
        break;

    case SPR_SHOT:
        if(!P_GiveWeapon(player, special, wp_shotgun, special->flags&MF_DROPPED)) {
            return;
        }
        player->message = GOTSHOTGUN;
        player->messagepic = 21;
        sound = sfx_sgcock;
        break;

    case SPR_SGN2:
        if(!P_GiveWeapon(player, special, wp_supershotgun, special->flags&MF_DROPPED)) {
            return;
        }
        player->message = GOTSHOTGUN2;
        player->messagepic = 22;
        sound = sfx_sgcock;
        break;

    case SPR_LSRG:
        if(!P_GiveWeapon(player, special, wp_laser, false)) {
            return;
        }
        player->message = GOTLASER;
        sound = sfx_sgcock;
        break;

    case SPR_ART1:
        if(netgame && player->artifacts & (1<<ART_FAST)) {
            return;
        }

        player->artifacts |= (1<<ART_FAST);
        player->message = GOTARTIFACT1;
        player->messagepic = 41;
        break;

    case SPR_ART2:
        if(netgame && player->artifacts & (1<<ART_DOUBLE)) {
            return;
        }

        player->artifacts |= (1<<ART_DOUBLE);
        player->message = GOTARTIFACT2;
        player->messagepic = 42;
        break;

    case SPR_ART3:
        if(netgame && player->artifacts & (1<<ART_TRIPLE)) {
            return;
        }

        player->artifacts |= (1<<ART_TRIPLE);
        player->message = GOTARTIFACT3;
        player->messagepic = 43;
        break;

    default:
        if(special->type != MT_FAKEITEM) {
            CON_Printf(YELLOW, "P_SpecialThing: Unknown gettable thing: %s\n", sprnames[special->sprite]);
            special->flags &= ~MF_SPECIAL;
            return;
        }
        break;
    }

    if(special->flags & MF_TRIGTOUCH || special->type == MT_FAKEITEM) {
        if(special->tid) {
            P_QueueSpecial(special);
        }
    }

    if(special->type != MT_FAKEITEM) {
        if(special->flags & MF_COUNTITEM) {
            player->itemcount++;
        }

        if(special->flags & MF_COUNTSECRET) {
            player->secretcount++;
        }

        P_RemoveMobj(special);
        player->bonuscount += BONUSADD;

        if(player == &players[consoleplayer]) {
            S_StartSound(NULL, sound);
        }
    }
}

//
// P_Obituary
//

static void P_Obituary(mobj_t* source, mobj_t* target) {
    static char omsg[128];
    char *name;
    int i;

    if(!target->player) {
        return;
    }

    name = player_names[target->player - players];

    if(source != NULL) {
        switch(source->type) {
        case MT_POSSESSED1:
            sprintf(omsg, "%s\nwas tickled to death\nby a Zombieman.", name);
            break;
        case MT_POSSESSED2:
            sprintf(omsg, "%s\ntook a shotgun to the face.", name);
            break;
        case MT_IMP1:
            sprintf(omsg, "%s\nwas burned by an Imp.", name);
            break;
        case MT_IMP2:
            sprintf(omsg, "%s\nwas killed by a\nNightmare Imp.", name);
            break;
        case MT_DEMON1:
            sprintf(omsg, "%s\nwas bit by a Demon.", name);
            break;
        case MT_DEMON2:
            sprintf(omsg, "%s\nwas eaten by a Spectre.", name);
            break;
        case MT_MANCUBUS:
            sprintf(omsg, "%s\nwas squashed by a Mancubus.", name);
            break;
        case MT_CACODEMON:
            sprintf(omsg, "%s\nwas smitten by a Cacodemon.", name);
            break;
        case MT_BRUISER1:
            sprintf(omsg, "%s\nwas bruised by a\nBaron of Hell.", name);
            break;
        case MT_BRUISER2:
            sprintf(omsg, "%s\nwas splayed by a\nHell Knight.", name);
            break;
        case MT_BABY:
            sprintf(omsg, "%s\nwas vaporized by\nan Arachnotron.", name);
            break;
        case MT_SKULL:
            sprintf(omsg, "A Lost Soul slammed into\n%s.", name);
            break;
        case MT_CYBORG:
            sprintf(omsg, "%s\nwas splattered by a\nCyberdemon.", name);
            break;
        case MT_RESURRECTOR:
            sprintf(omsg, "%s\nwas destroyed by \nthe Resurrector.", name);
            break;
        case MT_PLAYERBOT1:
        case MT_PLAYERBOT2:
        case MT_PLAYERBOT3:
            sprintf(omsg, "%s\nwas killed by a marine.", name);
            break;
        default:
            sprintf(omsg, "%s died.", name);
            break;
        }
    }
    else {
        sprintf(omsg, "%s died.", name);
    }

    for(i = 0; i < MAXPLAYERS; i++) {
        if(playeringame[i]) {
            players[i].message = omsg;
        }
    }


}


//
// KillMobj
//
extern int deathmocktics;

void P_KillMobj(mobj_t* source, mobj_t* target) {
    mobjtype_t  item;
    mobj_t*     mo;

    target->flags &= ~(MF_SHOOTABLE|MF_FLOAT|MF_SKULLFLY);

    if(target->type != MT_SKULL && (!(target->flags & MF_GRAVITY))) {
        target->flags |= MF_GRAVITY;
    }

    target->flags |= MF_CORPSE|MF_DROPOFF;
    target->height >>= 2;

    if(source && source->player) {
        // count for intermission
        if(target->flags & MF_COUNTKILL) {
            source->player->killcount++;
        }

        if(target->player) {
            source->player->frags[target->player-players]++;
        }
    }
    else if(!netgame && (target->flags & MF_COUNTKILL)) {
        // count all monster deaths,
        // even those caused by other monsters
        players[0].killcount++;
    }

    if(target->player) {
        // count environment kills against you
        if(!source) {
            target->player->frags[target->player-players]++;
        }

        target->flags &= ~MF_SOLID;
        target->player->playerstate = PST_DEAD;
        P_DropWeapon(target->player);

        deathmocktics = gametic;

        if(target->player == &players[consoleplayer]
                && automapactive) {
            // don't die in auto map,
            // switch view prior to dying
            AM_Stop();
        }

        // 20120123 villsa - obituaries!
        if(netgame) {
            P_Obituary(source, target);
        }

    }

    if(target->health < -target->info->spawnhealth
            && target->info->xdeathstate) {
        P_SetMobjState(target, target->info->xdeathstate);
    }
    else {
        P_SetMobjState(target, target->info->deathstate);
    }
    target->tics -= P_Random(pr_killtics)&3;

    if(target->tics < 1) {
        target->tics = 1;
    }


    // Drop stuff.
    // This determines the kind of object spawned
    // during the death frame of a thing.
    switch(target->type) {
    case MT_POSSESSED1:
        item = MT_AMMO_CLIP;
        break;

    case MT_POSSESSED2:
        item = MT_WEAP_SHOTGUN;
        break;

    default:
        return;
    }

    mo = P_SpawnMobj(target->x,target->y,ONFLOORZ, item);
    mo->flags |= MF_DROPPED;    // special versions of items
}




//
// P_DamageMobj
// Damages both enemies and players
// "inflictor" is the thing that caused the damage
//  creature or missile, can be NULL (slime, etc)
// "source" is the thing to target after taking damage
//  creature or NULL
// Source and inflictor are the same for melee attacks.
// Source can be NULL for slime, barrel explosions
// and other environmental stuff.
//
void P_DamageMobj(mobj_t* target, mobj_t* inflictor, mobj_t* source, int damage) {
    angle_t     ang;
    int         saved;
    player_t*   player;
    fixed_t     thrust;
    int         temp;

    if(!(target->flags & MF_SHOOTABLE)) {
        return;    // shouldn't happen...
    }

    if(target->health <= 0) {
        return;
    }

    if(source && target) {
        if(source->player &&
                (target->player && target->player != source->player) &&
                !(gameflags & GF_FRIENDLYFIRE)) {
            // don't take damage from teammates
            return;
        }

        if(p_damageindicator.value) {
            if(target->player && !source->player) {
                ST_AddDamageMarker(target, source);
            }
        }
    }

    // [d64] stop all flying skull movement
    if(target->flags & MF_SKULLFLY) {
        target->momx = 0;
        target->momy = 0;
        target->momz = 0;
    }

    player = target->player;

    if(player && gameskill == sk_baby) {
        damage >>= 1;    // take half damage in trainer mode
    }


    // Some close combat weapons should not
    // inflict thrust and push the victim out of reach,
    // thus kick away unless using the chainsaw.
    if(inflictor && !(target->flags & MF_NOCLIP) &&
            (!source || !source->player || source->player->readyweapon != wp_chainsaw)) {
        ang = R_PointToAngle2(inflictor->x, inflictor->y, target->x, target->y);
        thrust = damage * (FRACUNIT >> 2) * 100 / target->info->mass;

        // make fall forwards sometimes
        if(damage < 40 && damage > target->health &&
                target->z - inflictor->z > 64*FRACUNIT && (P_Random(pr_damagemobj) & 1)) {
            ang += ANG180;
            thrust *= 4;
        }

        ang >>= ANGLETOFINESHIFT;

        thrust >>= 16;
        target->momx += thrust * finecosine[ang];
        target->momy += thrust * finesine[ang];

        if(target->momx < (16*FRACUNIT)) {
            if(target->momx < -(16*FRACUNIT)) {
                target->momx = -(16*FRACUNIT);
            }
        }
        else {
            target->momx = (16*FRACUNIT);
        }

        if(target->momy < (16*FRACUNIT)) {
            if(target->momy < -(16*FRACUNIT)) {
                target->momy = -(16*FRACUNIT);
            }
        }
        else {
            target->momy = (16*FRACUNIT);
        }
    }

    // player specific
    if(player) {
        // ignore damage in GOD mode, or with INVUL power.
        if((player->cheats & CF_GODMODE) || player->powers[pw_invulnerability]) {
            return;
        }

        if(player->armortype) {
            if(player->armortype == 1) {
                saved = damage / 3;
            }
            else {
                saved = damage / 2;
            }

            if(player->armorpoints <= saved) {
                // armor is used up
                saved = player->armorpoints;
                player->armortype = 0;
            }
            player->armorpoints -= saved;
            damage -= saved;
        }
        player->health -= damage;       // mirror mobj health here for Dave
        if(player->health < 0) {
            player->health = 0;
        }

        player->attacker = source;
        player->damagecount += (damage<<1);  // add damage after armor / invuln

        if(player->damagecount > 100) {
            player->damagecount = 100;    // teleport stomp does 10k points...
        }

        temp = damage < 100 ? damage : 100;
    }

    // do the damage
    target->health -= damage;

    if(target->health <= 0) {
        if(devparm && target->player && target->player->cheats & CF_UNDYING) {
            target->player->health = 10;
            target->health = 10;
            return;
        }
        else {
            P_KillMobj(source, target);
            return;
        }
    }

    if((P_Random(pr_painchance) < target->info->painchance) && !(target->flags & MF_SKULLFLY)) {
        target->flags |= MF_JUSTHIT;    // fight back!
        P_SetMobjState(target, target->info->painstate);
    }

    target->reactiontime = 0;           // we're awake now...

    if((!(target->flags & MF_NOINFIGHTING)) &&
            ((!target->threshold) && source && source != target)) {
        // if not intent on another player,
        // chase after this one

        if(source->type != MT_DEST_PROJECTILE) {
            P_SetTarget(&target->target, source);
            target->threshold = BASETHRESHOLD;
            if(target->state == &states[target->info->spawnstate] && target->info->seestate != S_000) {
                P_SetMobjState(target, target->info->seestate);
            }
        }
    }

}

