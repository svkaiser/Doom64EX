// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 1993-1997 Id Software, Inc.
// Copyright(C) 2005 Simon Howard
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
//    Archiving: SaveGame I/O.
//
//-----------------------------------------------------------------------------

#include <time.h> // [kex] - for saving the date and time

#include "i_system.h"
#include "g_game.h"
#include "z_zone.h"
#include "p_local.h"
#include "p_macros.h"
#include "doomstat.h"
#include "info.h"
#include "m_password.h"
#include "p_saveg.h"
#include "d_englsh.h"
#include "m_misc.h"
#include "doomdef.h" // added just so MSVC would shut up about warning C4761

void G_DoLoadLevel(void);

//
// consistency markers
//

#define SAVEGAME_EOF    0x464F45
#define SAVEGAME_MOBJ   0x4A424F4D

static FILE*    save_stream;
static byte*    savebuffer;

static unsigned long save_offset = 0;

//
// P_GetSaveGameName
//

char *P_GetSaveGameName(int num) {
    static char name[256];

#ifdef _WIN32
    sprintf(name, SAVEGAMENAME"%d.dsg", num);
#else
    // 20120105 bkw: UNIX-friendly savegame location
    sprintf(name, "%s/.doom64ex/"SAVEGAMENAME"%d.dsg", getenv("HOME"), num);
#endif

    return name;
}

//------------------------------------------------------------------------
//
// Endian-safe integer read/write functions
//
//------------------------------------------------------------------------

static byte saveg_read8(void) {
    byte result;

    result = savebuffer[save_offset++];

    return result;
}

static void saveg_write8(byte value) {
    fwrite(&value, 1, 1, save_stream);
    save_offset++;
}

static short saveg_read16(void) {
    int result;

    result = saveg_read8();
    result |= saveg_read8() << 8;

    return result;
}

static void saveg_write16(short value) {
    saveg_write8(value & 0xff);
    saveg_write8((value >> 8) & 0xff);
}

static int saveg_read32(void) {
    int result;

    result = saveg_read8();
    result |= saveg_read8() << 8;
    result |= saveg_read8() << 16;
    result |= saveg_read8() << 24;

    return result;
}

static void saveg_write32(int value) {
    saveg_write8(value & 0xff);
    saveg_write8((value >> 8) & 0xff);
    saveg_write8((value >> 16) & 0xff);
    saveg_write8((value >> 24) & 0xff);
}

//------------------------------------------------------------------------
//
// Pad to 4-byte boundaries
//
//------------------------------------------------------------------------

static void saveg_read_pad(void) {
    /*unsigned long pos;
    int padding;
    int i;

    //
    // avoid using fseek due to returning incorrect file offsets
    //
    pos = save_offset;

    padding = (4 - (pos & 3)) & 3;

    for(i = 0; i < padding; i++)
        saveg_read8();*/
}

static void saveg_write_pad(void) {
    /*unsigned long pos;
    int padding;
    int i;

    //
    // avoid using fseek due to returning incorrect file offsets
    //
    pos = save_offset;

    padding = (4 - (pos & 3)) & 3;

    for(i = 0; i < padding; i++)
        saveg_write8(0);*/
}

//------------------------------------------------------------------------
//
// Mobj read/save functions
//
//------------------------------------------------------------------------

typedef struct {
    int index;
    mobj_t* mobj;
} savegmobj_t;

static savegmobj_t* savegmobj;
static int          savegmobjnum;

static void saveg_setup_mobjwrite(void) {
    mobj_t* mobj;
    int i;

    savegmobjnum = 0;

    // count mobjs
    for(mobj = mobjhead.next; mobj != &mobjhead; mobj = mobj->next) {
        // don't save if mobj is expected to be removed
        if(mobj->mobjfunc == P_SafeRemoveMobj) {
            continue;
        }

        savegmobjnum++;
    }

    // allocate ref table
    savegmobj = (savegmobj_t*)Z_Alloca(sizeof(savegmobj_t) * savegmobjnum);
    i = 0;

    // store index and mobj
    for(mobj = mobjhead.next; mobj != &mobjhead; mobj = mobj->next) {
        // don't save if mobj is expected to be removed
        if(mobj->mobjfunc == P_SafeRemoveMobj) {
            continue;
        }

        savegmobj[i].index = i + 1;
        savegmobj[i].mobj = mobj;
        i++;
    }
}

static void saveg_setup_mobjread(void) {
    int i;

    // get count and allocate table
    savegmobjnum = saveg_read32();
    savegmobj = (savegmobj_t*)Z_Alloca(sizeof(savegmobj_t) * savegmobjnum);

    // read and add mobjs
    for(i = 0; i < savegmobjnum; i++) {
        savegmobj[i].index = i + 1;
        savegmobj[i].mobj = Z_Calloc(sizeof(mobj_t), PU_LEVEL, NULL);
    }
}

static void saveg_write_mobjindex(mobj_t* mobj) {
    int i;

    for(i = 0; i < savegmobjnum; i++) {
        if(savegmobj[i].mobj != mobj) {
            continue;
        }

        saveg_write32(savegmobj[i].index);
        return;
    }

    saveg_write32(0);
}

static mobj_t* saveg_read_mobjindex(void) {
    int index = saveg_read32();

    if(index) {
        int i;

        for(i = 0; i < savegmobjnum; i++) {
            if(savegmobj[i].index == index) {
                return savegmobj[i].mobj;
            }
        }
    }

    return NULL;
}

/*
 * killough 11/98
 *
 * Same as P_SetTarget() in p_mobj.c, except that the target is nullified
 * first, so that no old target's reference count is decreased (when loading
 * savegames, old targets are indices, not really pointers to targets).
 */

static void saveg_set_mobjtarget(mobj_t **mop, mobj_t *targ) {
    *mop = NULL;
    P_SetTarget(mop, targ);
}

//------------------------------------------------------------------------
//
// Structure read/write functions
//
//------------------------------------------------------------------------

//
// mapthing_t
//

static void saveg_read_mapthing_t(mapthing_t* mt) {
    mt->x       = saveg_read16();
    mt->y       = saveg_read16();
    mt->z       = saveg_read16();
    mt->angle   = saveg_read16();
    mt->type    = saveg_read16();
    mt->options = saveg_read16();
    mt->tid     = saveg_read16();
    saveg_read_pad();
}

static void saveg_write_mapthing_t(mapthing_t* mt) {
    saveg_write16(mt->x);
    saveg_write16(mt->y);
    saveg_write16(mt->z);
    saveg_write16(mt->angle);
    saveg_write16(mt->type);
    saveg_write16(mt->options);
    saveg_write16(mt->tid);
    saveg_write_pad();
}

//
// ticcmd_t
//

static void saveg_read_ticcmd_t(ticcmd_t* cmd) {
    cmd->forwardmove    = saveg_read8();
    cmd->sidemove       = saveg_read8();
    cmd->angleturn      = saveg_read16();
    cmd->pitch          = saveg_read16();
    cmd->consistancy    = saveg_read8();
    cmd->chatchar       = saveg_read8();
    cmd->buttons        = saveg_read8();
    cmd->buttons2       = saveg_read8();
}

static void saveg_write_ticcmd_t(ticcmd_t* cmd) {
    saveg_write8(cmd->forwardmove);
    saveg_write8(cmd->sidemove);
    saveg_write16(cmd->angleturn);
    saveg_write16(cmd->pitch);
    saveg_write8(cmd->consistancy);
    saveg_write8(cmd->chatchar);
    saveg_write8(cmd->buttons);
    saveg_write8(cmd->buttons2);
}

//
// mobj_t
//

static void saveg_write_mobj_t(mobj_t* mo) {
    saveg_write32(mo->x);
    saveg_write32(mo->y);
    saveg_write32(mo->z);
    saveg_write32(mo->tid);
    saveg_write_mobjindex(mo->snext);
    saveg_write_mobjindex(mo->sprev);
    saveg_write32(mo->angle);
    saveg_write32(mo->pitch);
    saveg_write32(mo->sprite);
    saveg_write32(mo->frame);
    saveg_write_mobjindex(mo->bnext);
    saveg_write_mobjindex(mo->bprev);
    saveg_write32(mo->subsector - subsectors);
    saveg_write32(mo->floorz);
    saveg_write32(mo->ceilingz);
    saveg_write32(mo->radius);
    saveg_write32(mo->height);
    saveg_write32(mo->momx);
    saveg_write32(mo->momy);
    saveg_write32(mo->momz);
    saveg_write32(mo->validcount);
    saveg_write32(mo->state - states);
    saveg_write32(mo->type);
    saveg_write32(mo->tics);
    saveg_write32(mo->flags);
    saveg_write32(mo->health);
    saveg_write32(mo->alpha);
    saveg_write32(mo->blockflag);
    saveg_write32(mo->movedir);
    saveg_write32(mo->movecount);
    saveg_write_mobjindex(mo->target);
    saveg_write32(mo->reactiontime);
    saveg_write32(mo->threshold);
    saveg_write32(mo->player ? mo->player - players + 1 : 0);
    saveg_write_mapthing_t(&mo->spawnpoint);
    saveg_write_mobjindex(mo->tracer);
    saveg_write32(mo->frame_x);
    saveg_write32(mo->frame_y);
    saveg_write32(mo->frame_z);
    saveg_write32(mo->mobjfunc == P_RespawnSpecials ? 1 : 0);
}

static void saveg_read_mobj_t(mobj_t* mo) {
    int pl;

    mo->x               = saveg_read32();
    mo->y               = saveg_read32();
    mo->z               = saveg_read32();
    mo->tid             = saveg_read32();
    mo->snext           = saveg_read_mobjindex();
    mo->sprev           = saveg_read_mobjindex();
    mo->angle           = saveg_read32();
    mo->pitch           = saveg_read32();
    mo->sprite          = saveg_read32();
    mo->frame           = saveg_read32();
    mo->bnext           = saveg_read_mobjindex();
    mo->bprev           = saveg_read_mobjindex();
    mo->subsector       = &subsectors[saveg_read32() - numsubsectors];
    mo->floorz          = saveg_read32();
    mo->ceilingz        = saveg_read32();
    mo->radius          = saveg_read32();
    mo->height          = saveg_read32();
    mo->momx            = saveg_read32();
    mo->momy            = saveg_read32();
    mo->momz            = saveg_read32();
    mo->validcount      = saveg_read32();
    mo->state           = &states[saveg_read32()];
    mo->type            = saveg_read32();
    mo->tics            = saveg_read32();
    mo->flags           = saveg_read32();
    mo->health          = saveg_read32();
    mo->alpha           = saveg_read32();
    mo->blockflag       = saveg_read32();
    mo->movedir         = saveg_read32();
    mo->movecount       = saveg_read32();

    saveg_set_mobjtarget(&mo->target, saveg_read_mobjindex());

    mo->reactiontime    = saveg_read32();
    mo->threshold       = saveg_read32();
    pl                  = saveg_read32();
    mo->player          = pl > 0 ? &players[pl - 1] : NULL;
    saveg_read_mapthing_t(&mo->spawnpoint);

    saveg_set_mobjtarget(&mo->tracer, saveg_read_mobjindex());

    mo->frame_x         = saveg_read32();
    mo->frame_y         = saveg_read32();
    mo->frame_z         = saveg_read32();
    mo->mobjfunc        = saveg_read32() ? P_RespawnSpecials : NULL;
}

//
// pspdef_t
//

static void saveg_read_pspdef_t(pspdef_t *psp) {
    int state;

    state = saveg_read32();

    psp->state      = state > 0 ? &states[state] : NULL;
    psp->tics       = saveg_read32();
    psp->sx         = saveg_read32();
    psp->sy         = saveg_read32();
    psp->alpha      = saveg_read32();
    psp->frame_x    = saveg_read32();
    psp->frame_y    = saveg_read32();
}

static void saveg_write_pspdef_t(pspdef_t *psp) {
    saveg_write32(psp->state ? psp->state - states : 0);
    saveg_write32(psp->tics);
    saveg_write32(psp->sx);
    saveg_write32(psp->sy);
    saveg_write32(psp->alpha);
    saveg_write32(psp->frame_x);
    saveg_write32(psp->frame_y);
}

//
// player_t
//

static void saveg_write_player_t(player_t* p) {
    int i;

    saveg_write_mobjindex(p->mo);
    saveg_write32(p->playerstate);
    saveg_write_ticcmd_t(&p->cmd);
    saveg_write32(p->viewz);
    saveg_write32(p->viewheight);
    saveg_write32(p->deltaviewheight);
    saveg_write32(p->bob);
    saveg_write32(p->recoilpitch);
    saveg_write32(p->health);
    saveg_write32(p->armorpoints);
    saveg_write32(p->armortype);

    for(i = 0; i < NUMPOWERS; i++) {
        saveg_write32(p->powers[i]);
    }

    for(i = 0; i < NUMCARDS; i++) {
        saveg_write32(p->cards[i]);
        saveg_write32(p->tryopen[i]);
    }

    saveg_write32(p->artifacts);
    saveg_write32(p->backpack);

    for(i = 0; i < MAXPLAYERS; i++) {
        saveg_write32(p->frags[i]);
    }

    saveg_write32(p->readyweapon);
    saveg_write32(p->pendingweapon);

    for(i = 0; i < NUMWEAPONS; i++) {
        saveg_write32(p->weaponowned[i]);
    }

    for(i = 0; i < NUMAMMO; i++) {
        saveg_write32(p->ammo[i]);
        saveg_write32(p->maxammo[i]);
    }

    saveg_write32(p->attackdown);
    saveg_write32(p->usedown);
    saveg_write32(p->jumpdown);
    saveg_write32(p->cheats);
    saveg_write32(p->refire);
    saveg_write32(p->killcount);
    saveg_write32(p->itemcount);
    saveg_write32(p->secretcount);
    saveg_write32(p->damagecount);
    saveg_write32(p->bonuscount);
    saveg_write32(p->bfgcount);
    saveg_write_mobjindex(p->attacker);
    saveg_write_mobjindex(p->cameratarget);

    for(i = 0; i < NUMPSPRITES; i++) {
        saveg_write_pspdef_t(&p->psprites[i]);
    }

    saveg_write32(p->palette);
    saveg_write32(p->onground);
    saveg_write32(p->autoaim);
}

static void saveg_read_player_t(player_t* p) {
    int i;

    p->mo                   = saveg_read_mobjindex();
    p->playerstate          = saveg_read32();
    saveg_read_ticcmd_t(&p->cmd);
    p->viewz                = saveg_read32();
    p->viewheight           = saveg_read32();
    p->deltaviewheight      = saveg_read32();
    p->bob                  = saveg_read32();
    p->recoilpitch          = saveg_read32();
    p->health               = saveg_read32();
    p->armorpoints          = saveg_read32();
    p->armortype            = saveg_read32();

    for(i = 0; i < NUMPOWERS; i++) {
        p->powers[i]        = saveg_read32();
    }

    for(i = 0; i < NUMCARDS; i++) {
        p->cards[i]         = saveg_read32();
        p->tryopen[i]       = saveg_read32();
    }

    p->artifacts            = saveg_read32();
    p->backpack             = saveg_read32();

    for(i = 0; i < MAXPLAYERS; i++) {
        p->frags[i]         = saveg_read32();
    }

    p->readyweapon          = saveg_read32();
    p->pendingweapon        = saveg_read32();

    for(i = 0; i < NUMWEAPONS; i++) {
        p->weaponowned[i]   = saveg_read32();
    }

    for(i = 0; i < NUMAMMO; i++) {
        p->ammo[i]          = saveg_read32();
        p->maxammo[i]       = saveg_read32();
    }

    p->attackdown           = saveg_read32();
    p->usedown              = saveg_read32();
    p->jumpdown             = saveg_read32();
    p->cheats               = saveg_read32();
    p->refire               = saveg_read32();
    p->killcount            = saveg_read32();
    p->itemcount            = saveg_read32();
    p->secretcount          = saveg_read32();
    p->damagecount          = saveg_read32();
    p->bonuscount           = saveg_read32();
    p->bfgcount             = saveg_read32();
    p->attacker             = saveg_read_mobjindex();
    p->cameratarget         = saveg_read_mobjindex();

    if(!p->cameratarget) {
        p->cameratarget = p->mo;
    }

    for(i = 0; i < NUMPSPRITES; i++) {
        saveg_read_pspdef_t(&p->psprites[i]);
    }

    p->palette              = saveg_read32();
    p->onground             = saveg_read32();
    p->autoaim              = saveg_read32();
}

//
// ceiling_t
//

static void saveg_write_ceiling_t(ceiling_t* ceiling) {
    saveg_write32(ceiling->type);
    saveg_write32(ceiling->sector - sectors);
    saveg_write32(ceiling->bottomheight);
    saveg_write32(ceiling->topheight);
    saveg_write32(ceiling->speed);
    saveg_write32(ceiling->crush);
    saveg_write32(ceiling->direction);
    saveg_write32(ceiling->tag);
    saveg_write32(ceiling->olddirection);
    saveg_write32(ceiling->instant);
    saveg_write32(ceiling->thinker.function.acp1 != NULL ? 1 : 0);
}

static void saveg_read_ceiling_t(ceiling_t* ceiling) {
    ceiling->type           = saveg_read32();
    ceiling->sector         = &sectors[saveg_read32()];
    ceiling->bottomheight   = saveg_read32();
    ceiling->topheight      = saveg_read32();
    ceiling->speed          = saveg_read32();
    ceiling->crush          = saveg_read32();
    ceiling->direction      = saveg_read32();
    ceiling->tag            = saveg_read32();
    ceiling->olddirection   = saveg_read32();
    ceiling->instant        = saveg_read32();

    ceiling->sector->specialdata = ceiling;
}

//
// vldoor_t
//

static void saveg_write_vldoor_t(vldoor_t* door) {
    saveg_write32(door->type);
    saveg_write32(door->sector - sectors);
    saveg_write32(door->topheight);
    saveg_write32(door->bottomheight);
    saveg_write32(door->speed);
    saveg_write32(door->initceiling);
    saveg_write32(door->direction);
    saveg_write32(door->topwait);
    saveg_write32(door->topcountdown);
}

static void saveg_read_vldoor_t(vldoor_t* door) {
    door->type          = saveg_read32();
    door->sector        = &sectors[saveg_read32()];
    door->topheight     = saveg_read32();
    door->bottomheight  = saveg_read32();
    door->speed         = saveg_read32();
    door->initceiling   = saveg_read32();
    door->direction     = saveg_read32();
    door->topwait       = saveg_read32();
    door->topcountdown  = saveg_read32();

    door->sector->specialdata = door;
}

//
// floormove_t
//

static void saveg_write_floormove_t(floormove_t* floor) {
    saveg_write32(floor->type);
    saveg_write32(floor->crush);
    saveg_write32(floor->sector - sectors);
    saveg_write32(floor->direction);
    saveg_write32(floor->newspecial);
    saveg_write16(floor->texture);
    saveg_write32(floor->floordestheight);
    saveg_write32(floor->speed);
    saveg_write32(floor->instant);
}

static void saveg_read_floormove_t(floormove_t* floor) {
    floor->type             = saveg_read32();
    floor->crush            = saveg_read32();
    floor->sector           = &sectors[saveg_read32()];
    floor->direction        = saveg_read32();
    floor->newspecial       = saveg_read32();
    floor->texture          = saveg_read16();
    floor->floordestheight  = saveg_read32();
    floor->speed            = saveg_read32();
    floor->instant          = saveg_read32();

    floor->sector->specialdata = floor;
}

//
// splitmove_t
//

static void saveg_write_splitmove_t(splitmove_t* split) {
    saveg_write32(split->sector - sectors);
    saveg_write32(split->ceildest);
    saveg_write32(split->flrdest);
    saveg_write32(split->ceildir);
    saveg_write32(split->flrdir);
}

static void saveg_read_splitmove_t(splitmove_t* split) {
    split->sector   = &sectors[saveg_read32()];
    split->ceildest = saveg_read32();
    split->flrdest  = saveg_read32();
    split->ceildir  = saveg_read32();
    split->flrdir   = saveg_read32();

    split->sector->specialdata = split;
}

//
// plat_t
//

static void saveg_write_plat_t(plat_t* plat) {
    saveg_write32(plat->sector - sectors);
    saveg_write32(plat->speed);
    saveg_write32(plat->low);
    saveg_write32(plat->high);
    saveg_write32(plat->wait);
    saveg_write32(plat->count);
    saveg_write32(plat->status);
    saveg_write32(plat->oldstatus);
    saveg_write32(plat->crush);
    saveg_write32(plat->tag);
    saveg_write32(plat->type);
    saveg_write32(plat->thinker.function.acp1 != NULL ? 1 : 0);
}

static void saveg_read_plat_t(plat_t* plat) {
    plat->sector    = &sectors[saveg_read32()];
    plat->speed     = saveg_read32();
    plat->low       = saveg_read32();
    plat->high      = saveg_read32();
    plat->wait      = saveg_read32();
    plat->count     = saveg_read32();
    plat->status    = saveg_read32();
    plat->oldstatus = saveg_read32();
    plat->crush     = saveg_read32();
    plat->tag       = saveg_read32();
    plat->type      = saveg_read32();

    plat->sector->specialdata = plat;
}

//
// lightflash_t
//

static void saveg_write_lightflash_t(lightflash_t* lf) {
    saveg_write32(lf->sector - sectors);
    saveg_write32(lf->count);
    saveg_write32(lf->special);
}

static void saveg_read_lightflash_t(lightflash_t* lf) {
    lf->sector  = &sectors[saveg_read32()];
    lf->count   = saveg_read32();
    lf->special = saveg_read32();
}

//
// strobe_t
//

static void saveg_write_strobe_t(strobe_t* strobe) {
    saveg_write32(strobe->sector - sectors);
    saveg_write32(strobe->count);
    saveg_write32(strobe->maxlight);
    saveg_write32(strobe->darktime);
    saveg_write32(strobe->brighttime);
    saveg_write32(strobe->special);
}

static void saveg_read_strobe_t(strobe_t* strobe) {
    strobe->sector      = &sectors[saveg_read32()];
    strobe->count       = saveg_read32();
    strobe->maxlight    = saveg_read32();
    strobe->darktime    = saveg_read32();
    strobe->brighttime  = saveg_read32();
    strobe->special     = saveg_read32();
}

//
// glow_t
//

static void saveg_write_glow_t(glow_t* glow) {
    saveg_write32(glow->sector - sectors);
    saveg_write32(glow->type);
    saveg_write32(glow->count);
    saveg_write32(glow->minlight);
    saveg_write32(glow->direction);
    saveg_write32(glow->maxlight);
    saveg_write32(glow->special);
}

static void saveg_read_glow_t(glow_t* glow) {
    glow->sector    = &sectors[saveg_read32()];
    glow->type      = saveg_read32();
    glow->count     = saveg_read32();
    glow->minlight  = saveg_read32();
    glow->direction = saveg_read32();
    glow->maxlight  = saveg_read32();
    glow->special   = saveg_read32();
}

//
// fireflicker_t
//

static void saveg_write_fireflicker_t(fireflicker_t* ff) {
    saveg_write32(ff->sector - sectors);
    saveg_write32(ff->count);
    saveg_write32(ff->special);
}

static void saveg_read_fireflicker_t(fireflicker_t* ff) {
    ff->sector  = &sectors[saveg_read32()];
    ff->count   = saveg_read32();
    ff->special = saveg_read32();
}

//
// sequenceGlow_t
//

static void saveg_write_sequenceGlow_t(sequenceGlow_t* seq) {
    saveg_write32(seq->sector - sectors);
    saveg_write32(seq->headsector ? (seq->headsector - sectors) + 1 : 0);
    saveg_write32(seq->count);
    saveg_write32(seq->start);
    saveg_write32(seq->index);
    saveg_write32(seq->special);
}

static void saveg_read_sequenceGlow_t(sequenceGlow_t* seq) {
    seq->sector         = &sectors[saveg_read32()];
    seq->headsector     = &sectors[saveg_read32() - 1];
    seq->count          = saveg_read32();
    seq->start          = saveg_read32();
    seq->index          = saveg_read32();
    seq->special        = saveg_read32();
}

//
// combine_t
//

static void saveg_write_combine_t(combine_t* combine) {
    saveg_write32(combine->sector - sectors);
}

static void saveg_read_combine_t(combine_t* combine) {
    combine->sector = &sectors[saveg_read32()];
}

//
// lightmorph_t
//

static void saveg_write_lightmorph_t(lightmorph_t* morph) {
    saveg_write32(morph->dest - lights);
    saveg_write32(morph->src - lights);
    saveg_write32(morph->r);
    saveg_write32(morph->g);
    saveg_write32(morph->b);
    saveg_write32(morph->inc);
}

static void saveg_read_lightmorph_t(lightmorph_t* morph) {
    morph->dest = &lights[saveg_read32()];
    morph->src  = &lights[saveg_read32()];
    morph->r    = saveg_read32();
    morph->g    = saveg_read32();
    morph->b    = saveg_read32();
    morph->inc  = saveg_read32();
}

//
// delay_t
//

static void saveg_write_delay_t(delay_t* delay) {
    saveg_write32(delay->tics);
    saveg_write32(delay->finishfunc ? 1 : 0);
}

static void saveg_read_delay_t(delay_t* delay) {
    int func;

    delay->tics         = saveg_read32();
    func                = saveg_read32();
    delay->finishfunc   = func ? G_CompleteLevel : NULL;
}

//
// aimcamera_t
//

static void saveg_write_aimcamera_t(aimcamera_t* cam) {
    saveg_write_mobjindex(cam->viewmobj);
    saveg_write32(cam->activator ? cam->activator - players + 1 : 0);
}

static void saveg_read_aimcamera_t(aimcamera_t* cam) {
    int pl;

    cam->viewmobj   = saveg_read_mobjindex();
    pl              = saveg_read32();
    cam->activator  = pl > 0 ? &players[pl - 1] : NULL;
}

//
// movecamera_t
//

static void saveg_write_movecamera_t(movecamera_t* cam) {
    saveg_write32(cam->x);
    saveg_write32(cam->y);
    saveg_write32(cam->z);
    saveg_write32(cam->slopex);
    saveg_write32(cam->slopey);
    saveg_write32(cam->slopez);
    saveg_write32(cam->player ? cam->player - players + 1 : 0);
    saveg_write32(cam->current);
    saveg_write32(cam->tic);
}

static void saveg_read_movecamera_t(movecamera_t* cam) {
    int pl;

    cam->x          = saveg_read32();
    cam->y          = saveg_read32();
    cam->z          = saveg_read32();
    cam->slopex     = saveg_read32();
    cam->slopey     = saveg_read32();
    cam->slopez     = saveg_read32();
    pl              = saveg_read32();
    cam->player     = pl > 0 ? &players[pl - 1] : NULL;
    cam->current    = saveg_read32();
    cam->tic        = saveg_read32();
}

//
// mobjfade_t
//

static void saveg_write_mobjfade_t(mobjfade_t* fade) {
    saveg_write_mobjindex(fade->mobj);
    saveg_write32(fade->amount);
    saveg_write32(fade->destAlpha);
    saveg_write32(fade->flagReserve);
}

static void saveg_read_mobjfade_t(mobjfade_t* fade) {
    saveg_set_mobjtarget(&fade->mobj, saveg_read_mobjindex());

    fade->amount        = saveg_read32();
    fade->destAlpha     = saveg_read32();
    fade->flagReserve   = saveg_read32();
}

//
// mobjexp_t
//

static void saveg_write_mobjexp_t(mobjexp_t* exp) {
    saveg_write32(exp->delay);
    saveg_write32(exp->lifetime);
    saveg_write32(exp->delaymax);
    saveg_write_mobjindex(exp->mobj);
}

static void saveg_read_mobjexp_t(mobjexp_t* exp) {
    exp->delay      = saveg_read32();
    exp->lifetime   = saveg_read32();
    exp->delaymax   = saveg_read32();

    saveg_set_mobjtarget(&exp->mobj, saveg_read_mobjindex());
}

//
// quake_t
//

static void saveg_write_quake_t(quake_t* quake) {
    saveg_write32(quake->tics);
}

static void saveg_read_quake_t(quake_t* quake) {
    quake->tics = saveg_read32();
}

//
// laserthinker_t
//

static void saveg_write_laserthinker_t(laserthinker_t* laserthinker) {
    laser_t* l;

    saveg_write_mobjindex(laserthinker->dest);

    for(l = laserthinker->laser; l != NULL; l = l->next) {
        saveg_write32(l->x1);
        saveg_write32(l->y1);
        saveg_write32(l->z1);
        saveg_write32(l->x2);
        saveg_write32(l->y2);
        saveg_write32(l->z2);
        saveg_write32(l->slopex);
        saveg_write32(l->slopey);
        saveg_write32(l->slopez);
        saveg_write32(l->dist);
        saveg_write32(l->distmax);
        saveg_write_mobjindex(l->marker);
        saveg_write32(l->angle);
        saveg_write32(l->next ? 1 : 0);
    }
}

static void saveg_read_laserthinker_t(laserthinker_t* laserthinker) {
    laser_t* l = NULL;
    int next;
    dboolean head = true;

    laserthinker->dest = saveg_read_mobjindex();

    while(1) {
        if(head) {
            head = false;
            laserthinker->laser = (laser_t*)Z_Calloc(sizeof(laser_t), PU_LEVSPEC, NULL);
            l = laserthinker->laser;
        }
        else {
            l->next = (laser_t*)Z_Calloc(sizeof(laser_t), PU_LEVSPEC, NULL);
            l = l->next;
        }

        l->x1       = saveg_read32();
        l->y1       = saveg_read32();
        l->z1       = saveg_read32();
        l->x2       = saveg_read32();
        l->y2       = saveg_read32();
        l->z2       = saveg_read32();
        l->slopex   = saveg_read32();
        l->slopey   = saveg_read32();
        l->slopez   = saveg_read32();
        l->dist     = saveg_read32();
        l->distmax  = saveg_read32();
        l->marker   = saveg_read_mobjindex();
        l->angle    = saveg_read32();

        l->marker->extradata = (laser_t*)l;

        next = saveg_read32();

        if(!next) {
            return;
        }
    }
}

//------------------------------------------------------------------------
//
// Header read/write functions
//
//------------------------------------------------------------------------

static char* saveg_gettime(void) {
    time_t clock;
    struct tm* lt;

    time(&clock);
    lt = localtime(&clock);
    return asctime(lt);
}

static void saveg_write_header(char *description) {
    int i;
    int size;
    char date[32];
    byte* tbn;

    for(i = 0; description[i] != '\0'; i++) {
        saveg_write8(description[i]);
    }

    for(; i < SAVESTRINGSIZE; i++) {
        saveg_write8(0);
    }

    sprintf(date, "%s", saveg_gettime());
    size = dstrlen(date);

    for(i = 0; i < size; i++) {
        saveg_write8(date[i]);
    }

    for(; i < 32; i++) {
        saveg_write8(0);
    }

    size = M_CacheThumbNail(&tbn);

    saveg_write32(size);

    for(i = 0; i < size; i++) {
        saveg_write8(tbn[i]);
    }

    Z_Free(tbn);

    for(i = 0; i < 16; i++) {
        saveg_write8(passwordData[i]);
    }

    saveg_write8(gameskill);
    saveg_write8(gamemap);
    saveg_write8(nextmap);
    saveg_write_pad();
    saveg_write16(globalint);

    //
    // [kex] 12/26/11 - write total stat info
    //
    saveg_write_pad();
    saveg_write32(totalkills);
    saveg_write32(totalitems);
    saveg_write32(totalsecret);

    for(i = 0; i < MAXPLAYERS; i++) {
        saveg_write8(playeringame[i]);
    }

    saveg_write8((leveltime >> 16) & 0xff);
    saveg_write8((leveltime >> 8) & 0xff);
    saveg_write8(leveltime & 0xff);
    saveg_write_pad();
}

static void saveg_read_header(void) {
    int i;
    int size;
    byte a, b, c;

    // skip the description field
    for(i = 0; i < SAVESTRINGSIZE; i++) {
        saveg_read8();
    }

    // skip the date
    for(i = 0; i < 32; i++) {
        saveg_read8();
    }

    size = saveg_read32() / sizeof(int);

    // skip the thumbnail
    for(i = 0; i < size; i++) {
        saveg_read32();
    }

    for(i = 0; i < 16; i++) {
        passwordData[i] = saveg_read8();
    }

    gameskill   = saveg_read8();
    gamemap     = saveg_read8();
    nextmap     = saveg_read8();

    saveg_read_pad();

    globalint   = saveg_read16();

    //
    // [kex] 12/26/11 - read total stat info
    //
    saveg_read_pad();

    totalkills  = saveg_read32();
    totalitems  = saveg_read32();
    totalsecret = saveg_read32();

    for(i = 0; i < MAXPLAYERS; i++) {
        playeringame[i] = saveg_read8();
    }

    // get the times
    a = saveg_read8();
    b = saveg_read8();
    c = saveg_read8();

    leveltime = (a<<16) + (b<<8) + c;

    saveg_read_pad();
}

//------------------------------------------------------------------------
//
// Read/write consistency marker
//
//------------------------------------------------------------------------

static dboolean saveg_read_marker(int marker) {
    int value;

    saveg_read_pad();
    value = saveg_read32();

    return value == marker;
}

static void saveg_write_marker(int marker) {
    saveg_write_pad();
    saveg_write32(marker);
}

//
// P_WriteSaveGame
//

dboolean P_WriteSaveGame(char* description, int slot) {
    //char name[256];

    // setup game save file
    // sprintf(name, SAVEGAMENAME"%d.dsg", slot);
    save_stream = fopen(P_GetSaveGameName(slot), "wb");

    // success?
    if(save_stream == NULL) {
        return false;
    }

    save_offset = 0;

    saveg_write_header(description);

    P_ArchiveMobjs();
    P_ArchivePlayers();
    P_ArchiveWorld();
    P_ArchiveSpecials();
    P_ArchiveMacros();

    saveg_write_marker(SAVEGAME_EOF);

    // close out file
    fclose(save_stream);

    return true;
}

//
// P_ReadSaveGame
//

dboolean P_ReadSaveGame(char* name) {
    M_ReadFile(name, &savebuffer);
    save_offset = 0;

    saveg_read_header();

    // load a base level
    G_InitNew(gameskill, gamemap);
    G_DoLoadLevel();

    P_UnArchiveMobjs();
    P_UnArchivePlayers();
    P_UnArchiveWorld();
    P_UnArchiveSpecials();
    P_UnArchiveMacros();

    if(!saveg_read_marker(SAVEGAME_EOF)) {
        I_Error("Bad savegame");
    }

    Z_Free(savebuffer);

    return true;
}

//
// P_QuickReadSaveHeader
//

dboolean P_QuickReadSaveHeader(char* name, char* date,
                               int* thumbnail, int* skill, int* map) {
    int i;
    int size;

    if(M_ReadFile(name, &savebuffer) == -1) {
        return 0;
    }

    save_offset = 0;

    // skip the description field
    for(i = 0; i < SAVESTRINGSIZE; i++) {
        saveg_read8();
    }

    for(i = 0; i < 32; i++) {
        date[i] = saveg_read8();
    }

    size = saveg_read32() / sizeof(int);

    for(i = 0; i < size; i++) {
        thumbnail[i] = saveg_read32();
    }

    // skip password
    for(i = 0; i < 16; i++) {
        saveg_read8();
    }

    *skill  = saveg_read8();
    *map    = saveg_read8();

    Z_Free(savebuffer);

    return 1;
}


//
// P_ArchivePlayers
//
void P_ArchivePlayers(void) {
    int i;

    for(i = 0; i < MAXPLAYERS; i++) {
        if(!playeringame[i]) {
            continue;
        }

        saveg_write_pad();
        saveg_write_player_t(&players[i]);
    }
}

//
// P_UnArchivePlayers
//
void P_UnArchivePlayers(void) {
    int i;

    for(i = 0; i < MAXPLAYERS; i++) {
        if(!playeringame[i]) {
            continue;
        }

        saveg_read_pad();
        saveg_read_player_t(&players[i]);
    }
}


//
// P_ArchiveWorld
//
void P_ArchiveWorld(void) {
    int         i;
    int         j;
    sector_t    *sec;
    line_t      *li;
    side_t      *si;
    light_t     *light;

    // do sectors
    for(i = 0, sec = sectors; i < numsectors; i++, sec++) {
        saveg_write16(F2INT(sec->floorheight));
        saveg_write16(F2INT(sec->ceilingheight));
        saveg_write16(sec->floorpic);
        saveg_write16(sec->ceilingpic);
        saveg_write16(sec->special);
        saveg_write16(sec->tag);
        saveg_write16(sec->flags);
        saveg_write16(sec->lightlevel);
        saveg_write32(sec->xoffset);
        saveg_write32(sec->yoffset);
        saveg_write_mobjindex(sec->soundtarget);

        for(j = 0; j < 5; j++) {
            saveg_write16(sec->colors[j]);
        }
    }


    // do lines
    for(i = 0, li = lines; i < numlines; i++, li++) {
        saveg_write16((li->flags >> 16));
        saveg_write16((li->flags & 0xFFFF));
        saveg_write16(li->special);
        saveg_write16(li->tag);

        for(j = 0; j < 2; j++) {
            if(li->sidenum[j] == NO_SIDE_INDEX) {
                continue;
            }

            si = &sides[li->sidenum[j]];

            saveg_write16(F2INT(si->textureoffset));
            saveg_write16(F2INT(si->rowoffset));
            saveg_write16(si->toptexture);
            saveg_write16(si->bottomtexture);
            saveg_write16(si->midtexture);
        }
    }

    // do lights
    for(i = 0, light = lights; i < numlights; i++, light++) {
        saveg_write8(light->base_r);
        saveg_write8(light->base_g);
        saveg_write8(light->base_b);
        saveg_write8(light->active_r);
        saveg_write8(light->active_g);
        saveg_write8(light->active_b);
        saveg_write8(light->r);
        saveg_write8(light->g);
        saveg_write8(light->b);
        saveg_write_pad();
        saveg_write16(light->tag);
    }
}

//
// P_UnArchiveWorld
//
void P_UnArchiveWorld(void) {
    int         i;
    int         j;
    sector_t    *sec;
    line_t      *li;
    light_t     *light;
    side_t      *si;

    // do sectors
    for(i = 0, sec = sectors; i < numsectors; i++, sec++) {
        sec->floorheight    = INT2F(saveg_read16());
        sec->ceilingheight  = INT2F(saveg_read16());
        sec->floorpic       = saveg_read16();
        sec->ceilingpic     = saveg_read16();
        sec->special        = saveg_read16();
        sec->tag            = saveg_read16();
        sec->flags          = saveg_read16();
        sec->lightlevel     = saveg_read16();
        sec->xoffset        = saveg_read32();
        sec->yoffset        = saveg_read32();

        saveg_set_mobjtarget(&sec->soundtarget, saveg_read_mobjindex());

        for(j = 0; j < 5; j++) {
            sec->colors[j]  = saveg_read16();
        }

        sec->specialdata    = 0;
    }

    // do lines
    for(i = 0, li = lines; i < numlines; i++, li++) {
        li->flags           = (saveg_read16() << 16);
        li->flags           = li->flags | (saveg_read16() & 0xFFFF);
        li->special         = saveg_read16();
        li->tag             = saveg_read16();

        for(j = 0; j < 2; j++) {
            if(li->sidenum[j] == NO_SIDE_INDEX) {
                continue;
            }

            si                  = &sides[li->sidenum[j]];
            si->textureoffset   = INT2F(saveg_read16());
            si->rowoffset       = INT2F(saveg_read16());
            si->toptexture      = saveg_read16();
            si->bottomtexture   = saveg_read16();
            si->midtexture      = saveg_read16();
        }
    }

    // do lights
    for(i = 0, light = lights; i < numlights; i++, light++) {
        light->base_r       = saveg_read8();
        light->base_g       = saveg_read8();
        light->base_b       = saveg_read8();
        light->active_r     = saveg_read8();
        light->active_g     = saveg_read8();
        light->active_b     = saveg_read8();
        light->r            = saveg_read8();
        light->g            = saveg_read8();
        light->b            = saveg_read8();
        saveg_read_pad();
        light->tag          = saveg_read16();
    }
}


//
// P_ArchiveMobjs
//

void P_ArchiveMobjs(void) {
    mobj_t*    mobj;

    saveg_setup_mobjwrite();
    saveg_write32(savegmobjnum);

    // save off the current mobjs
    for(mobj = mobjhead.next; mobj != &mobjhead; mobj = mobj->next) {
        // don't save if mobj is expected to be removed
        if(mobj->mobjfunc == P_SafeRemoveMobj) {
            continue;
        }

        saveg_write_pad();
        saveg_write_mobj_t(mobj);
        saveg_write_marker(SAVEGAME_MOBJ);
    }

    saveg_write_pad();
}

//
// P_UnArchiveMobjs
//

void P_UnArchiveMobjs(void) {
    mobj_t* current;
    mobj_t* next;
    mobj_t* mobj;
    int     i;

    // remove all the current thinkers
    current = mobjhead.next;
    while(current != &mobjhead) {
        next = current->next;
        P_RemoveMobj(current);

        current = next;
    }

    saveg_setup_mobjread();
    mobjhead.next = mobjhead.prev = &mobjhead;

    for(i = 0; i < savegmobjnum; i++) {
        mobj = savegmobj[i].mobj;

        saveg_read_pad();
        saveg_read_mobj_t(mobj);

        if(!saveg_read_marker(SAVEGAME_MOBJ))
            I_Error("P_UnArchiveMobjs: Mobj read is inconsistent\nfile offset: %i\nmobj count: %i",
                    save_offset, savegmobjnum);

        P_SetThingPosition(mobj);
        P_LinkMobj(mobj);

        mobj->info = &mobjinfo[mobj->type];
    }

    saveg_read_pad();
}


enum {
    tc_ceiling,
    tc_door,
    tc_floor,
    tc_plat,
    tc_flash,
    tc_strobe,
    tc_glow,
    tc_flicker,
    tc_delay,
    tc_aimcam,
    tc_movecam,
    tc_fade,
    tc_sequence,
    tc_quake,
    tc_combine,
    tc_laser,
    tc_split,
    tc_morph,
    tc_exp,
    tc_endthinkers
} specials_e;

struct {
    think_t function;
    int     type;
    void (*writefunc)(void* th);
    void (*readfunc)(void* th);
    int     structsize;
} saveg_specials[] = {
    {
        T_MoveCeiling,
        tc_ceiling,
        saveg_write_ceiling_t,
        saveg_read_ceiling_t,
        sizeof(ceiling_t)
    },

    {
        T_VerticalDoor,
        tc_door,
        saveg_write_vldoor_t,
        saveg_read_vldoor_t,
        sizeof(vldoor_t)
    },

    {
        T_MoveFloor,
        tc_floor,
        saveg_write_floormove_t,
        saveg_read_floormove_t,
        sizeof(floormove_t)
    },

    {
        T_PlatRaise,
        tc_plat,
        saveg_write_plat_t,
        saveg_read_plat_t,
        sizeof(plat_t)
    },

    {
        T_LightFlash,
        tc_flash,
        saveg_write_lightflash_t,
        saveg_read_lightflash_t,
        sizeof(lightflash_t)
    },

    {
        T_StrobeFlash,
        tc_strobe,
        saveg_write_strobe_t,
        saveg_read_strobe_t,
        sizeof(strobe_t)
    },

    {
        T_Glow,
        tc_glow,
        saveg_write_glow_t,
        saveg_read_glow_t,
        sizeof(glow_t)
    },

    {
        T_FireFlicker,
        tc_flicker,
        saveg_write_fireflicker_t,
        saveg_read_fireflicker_t,
        sizeof(fireflicker_t)
    },

    {
        T_CountdownTimer,
        tc_delay,
        saveg_write_delay_t,
        saveg_read_delay_t,
        sizeof(delay_t)
    },

    {
        T_LookAtCamera,
        tc_aimcam,
        saveg_write_aimcamera_t,
        saveg_read_aimcamera_t,
        sizeof(aimcamera_t)
    },

    {
        T_MovingCamera,
        tc_movecam,
        saveg_write_movecamera_t,
        saveg_read_movecamera_t,
        sizeof(movecamera_t)
    },

    {
        T_MobjFadeThinker,
        tc_fade,
        saveg_write_mobjfade_t,
        saveg_read_mobjfade_t,
        sizeof(mobjfade_t)
    },

    {
        T_Sequence,
        tc_sequence,
        saveg_write_sequenceGlow_t,
        saveg_read_sequenceGlow_t,
        sizeof(sequenceGlow_t)
    },

    {
        T_Quake,
        tc_quake,
        saveg_write_quake_t,
        saveg_read_quake_t,
        sizeof(quake_t)
    },

    {
        T_Combine,
        tc_combine,
        saveg_write_combine_t,
        saveg_read_combine_t,
        sizeof(combine_t)
    },

    {
        T_LaserThinker,
        tc_laser,
        saveg_write_laserthinker_t,
        saveg_read_laserthinker_t,
        sizeof(laserthinker_t)
    },

    {
        T_MoveSplitPlane,
        tc_split,
        saveg_write_splitmove_t,
        saveg_read_splitmove_t,
        sizeof(splitmove_t)
    },

    {
        T_LightMorph,
        tc_morph,
        saveg_write_lightmorph_t,
        saveg_read_lightmorph_t,
        sizeof(lightmorph_t)
    },

    {
        T_MobjExplode,
        tc_exp,saveg_write_mobjexp_t,
        saveg_read_mobjexp_t,
        sizeof(mobjexp_t)
    },

    {
        NULL,
        tc_endthinkers,
        NULL,
        NULL,
        0
    }
};

//
// P_ArchiveSpecials
//

void P_ArchiveSpecials(void) {
    thinker_t*  th;
    int         i;

    for(th = thinkercap.next; th != &thinkercap; th = th->next) {
        if(th->function.acv == (actionf_v)NULL) {
            for(i = 0; i < MAXCEILINGS; i++) {
                if(activeceilings[i] == (ceiling_t *)th) {
                    break;
                }
            }

            if(i < MAXCEILINGS) {
                saveg_write8(tc_ceiling);
                saveg_write_pad();
                saveg_write_ceiling_t((ceiling_t *)th);
            }

            continue;
        }

        for(i = 0; saveg_specials[i].type != tc_endthinkers; i++) {
            if(th->function.acp1 == (actionf_p1)saveg_specials[i].function.acp1) {
                saveg_write8(saveg_specials[i].type);
                saveg_write_pad();
                saveg_specials[i].writefunc(th);
                saveg_write32(th == macrothinker ? 1 : 0);
                break;
            }
        }
    }

    // add a terminating marker
    saveg_write8(tc_endthinkers);
}

//
// P_UnArchiveSpecials
//

void P_UnArchiveSpecials(void) {
    int         i;
    dboolean    specialthinker;
    byte        tclass;
    void*       thinker;
    thinker_t*  currentthinker;
    thinker_t*  next;

    // remove all the current thinkers
    currentthinker = thinkercap.next;
    while(currentthinker != &thinkercap) {
        next = currentthinker->next;
        Z_Free(currentthinker);

        currentthinker = next;
    }

    thinkercap.prev = thinkercap.next  = &thinkercap;

    while(1) {
        tclass = saveg_read8();

        if(tclass == tc_endthinkers) {
            return;
        }

        if(tclass > tc_endthinkers) {
            I_Error("P_UnarchiveSpecials: Unknown tclass %i in savegame", tclass);
        }

        for(i = 0; saveg_specials[i].type != tc_endthinkers; i++) {
            if(tclass == saveg_specials[i].type) {
                saveg_read_pad();
                thinker = Z_Malloc(saveg_specials[i].structsize, PU_LEVEL, NULL);
                saveg_specials[i].readfunc(thinker);

                ((thinker_t*)thinker)->function.acp1 = (actionf_p1)saveg_specials[i].function.acp1;
                P_AddThinker(thinker);

                // handle special cases
                switch(tclass) {
                case tc_ceiling:
                    specialthinker = saveg_read32();
                    if(!specialthinker) {
                        ((thinker_t*)thinker)->function.acp1 = NULL;
                    }

                    P_AddActiveCeiling(thinker);
                    break;

                case tc_plat:
                    specialthinker = saveg_read32();
                    if(!specialthinker) {
                        ((thinker_t*)thinker)->function.acp1 = NULL;
                    }

                    P_AddActivePlat(thinker);
                    break;

                case tc_combine:
                    P_CombineLightSpecials(((combine_t*)thinker)->sector);
                    P_RemoveThinker(thinker);
                    break;
                }

                if(((thinker_t*)thinker)->function.acp1 != NULL) {
                    specialthinker = saveg_read32();
                    if(specialthinker) {
                        macrothinker = (thinker_t*)thinker;
                    }
                }

                break;
            }
        }
    }
}


//
// P_ArchiveMacros
//

void P_ArchiveMacros(void) {
    int i;

    saveg_write_pad();

    // [kex] 12/26/11 - keep track of disabled macros
    for(i = 0; i < macros.macrocount; i++) {
        saveg_write8(macros.def[i].data[0].id == 0 ? 1 : 0);
    }

    if(!macro) {
        saveg_write8(0);
        return;
    }

    saveg_write8(1);
    saveg_write16(macroid);
    saveg_write16(macrocounter);
    saveg_write16(nextmacro - macro->data);
    saveg_write_mobjindex(mobjmacro);
    saveg_write16(taglistidx);

    for(i = 0; i < MAXQUEUELIST; i++) {
        saveg_write16(taglist[i]);
    }
}

//
// P_UnArchiveMacros
//

void P_UnArchiveMacros(void) {
    int i;
    int current;
    byte havemacro;

    saveg_read_pad();

    // [kex] 12/26/11 - read tracked info for disabled macros
    for(i = 0; i < macros.macrocount; i++) {
        if(saveg_read8()) {
            macros.def[i].data[0].id = 0;
        }
    }

    havemacro = saveg_read8();

    if(!havemacro) {
        return;
    }

    macroid         = saveg_read16();
    macrocounter    = saveg_read16();
    current         = saveg_read16();
    mobjmacro       = saveg_read_mobjindex();
    taglistidx      = saveg_read16();

    macro           = &macros.def[macroid];
    nextmacro       = &macro->data[current];

    for(i = 0; i < MAXQUEUELIST; i++) {
        taglist[i] = saveg_read16();
    }
}


