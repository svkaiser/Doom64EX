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
//    Implements special effects:
//    Texture animation, height or lighting changes
//     according to adjacent sectors, respective
//     utility functions, etc.
//    Line Tag handling. Line and Sector triggers.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>

#include "doomdef.h"
#include "doomstat.h"
#include "i_system.h"
#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"
#include "p_macros.h"
#include "m_fixed.h"
#include "tables.h"
#include "p_local.h"
#include "g_game.h"
#include "s_sound.h"
#include "d_englsh.h"
#include "r_local.h"
#include "sounds.h"
#include "gl_texture.h"
#include "m_misc.h"
#include "con_console.h"
#include "r_sky.h"
#include "sc_main.h"

CVAR_EXTERNAL(p_features);

short globalint = 0;
static byte tryopentype[3];

//
//      source animation definition
//

typedef struct {
    dboolean isreverse;
    int delay;
    int texnum;
    int tic;
    int frame;
} animinfo_t;

//
// ANIMPIC DEFINITIONS
//

int         numanimdef;
animdef_t*  animdefs;

static scdatatable_t animdatatable[] = {
    {   "RESTARTDELAY", (int64)&((animdef_t*)0)->delay,   'i' },
    {   "FRAMES", (int64)&((animdef_t*)0)->frames,  'i' },
    {   "CYCLEPALETTES", (int64)&((animdef_t*)0)->palette, 'b' },
    {   "REWIND", (int64)&((animdef_t*)0)->reverse, 'b' },
    {   "SPEED", (int64)&((animdef_t*)0)->speed,   'i' },
    {   NULL,               0,                              0   }
};

//
// P_InitAnimdef
//

static void P_InitAnimdef(void) {
    animdef_t anim;

    numanimdef = 0;
    animdefs = NULL;

    sc_parser.open("ANIMDEFS");

    while(sc_parser.readtokens()) {
        sc_parser.find(false);

        //
        // find animpic block
        //
        if(!dstricmp(sc_parser.token, "ANIMPIC")) {
            dmemset(&anim, 0, sizeof(animdef_t));

            sc_parser.find(false);
            dstrncpy(anim.name, sc_parser.token, dstrlen(sc_parser.token));

            sc_parser.compare("{");  // must expect open bracket

            //
            // read into block
            //
            while(sc_parser.readtokens()) {
                sc_parser.find(false);

                if(sc_parser.token[0] == '}') { // exit block if closed bracket is found
                    break;
                }

                if(!sc_parser.setdata(&anim, animdatatable)) {
                    sc_parser.error("P_InitAnimdef");
                }
            }

            animdefs = Z_Realloc(animdefs,
                                 sizeof(animdef_t) * ++numanimdef, PU_STATIC, 0);
            dmemcpy(&animdefs[numanimdef - 1], &anim, sizeof(animdef_t));
        }
        else {
            sc_parser.error("P_InitAnimdef");
        }
    }

    sc_parser.close();
}

//
// P_InitPicAnims
//

// Floor/ceiling animation sequences,
//  defined by first and last frame,
//  i.e. the flat (64x64 tile) name to
//  be used.
// The full animation sequence is given
//  using all the flats between the start
//  and end entry, in the order found in
//  the WAD file.
//

animinfo_t* animinfo;

//
//      Animating line specials
//

extern line_t** linespeciallist;

//
// P_InitPicAnims
//
void P_InitPicAnims(void) {
    int    i = 0;

    P_InitAnimdef();

    animinfo = (animinfo_t*)Z_Malloc(sizeof(animinfo_t) * numanimdef, PU_STATIC, 0);

    //    Init animation
    for(i = 0; i < numanimdef; i++) {
        animinfo[i].delay = 0;
        animinfo[i].tic = 0;
        animinfo[i].isreverse = false;
        animinfo[i].texnum = W_GetNumForName(animdefs[i].name) - t_start;
        animinfo[i].frame = -1;

        // reallocate texture pointers if they contain multiple palettes
        // check by looking up animdefs

        if(animdefs[i].palette) {
            int lump = animinfo[i].texnum;

            textureptr[lump] = (dtexture*)Z_Realloc(textureptr[lump],
                                                    animdefs[i].frames * sizeof(dtexture), PU_STATIC, 0);
        }
    }
}

//
// P_CyclePicAnims
//

void P_CyclePicAnims(void) {
    animdef_t* anim = NULL;
    animinfo_t* info = NULL;
    int i = 0;
    int lastpic = 0;

    for(i = 0; i < numanimdef; i++) {
        anim = &animdefs[i];
        info = &animinfo[i];

        if(info->delay) {
            info->delay--;
            continue;
        }

        if(leveltime <= info->tic) {
            continue;
        }

        if(info->isreverse) {
            lastpic = 0;
        }
        else {
            lastpic = (anim->frames - 1);
        }

        info->tic = leveltime + anim->speed;

        if(info->isreverse) {
            info->frame--;
        }
        else {
            info->frame++;
        }

        if(anim->palette) {
            GL_SetNewPalette(info->texnum, info->frame);
        }
        else {
            texturetranslation[info->texnum] = info->texnum + info->frame;
        }

        if(info->frame == lastpic) {
            if(anim->delay) {
                info->delay = anim->delay;
            }

            if(anim->reverse) {
                info->isreverse ^= 1;
            }
            else {
                info->frame = -1;
            }
        }
    }
}


//
// UTILITIES
//



//
// getSide()
// Will return a side_t*
//  given the number of the current sector,
//  the line number, and the side (0/1) that you want.
//
side_t* getSide(int currentSector, int line, int side) {
    return &sides[(sectors[currentSector].lines[line])->sidenum[side]];
}


//
// getSector()
// Will return a sector_t*
//  given the number of the current sector,
//  the line number and the side (0/1) that you want.
//
sector_t* getSector(int currentSector, int line, int side) {
    return sides[(sectors[currentSector].lines[line])->sidenum[side]].sector;
}


//
// twoSided()
// Given the sector number and the line number,
//  it will tell you whether the line is two-sided or not.
//
int twoSided(int sector, int line) {
    return(sectors[sector].lines[line])->flags & ML_TWOSIDED;
}




//
// getNextSector()
// Return sector_t * of sector next to current.
// NULL if not two-sided line
//
sector_t* getNextSector(line_t* line, sector_t* sec) {
    if(!(line->flags & ML_TWOSIDED)) {
        return NULL;
    }

    if(line->frontsector == sec) {
        return line->backsector;
    }

    return line->frontsector;
}



//
// P_FindLowestFloorSurrounding()
// FIND LOWEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t    P_FindLowestFloorSurrounding(sector_t* sec) {
    int            i;
    line_t*        check;
    sector_t*      other;
    fixed_t        floor = sec->floorheight;

    for(i=0 ; i < sec->linecount ; i++) {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if(!other) {
            continue;
        }

        if(other->floorheight < floor) {
            floor = other->floorheight;
        }
    }
    return floor;
}



//
// P_FindHighestFloorSurrounding()
// FIND HIGHEST FLOOR HEIGHT IN SURROUNDING SECTORS
//
fixed_t    P_FindHighestFloorSurrounding(sector_t *sec) {
    int            i;
    line_t*        check;
    sector_t*      other;
    fixed_t        floor = -500*FRACUNIT;

    for(i=0 ; i < sec->linecount ; i++) {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if(!other) {
            continue;
        }

        if(other->floorheight > floor) {
            floor = other->floorheight;
        }
    }
    return floor;
}



//
// P_FindNextHighestFloor
// FIND NEXT HIGHEST FLOOR IN SURROUNDING SECTORS
// Note: this should be doable w/o a fixed array.

// 20 adjoining sectors max!
#define MAX_ADJOINING_SECTORS        20

fixed_t P_FindNextHighestFloor(sector_t* sec, int currentheight) {
    int         i;
    int         h;
    int         min;
    line_t*     check;
    sector_t*   other;
    fixed_t     height = currentheight;
    fixed_t     heightlist[MAX_ADJOINING_SECTORS];

    for(i = 0, h = 0; i < sec->linecount; i++) {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if(!other) {
            continue;
        }

        if(other->floorheight > height) {
            heightlist[h++] = other->floorheight;
        }

        // Check for overflow. Exit.
        if(h >= MAX_ADJOINING_SECTORS) {
            CON_Warnf("Sector with more than 20 adjoining sectors\n");
            break;
        }
    }

    // Find lowest height in list
    if(!h) {
        return currentheight;
    }

    min = heightlist[0];

    // Range checking?
    for(i = 1; i < h; i++)
        if(heightlist[i] < min) {
            min = heightlist[i];
        }

    return min;
}


//
// FIND LOWEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t P_FindLowestCeilingSurrounding(sector_t* sec) {
    int         i;
    line_t*     check;
    sector_t*   other;
    fixed_t     height = D_MAXINT;

    for(i = 0; i < sec->linecount; i++) {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if(!other) {
            continue;
        }

        if(other->ceilingheight < height) {
            height = other->ceilingheight;
        }
    }
    return height;
}


//
// FIND HIGHEST CEILING IN THE SURROUNDING SECTORS
//
fixed_t    P_FindHighestCeilingSurrounding(sector_t* sec) {
    int         i;
    line_t*     check;
    sector_t*   other;
    fixed_t     height = 0;

    for(i = 0; i < sec->linecount; i++) {
        check = sec->lines[i];
        other = getNextSector(check,sec);

        if(!other) {
            continue;
        }

        if(other->ceilingheight > height) {
            height = other->ceilingheight;
        }
    }
    return height;
}



//
// P_FindSectorFromLineTag
// RETURN NEXT SECTOR # THAT LINE TAG REFERS TO
//

int P_FindSectorFromLineTag(line_t* line, int start) {
    int    i;

    for(i = start + 1; i < numsectors; i++) {
        if(sectors[i].tag == line->tag) {
            return i;
        }
    }
    return -1;
}


//
// P_FindLinedefFromTag
//

int P_FindLinedefFromTag(int tag) {
    int i = 0;

    for(i = 0; i < numlines; i++) {
        if(lines[i].tag == tag) {
            return i;
        }
    }

    return -1;
}

//
// P_FindSectorFromTag
// Simplier version of P_FindSectorFromLineTag
//

int P_FindSectorFromTag(int tag) {
    int i = 0;

    for(i = 0; i < numsectors; i++) {
        if(sectors[i].tag == tag) {
            return i;
        }
    }

    return -1;
}

//
// P_ActivateLineByTag
//

dboolean P_ActivateLineByTag(int tag, mobj_t* activator) {
    int i;

    for(i = 0; i < numlines; i++) {
        if(lines[i].tag == tag) {
            return P_UseSpecialLine(activator, &lines[i], 0);
        }
    }

    return 1;
}

//
// P_ModifyLine
//

typedef enum {
    modl_flags,
    modl_texture,
    modl_data
} modifyline_t;

static int P_ModifyLine(int tag1, int tag2, int type) {
    int i = 0;
    line_t* line1;
    line_t* line2;
    int linenum;
    
    linenum = P_FindLinedefFromTag(tag2);
    if(linenum == -1) {
        return 0;
    }
    
    line2 = &lines[linenum];

    for(i = 0; i < numlines; i++) {
        line1 = &lines[i];
        if(line1->tag == tag1) {
            switch(type) {
            case modl_flags:
                if(line1->flags & ML_TWOSIDED) {
                    line1->flags = (line2->flags | ML_TWOSIDED);
                }
                else {
                    line1->flags = line2->flags;
                    line1->flags &= ~ML_TWOSIDED;
                }
                break;
            case modl_texture:
                sides[line1->sidenum[0]].bottomtexture = sides[line2->sidenum[0]].bottomtexture;
                sides[line1->sidenum[0]].midtexture = sides[line2->sidenum[0]].midtexture;
                sides[line1->sidenum[0]].toptexture = sides[line2->sidenum[0]].toptexture;

                if(line1->flags & ML_TWOSIDED || line1->sidenum[1] != NO_SIDE_INDEX) {
                    sides[line1->sidenum[1]].bottomtexture = sides[line2->sidenum[1]].bottomtexture;
                    sides[line1->sidenum[1]].midtexture = sides[line2->sidenum[1]].midtexture;
                    sides[line1->sidenum[1]].toptexture = sides[line2->sidenum[1]].toptexture;
                }

                if(line1->flags & ML_SWITCHX02 &&
                        !sides[line1->sidenum[0]].toptexture) {
                    line1->flags &= ~ML_SWITCHX02;
                }

                if(line1->flags & (ML_SWITCHX04 | ML_SWITCHX08) &&
                        !sides[line1->sidenum[0]].bottomtexture) {
                    line1->flags &= ~(ML_SWITCHX04 | ML_SWITCHX08);
                }

                if(line1->flags & (ML_SWITCHX02 | ML_SWITCHX04) &&
                        !sides[line1->sidenum[0]].midtexture) {
                    line1->flags &= ~(ML_SWITCHX02 | ML_SWITCHX04);
                }

                if(line1->flags & (ML_SWITCHX02 | ML_SWITCHX08) &&
                        !sides[line1->sidenum[0]].toptexture) {
                    line1->flags &= ~(ML_SWITCHX02 | ML_SWITCHX08);
                }

                break;
            case modl_data:
                line1->special = line2->special;
                break;
            default:
                break;
            }
        }
    }

    return 1;
}

//
// P_ModifySector
//

typedef enum {
    mods_lights,
    mods_flats,
    mods_special,
    mods_flags,
} modifysector_t;

static int P_ModifySector(line_t *line, int tag, int type) {
    int j = 0;
    sector_t* sec1;
    sector_t* sec2;
    int secnum;
    int rtn;

    secnum = P_FindSectorFromTag(tag);

    if(secnum == -1) {
        return 0;
    }

    sec2 = &sectors[secnum];
    
    secnum = -1;
    rtn = 0;

    while((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0) {
        sec1 = &sectors[secnum];
        rtn = 1;

        switch(type) {
            case mods_lights:
                for(j = 0; j < 5; j++) {
                    sec1->colors[j] = sec2->colors[j];
                }
                break;
            case mods_flats:
                sec1->ceilingpic = sec2->ceilingpic;
                sec1->floorpic = sec2->floorpic;
                break;
            case mods_special:
                sec1->special = sec2->special;
                P_AddSectorSpecial(sec1);
                break;
            case mods_flags:
                sec1->flags = sec2->flags;
                break;
            default:
                break;
        }
    }

    return rtn;
}

//
// P_ModifySectorColor
//
// Changes a sector light index
// This doesn't appear to be used at all
//

int P_ModifySectorColor(line_t* line, int index, int type) {
    int secnum;
    int rtn;
    sector_t* sec;

    secnum = -1;
    rtn = 0;

    while((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0) {
        sec = &sectors[secnum];
        rtn = 1;

        switch(type) {
            case LIGHT_FLOOR:
                sec->colors[LIGHT_FLOOR] = index;
                break;
            case LIGHT_CEILING:
                sec->colors[LIGHT_CEILING] = index;
                break;
            case LIGHT_THING:
                sec->colors[LIGHT_THING] = index;
                break;
            case LIGHT_UPRWALL:
                sec->colors[LIGHT_UPRWALL] = index;
                break;
            case LIGHT_LWRWALL:
                sec->colors[LIGHT_LWRWALL] = index;
                break;
        }
    }
    
    return rtn;
}

//
// P_RandomLineTrigger
//

int P_RandomLineTrigger(line_t* line, mobj_t* thing, int side) {
    line_t** randLine = NULL;
    line_t** linelist = NULL;
    int i = 0;
    int count = 0;

    for(i = 0; i < numlines; i++) {
        if(lines[i].tag == line->tag &&
                SPECIALMASK(lines[i].special) != SPECIALMASK(line->special)) {
            count++;
        }
    }
    if(!count) {
        return 0;
    }

    linelist = (line_t **)Z_Malloc(count*sizeof(line_t *), PU_LEVEL, NULL);
    randLine = linelist;

    for(i = 0; i < numlines; i++) {
        if(lines[i].tag == line->tag &&
                SPECIALMASK(lines[i].special) != SPECIALMASK(line->special)) {
            *randLine++ = &lines[i];
        }
    }

    P_UseSpecialLine(thing, linelist[(P_Random(pr_randomtrigger)%count)], side);
    Z_Free(linelist);

    return 1;
}

//
// T_CountdownTimer
//

void T_CountdownTimer(delay_t* timer) {
    if(!timer->tics--) {
        if(timer->finishfunc) {
            timer->finishfunc();
        }

        P_RemoveThinker(&timer->thinker);
        return;
    }
}

//
// P_SpawnDelayTimer
//

void P_SpawnDelayTimer(line_t* line, void (*func)(void)) {
    delay_t* timer;

    timer = Z_Malloc(sizeof(*timer), PU_LEVSPEC, 0);
    P_AddThinker(&timer->thinker);
    timer->thinker.function.acp1 = (actionf_p1)T_CountdownTimer;
    timer->tics = line->tag;
    timer->finishfunc = func;
}

//
// T_Quake
//

void T_Quake(quake_t* quake) { // 0x8000EDE8
    if(!quake->tics--) {
        P_RemoveThinker(&quake->thinker);
        quakeviewx = 0;
        quakeviewy = 0;
        S_StopSound(NULL, sfx_quake);
        return;
    }

    quakeviewx = (((P_Random(pr_quake) & 1) << 24)-128);
    quakeviewy = (((P_Random(pr_quake) & 1) << 18)-2);
}

//
// P_SpawnQuake
//

static void P_SpawnQuake(line_t* line) {
    quake_t* quake;

    quake = Z_Malloc(sizeof(*quake), PU_LEVSPEC, 0);
    P_AddThinker(&quake->thinker);
    quake->thinker.function.acp1 = (actionf_p1)T_Quake;
    quake->tics = line->tag;

    S_StartSound(NULL, sfx_quake);
}

//
// P_AlertTaggedMobj
//

static void P_AlertTaggedMobj(mobj_t *activator, int tid) {
    mobj_t* mo;
    player_t *player;
    state_t* st;

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        // not matching the tid
        if(mo->tid != tid) {
            continue;
        }

        if(!mo->info->seestate) {
            continue;
        }

        // 20120610 villsa - check for killable things only
        if(!(mo->flags & MF_COUNTKILL)) {
            continue;
        }

        // [kex] TODO - there's no check if the mobj is already dead but
        // if it is, then just revive it. May need to add a feature
        // to skip dead mobjs sometime in the future
        if(mo->health <= 0) {
            mobjinfo_t* info = &mobjinfo[mo->type];

            mo->health      = info->spawnhealth;
            mo->flags       = info->flags;
            mo->height      = info->height;
            mo->radius      = info->radius;
            mo->blockflag   = BF_MOBJPASS;
        }

        st = &states[mo->info->seestate];

        // 03022014 villsa - handle checks if activator is not a player
        if(activator->player) {
            player = activator->player;
        }
        else if(activator->target && activator->target->player) {
            // if the activator's target is a player
            player = activator->target->player;
        }
        else {
            // default to player 1
            player = &players[0];
        }

        P_SetTarget(&mo->target, player->mo);

        mo->threshold   = 0;
        mo->state       = st;
        mo->tics        = st->tics;
        mo->frame       = st->frame;
        mo->sprite      = st->sprite;
    }
}

//
// T_LookAtCamera
//

#define CAMAIMANGLE     ANG5

void T_LookAtCamera(aimcamera_t* camera) {
    mobj_t* camtarget;
    player_t* player;

    camtarget = camera->activator->cameratarget;
    player = camera->activator;

    if(camera->viewmobj != camtarget || camtarget == player->mo) {
        P_RemoveThinker(&camera->thinker);
        return;
    }

    camtarget->angle = P_AdjustAngle(camtarget->angle,
                                     R_PointToAngle2(camtarget->x, camtarget->y, player->mo->x, player->mo->y),
                                     CAMAIMANGLE);
}

//
// P_SetAimCamera
//

int P_SetAimCamera(player_t* player, line_t* line, dboolean aim) {
    mobj_t* mo;
    aimcamera_t* camera;

    P_ClearUserCamera(player);
    player->cheats |= CF_LOCKCAM;

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        // not matching the tid
        if(mo->tid != line->tag) {
            continue;
        }

        // skip if cameratarget matches tag
        if(player->cameratarget->tid == line->tag) {
            continue;
        }

        // 20120304 villsa - handle certain case for co-op
        if(netgame && mo->type == MT_PLAYER) {
            player->cameratarget = player->mo;
        }
        else {
            player->cameratarget = mo;
        }

        if(player->mo == player->cameratarget) {
            player->cheats &= ~CF_LOCKCAM;
            return 1;
        }

        if(!aim) {
            return 1;
        }

        mo->angle = R_PointToAngle2(mo->x, mo->y, player->mo->x, player->mo->y);

        camera = Z_Malloc(sizeof(*camera), PU_LEVSPEC, 0);
        P_AddThinker(&camera->thinker);
        camera->thinker.function.acp1 = (actionf_p1)T_LookAtCamera;
        camera->viewmobj = mo;

        // [kex] store player information
        camera->activator = player;
        return 1;
    }

    return 0;
}

//
// T_MovingCamera
//

#define CAMMOVESPEED    164
#define    CAMTRACEANGLE   ANG1

void T_MovingCamera(movecamera_t* camera) {
    int dist;
    mobj_t* mo;
    mobj_t* camtarget;

    camtarget = camera->player->cameratarget;

    //
    // adjust angle
    //
    camtarget->angle = P_AdjustAngle(camtarget->angle,
                                     R_PointToAngle2(camtarget->x, camtarget->y, camera->x, camera->y),
                                     CAMTRACEANGLE);

    //
    // adjust pitch
    //
    dist = P_AproxDistance(camtarget->x - camera->x, camtarget->y - camera->y);
    if(dist >= (48*FRACUNIT)) {
        camtarget->pitch = P_AdjustAngle(camtarget->pitch,
                                         R_PointToPitch(camtarget->z, camera->z, dist),
                                         CAMTRACEANGLE);
    }

    //
    // adjust camera position
    //
    camera->tic++;
    if(camera->tic < CAMMOVESPEED) {
        camtarget->x += camera->slopex;
        camtarget->y += camera->slopey;
        camtarget->z += camera->slopez;

        return;
    }

    //
    // jump to next camera spot
    //
    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        // not a camera
        if(mo->type != MT_CAMERA) {
            continue;
        }

        // must match tid
        if(mo->tid != camera->current) {
            continue;
        }

        camera->slopex = (mo->x - camtarget->x) / CAMMOVESPEED;
        camera->slopey = (mo->y - camtarget->y) / CAMMOVESPEED;
        camera->slopez = (mo->z - camtarget->z) / CAMMOVESPEED;

        camera->current++;
        camera->tic = 0;

        return;
    }

    // [kex] remove thinker after its done
    P_RemoveThinker(&camera->thinker);
}

//
// P_SetMovingCamera
//

void P_SetMovingCamera(player_t* player, line_t* line) {
    movecamera_t* camera;
    mobj_t* mo;

    if(!player) {
        return;
    }

    if(!player->cameratarget) {
        return;
    }

    camera = Z_Malloc(sizeof(*camera), PU_LEVSPEC, 0);
    P_AddThinker(&camera->thinker);
    camera->thinker.function.acp1 = (actionf_p1)T_MovingCamera;

    if(!(player->cheats & CF_LOCKCAM)) {
        P_ClearUserCamera(player);
        player->cheats |= CF_LOCKCAM;
    }

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        // not matching the tid
        if(mo->tid != line->tag) {
            continue;
        }

        // setup moving camera
        camera->x = mo->x;
        camera->y = mo->y;
        camera->z = mo->z;
        camera->slopex = 0;
        camera->slopey = 0;
        camera->slopez = 0;
        camera->current = mo->tid + 1;
        camera->tic = CAMMOVESPEED;

        // set camera target
        mo->angle = player->cameratarget->angle;
        mo->x = player->cameratarget->x;
        mo->y = player->cameratarget->y;
        player->cameratarget = mo;

        // [kex] store player information
        camera->player = player;

        return;
    }
}

//
// P_ModifyMobjFlags
//
// Removes flags on all current mobjs.
// This doesn't appear to be used at all
//

static dboolean P_ModifyMobjFlags(int tid, int flags) {
    mobj_t* mo;
    dboolean ok = false;

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        // not matching the tid
        if(mo->tid != tid) {
            continue;
        }

        ok = true;

        mo->flags &= ~flags;
    }

    return ok;
}

//
// P_CheckArtifact
//

int P_CheckArtifact(line_t* line, mobj_t* mo, byte type) {
    int num = 0;

    if(mo->player->artifacts & (1<<type)) {
        num = P_FindLinedefFromTag(line->tag+1);
        return P_UseSpecialLine(mo->player->mo, &lines[num], 0);
    }
    else {
        mo->player->message = PD_ARTIFACT;
        mo->player->messagepic = 44;
    }

    return 0;
}

//
// EVENTS
// Events are operations triggered by using, crossing,
// or shooting special lines, or by timed thinkers.
//

//
// P_DoSpecialLine
//

int P_DoSpecialLine(mobj_t* thing, line_t* line, int side) {
    int ok = 0;

    // do something
    switch(SPECIALMASK(line->special)) {
    case 0:
        return 0;
        break;

    case 1:
        // Vertical Door
        EV_VerticalDoor(line, thing);
        break;

    case 2:
        // Open Door
        ok = EV_DoDoor(line, dooropen);
        break;

    case 3:
        // Close Door
        ok = EV_DoDoor(line, doorclose);
        break;

    case 4:
        // Raise Door
        ok = EV_DoDoor(line, normal);
        break;

    case 5:
        // Raise Floor
        ok = EV_DoFloor(line, raiseFloor, FLOORSPEED);
        break;

    case 6:
        // Fast Ceiling Crush & Raise
        ok = EV_DoCeiling(line, fastCrushAndRaise, CEILSPEED);
        break;

    case 8:
        // Build Stairs
        ok = EV_BuildStairs(line,build8);
        break;

    case 10:
        // PlatDownWaitUp
        ok = EV_DoPlat(line,downWaitUpStay,0);
        break;

    case 16:
        // Raise Door
        ok = EV_DoDoor(line, close30ThenOpen);
        break;

    case 17:
        // Spawn Light Strobe
        EV_StartLightStrobing(line);
        ok = 1;
        break;

    case 19:
        // Lower Floor
        ok = EV_DoFloor(line, lowerFloor, FLOORSPEED);
        break;

    case 22:
        // Plat Raise and Change
        ok = EV_DoPlat(line, raiseAndChange, 0);
        break;

    case 25:
        // Ceiling Crush and Raise
        ok = EV_DoCeiling(line, crushAndRaise, CEILSPEED);
        break;

    case 30:
        // Raise Floor To Nearest
        ok = EV_DoFloor(line, raiseFloorToNearest, FLOORSPEED);
        break;

    case 31:
        // Vertical Door
        EV_VerticalDoor(line, thing);
        break;

    case 36:
        // Lower Floor (TURBO)
        ok = EV_DoFloor(line, turboLower, FLOORSPEED * 4);
        break;

    case 37:
        // LowerAndChange
        ok = EV_DoFloor(line, lowerAndChange, FLOORSPEED);
        break;

    case 38:
        // Lower Floor To Lowest
        ok = EV_DoFloor(line, lowerFloorToLowest, FLOORSPEED);
        break;

    case 39:
        // TELEPORT!
        ok = EV_Teleport(line, side, thing);
        break;

    case 43:
        // Ceiling Crush
        ok = EV_DoCeiling(line, lowerToFloor, CEILSPEED);
        break;

    case 44:
        // Ceiling Crush and Raise
        ok = EV_DoCeiling(line, lowerAndCrush, CEILSPEED);
        break;

    case 52:
        // EXIT!
        G_ExitLevel();
        ok = 1;
        break;

    case 53:
        // Perpetual Platform Raise
        ok = EV_DoPlat(line,perpetualRaise,0);
        break;

    case 54:
        // Platform Stop
        EV_StopPlat(line);
        ok = 1;
        break;

    case 56:
        // Raise Floor Crush
        ok = EV_DoFloor(line, raiseFloorCrush, FLOORSPEED);
        break;

    case 57:
        // Ceiling Crush Stop
        ok = EV_CeilingCrushStop(line);
        break;

    case 58:
        // Raise Floor 24
        ok = EV_DoFloor(line, raiseFloor24, FLOORSPEED);
        break;

    case 59:
        // Raise Floor 24
        ok = EV_DoFloor(line, raiseFloor24AndChange, FLOORSPEED);
        break;

    case 66:
        // Perpetual Platform Raise
        ok = EV_DoPlat(line, downWaitUpStay, 24);
        break;

    case 67:
        // Perpetual Platform Raise
        ok = EV_DoPlat(line, downWaitUpStay, 32);
        break;

    case 90:
        // Artifact Switch 1
        ok = P_CheckArtifact(line, thing, ART_FAST);
        break;

    case 91:
        // Artifact Switch 2
        ok = P_CheckArtifact(line, thing, ART_DOUBLE);
        break;

    case 92:
        // Artifact Switch 3
        ok = P_CheckArtifact(line, thing, ART_TRIPLE);
        break;

    case 93:
        // Modify mobj flags
        ok = P_ModifyMobjFlags(line->tag, MF_NOINFIGHTING);
        break;

    case 94:
        // Noise Alert
        P_AlertTaggedMobj(thing, line->tag);
        ok = 1;
        break;

    case 100:
        // Build Stairs Turbo 16
        ok = EV_BuildStairs(line,turbo16);
        break;

    case 108:
        // Blazing Door Raise (faster than TURBO!)
        ok = EV_DoDoor(line,blazeRaise);
        break;

    case 109:
        // Blazing Door Open (faster than TURBO!)
        ok = EV_DoDoor(line,blazeOpen);
        break;

    case 110:
        // Blazing Door Close (faster than TURBO!)
        ok = EV_DoDoor(line,blazeClose);
        break;

    case 117:
        // Vertical Door
        EV_VerticalDoor(line, thing);
        break;

    case 118:
        EV_VerticalDoor(line, thing);
        break;

    case 119:
        // Raise Floor To Nearest
        ok = EV_DoFloor(line, raiseFloorToNearest, FLOORSPEED);
        break;

    case 121:
        // Blazing PlatDownWaitUpStay
        ok = EV_DoPlat(line,blazeDWUS,0);
        break;

    case 122:
        // PlatUpWaitDownStay
        ok = EV_DoPlat(line,upWaitDownStay,0);
        break;

    case 123:
        // Blazing PlatUpWaitDownStay
        ok = EV_DoPlat(line,blazeUWDS,0);
        break;

    case 124:
        // Secret EXIT
        G_SecretExitLevel(line->tag);
        ok = 1;
        break;

    case 125:
        if(!thing->player) {
            ok = EV_Teleport(line, side, thing);
        }
        break;

    case 141:
        // Demon Disposer
        ok = EV_DoCeiling(line, silentCrushAndRaise, CEILSPEED * 2);
        break;

    case 200:
        // Set Lookat Camera
        ok = P_SetAimCamera(thing->player, line, true);
        break;

    case 201:
        // Set Camera
        ok = P_SetAimCamera(thing->player, line, false);
        break;

    case 202:
        // Invoke Dart
        P_SpawnDartMissile(line->tag, MT_PROJ_DART, thing);
        ok = 1;
        break;

    case 203:
        // Delay Thinker
        P_SpawnDelayTimer(line, NULL);
        ok = 1;
        break;

    case 204:
        // Set global integer
        globalint = line->tag;
        ok = 1;
        break;

    case 205:
        // Modify sector color
        ok = P_ModifySectorColor(line, globalint, 0);
        break;

    case 206:
        // Modify sector color
        ok = P_ModifySectorColor(line, globalint, 1);
        break;

    case 207:
        // Modify sector color
        ok = P_ModifySectorColor(line, globalint, 2);
        break;

    case 208:
        // Modify sector color
        ok = P_ModifySectorColor(line, globalint, 3);
        break;

    case 209:
        // Modify sector color
        ok = P_ModifySectorColor(line, globalint, 4);
        break;

    case 210:
        ok = EV_DoCeiling(line, customCeiling, CEILSPEED);
        break;

    case 212:
        ok = EV_DoFloor(line, customFloor, FLOORSPEED);
        break;

    case 214:
        // Elevator Sector
        ok = EV_SplitSector(line, true);
        break;

    case 218:
        //Modify Line Flags
        ok = P_ModifyLine(line->tag, globalint, modl_flags);
        break;

    case 219:
        //Modify Line Texture
        ok = P_ModifyLine(line->tag, globalint, modl_texture);
        break;

    case 220:
        // Modify Sector Flags
        ok = P_ModifySector(line, globalint, mods_flags);
        break;

    case 221:
        // Modify Sector Specials
        ok = P_ModifySector(line, globalint, mods_special);
        break;

    case 222:
        // Modify Sector Lights
        ok = P_ModifySector(line, globalint, mods_lights);
        break;

    case 223:
        // Modify Sector Flats
        ok = P_ModifySector(line, globalint, mods_flats);
        break;

    case 224:
        // Spawn Thing
        ok = EV_SpawnMobjTemplate(line);
        break;

    case 225:
        // Quake Effect
        P_SpawnQuake(line);
        ok = 1;
        break;

    case 226:
        ok = EV_DoCeiling(line, customCeiling, CEILSPEED * 4);
        break;

    case 227:
        ok = EV_DoCeiling(line, customCeiling, CEILSPEED * 2048);
        break;

    case 228:
        ok = EV_DoFloor(line, customFloor, FLOORSPEED * 4);
        break;

    case 229:
        ok = EV_DoFloor(line, customFloor, FLOORSPEED * 2048);
        break;

    case 230:
        // Modify Line Special
        ok = P_ModifyLine(line->tag, globalint, modl_data);
        break;

    case 231:
        // Invoke Revenant Missile
        P_SpawnDartMissile(line->tag, MT_PROJ_TRACER, thing);
        ok = 1;
        break;

    case 232:
        // Fast Ceiling Crush & Raise
        ok = EV_DoCeiling(line, crushAndRaiseOnce, CEILSPEED * 4);
        break;

    case 233:
        // Freeze Player
        thing->reactiontime = line->tag;
        ok = 1;
        break;

    case 234:
        // Change light by light tag
        ok = P_ChangeLightByTag(line->tag, globalint);
        break;

    case 235:
        // Modify Light Data
        ok = P_DoSectorLightChange(line, globalint);
        break;

    case 236:
        ok = EV_DoPlat(line,customDownUp,0);
        break;

    case 237:
        ok = EV_DoPlat(line,customDownUpFast,0);
        break;

    case 238:
        ok = EV_DoPlat(line,customUpDown,0);
        break;

    case 239:
        ok = EV_DoPlat(line,customUpDownFast,0);
        break;

    case 240:
        ok = P_RandomLineTrigger(line, thing, side);
        break;

    case 241:
        // Split Open Sector
        ok = EV_SplitSector(line, false);
        break;

    case 242:
        // Fade Out Thing
        ok = EV_FadeOutMobj(line);
        break;

    case 243:
        // Move and Aim Camera
        P_SetMovingCamera(thing->player, line);
        ok = 1;
        break;

    case 244:
        // Modify Sector Floor
        ok = EV_DoFloor(line, customFloorToHeight, FLOORSPEED * 2048);
        break;

    case 245:
        // Modify Sector Ceiling
        ok = EV_DoCeiling(line, customCeilingToHeight, CEILSPEED * 2048);
        break;

    case 246:
        // Restart Macro at ID
        ok = P_InitMacroCounter(line->tag);
        break;

    case 247:
        ok = EV_DoFloor(line, customFloorToHeight, FLOORSPEED);
        break;

    case 248:
        // Suspend a macro script
        ok = P_SuspendMacro(line->tag);
        break;

    case 249:
        // Silent Teleport
        ok = EV_SilentTeleport(line, thing);
        break;

    case 250:
        // Toggle macros on
        P_ToggleMacros(line->tag, true);
        ok = 1;
        break;

    case 251:
        // Toggle macros off
        P_ToggleMacros(line->tag, false);
        ok = 1;
        break;

    case 252:
        ok = EV_DoCeiling(line, customCeilingToHeight, CEILSPEED);
        break;

    case 253:
        // Unlock Cheat Menu
        CON_CvarSetValue(p_features.name, 1);
        ok = 1;
        break;

    case 254:
        // D64 Map33 Logo
        skyfadeback = true;
        ok = 1;
        break;

    default:
        CON_Warnf("P_DoSpecialLine: Unknown Special: %i\n", line->special);
        return 0;
    }

    return ok;
}

//
// P_InitSpecialLine
//

dboolean P_InitSpecialLine(mobj_t* thing, line_t* line, int side) {
    int ok = 0;
    int use = line->special & MLU_REPEAT;

    if(line->special & MLU_MACRO) {
        ok = P_StartMacro(thing, line);
    }
    else {
        ok = P_DoSpecialLine(thing, line, side);
    }

    if(!ok) {
        return false;
    }

    if((line->special & MLU_USE || line->special & MLU_SHOOT) && ok) {
        P_ChangeSwitchTexture(line, use);
    }

    if(!use) {
        line->special = 0;
    }

    return true;
}

//
// P_UseSpecialLine
// Called when a thing uses a special line.
// Only the front sides of lines are usable.
//

dboolean P_UseSpecialLine(mobj_t* thing, line_t* line, int side) {
    player_t* p;

    if(line->flags & ML_TRIGGERFRONT && side == 1) {
        return false;
    }

    // Switches that other things can activate. MT_FAKEITEM has full player privilages.
    if(!thing->player && thing->type != MT_FAKEITEM) {
        if(thing->tid == line->tag) {
            // triggered pickups can activate line specials
            if(thing->flags & MF_SPECIAL && thing->flags & MF_TRIGTOUCH) {
                return P_InitSpecialLine(thing, line, side);
            }

            // triggered dead things can activate line specials
            if(line->flags & ML_THINGTRIGGER && thing->flags & MF_TRIGDEATH) {
                return P_InitSpecialLine(thing, line, side);
            }
        }

        // never open secret doors
        if(line->flags & ML_SECRET) {
            return false;
        }

        // never allow a non-player mobj to use lines with these useflags
        if(line->special & (MLU_BLUE|MLU_YELLOW|MLU_RED|MLU_SHOOT|MLU_MACRO)) {
                return false;
        }

        // Missiles should NOT trigger specials...
        if(thing->flags & MF_MISSILE) {
            return false;
        }

        if(line->special & MLU_USE) {
            if(SPECIALMASK(line->special) != 1) {
                return false;
            }
        }
        else if(line->special & MLU_CROSS) {
            switch(SPECIALMASK(line->special)) {
                case 4:     // RAISE DOOR
                case 10:    // RAISE PLATFORM
                case 39:    // TELEPORT TRIGGER
                case 125:   // TELEPORT MONSTERONLY TRIGGER
                    break;

                default:
                    return false;
            }
        }
    }
    else {
        p = thing->player;

        if(line->special & MLU_BLUE) {
            if(!p->cards[it_bluecard] && !p->cards[it_blueskull]) {
                p->message = PD_BLUE;
                p->messagepic = 0;
                S_StartSound(thing,sfx_noway);
                p->tryopen[tryopentype[0]] = true;
                return false;
            }
        }
        else if(line->special & MLU_YELLOW) {
            if(!p->cards[it_yellowcard] && !p->cards[it_yellowskull]) {
                p->message = PD_YELLOW;
                p->messagepic = 1;
                S_StartSound(thing,sfx_noway);
                p->tryopen[tryopentype[1]] = true;
                return false;
            }
        }
        else if(line->special & MLU_RED) {
            if(!p->cards[it_redcard] && !p->cards[it_redskull]) {
                p->message = PD_RED;
                p->messagepic = 2;
                S_StartSound(thing,sfx_noway);
                p->tryopen[tryopentype[2]] = true;
                return false;
            }
        }
    }

    return P_InitSpecialLine(thing, line, side);
}



//
// P_PlayerInSpecialSector
//
// Called every tic frame
// that the player origin is in a special sector
//

void P_PlayerInSpecialSector(player_t* player) {
    sector_t*    sector;
    sector = player->mo->subsector->sector;

    // Falling, not all the way down yet?
    if(player->mo->z != sector->floorheight) {
        return;
    }

    if(sector->flags & MS_SECRET) {
        player->secretcount++;
        player->message = FOUNDSECRET;
        player->messagepic = 40;
        sector->flags &= ~MS_SECRET;
    }

    if(sector->flags & MS_DAMAGEX5) {
        // NUKAGE DAMAGE
        if(!player->powers[pw_ironfeet])
            if(!(leveltime&0x1f)) {
                P_DamageMobj(player->mo, NULL, NULL, 5);
            }
    }
    if(sector->flags & MS_DAMAGEX10) {
        // HELLSLIME DAMAGE
        if(!player->powers[pw_ironfeet])
            if(!(leveltime&0x1f)) {
                P_DamageMobj(player->mo, NULL, NULL, 10);
            }
    }
    if(sector->flags & MS_DAMAGEX20) {
        if(!player->powers[pw_ironfeet] || (P_Random(pr_slimehurt)<5)) {
            if(!(leveltime&0x1f)) {
                P_DamageMobj(player->mo, NULL, NULL, 20);
            }
        }
    }

    if(sector->flags & MS_SCROLLFLOOR) {
        fixed_t speed;

        if(sector->flags & MS_SCROLLFAST) {
            speed = 0x3000;
        }
        else {
            speed = 0x1000;
        }

        if(sector->flags & MS_SCROLLLEFT) {
            P_Thrust(player, ANG180, speed);
        }
        if(sector->flags & MS_SCROLLRIGHT) {
            P_Thrust(player, 0, speed);
        }
        if(sector->flags & MS_SCROLLUP) {
            P_Thrust(player, ANG90, speed);
        }
        if(sector->flags & MS_SCROLLDOWN) {
            P_Thrust(player, ANG270, speed);
        }
    }
}


//
// P_UpdateSpecials
// Animate planes, scroll walls, etc.
//

dboolean        levelTimer;
int             levelTimeCount;
extern line_t** linespeciallist;
extern short    numlinespecials;

void P_UpdateSpecials(void) {
    int         i;
    line_t*     line;
    sector_t*   sector;

    //    LEVEL TIMER
    if(levelTimer == true) {
        levelTimeCount--;
        if(!levelTimeCount) {
            G_ExitLevel();
        }
    }

    // ANIMATE FLATS AND TEXTURES GLOBALLY
    P_CyclePicAnims();

    // ANIMATE LINE SPECIALS
    for(i = 0; i < numlinespecials; i++) {
        line = linespeciallist[i];
        if(line->flags & ML_SCROLLRIGHT) {
            sides[line->sidenum[0]].textureoffset += FRACUNIT;
        }

        if(line->flags & ML_SCROLLLEFT) {
            sides[line->sidenum[0]].textureoffset -= FRACUNIT;
        }

        if(line->flags & ML_SCROLLUP) {
            sides[line->sidenum[0]].rowoffset += FRACUNIT;
        }

        if(line->flags & ML_SCROLLDOWN) {
            sides[line->sidenum[0]].rowoffset -= FRACUNIT;
        }
    }

    // UPDATE SCROLLING FLATS
    for(i = 0; i < numsectors; i++) {
        sector = &sectors[i];

        if(sector->flags & (MS_SCROLLFLOOR|MS_SCROLLCEILING)) {
            fixed_t speed;

            if(sector->flags & MS_SCROLLFAST) {
                speed = 3*FRACUNIT;
            }
            else {
                speed = FRACUNIT;
            }

            if(sector->flags & MS_SCROLLLEFT) {
                sector->xoffset += speed;
            }
            if(sector->flags & MS_SCROLLRIGHT) {
                sector->xoffset -= speed;
            }
            if(sector->flags & MS_SCROLLUP) {
                sector->yoffset += speed;
            }
            if(sector->flags & MS_SCROLLDOWN) {
                sector->yoffset -= speed;
            }
        }

    }

    // SKY TICKER
    if(bRenderSky) {
        R_SkyTicker();
    }

    // DO BUTTONS
    for(i = 0; i < MAXBUTTONS; i++) {
        if(buttonlist[i].btimer) {
            buttonlist[i].btimer--;
            if(!buttonlist[i].btimer) {
                switch(buttonlist[i].where) {
                case top:
                    sides[buttonlist[i].line->sidenum[0]].toptexture =
                        buttonlist[i].btexture ^ 1;
                    break;

                case middle:
                    sides[buttonlist[i].line->sidenum[0]].midtexture =
                        buttonlist[i].btexture ^ 1;
                    break;

                case bottom:
                    sides[buttonlist[i].line->sidenum[0]].bottomtexture =
                        buttonlist[i].btexture ^ 1;
                    break;
                }

                S_StartSound((mobj_t *)&buttonlist[i].line->frontsector->soundorg, sfx_switch1);
                dmemset(&buttonlist[i],0,sizeof(button_t));
            }
        }
    }

    scrollfrac += (FRACUNIT / 2);
}


//
// SPECIAL SPAWNING
//

//
// P_SpawnSpecials
//
// After the map has been loaded, scan for specials
// that spawn thinkers
//

line_t**    linespeciallist;
short       numlinespecials;

void P_AddSectorSpecial(sector_t* sector) {
    if(!sector->special) {
        return;
    }

    if(sector->flags & MS_SYNCSPECIALS) {
        if(sector->special) {
            P_CombineLightSpecials(sector);
            return;
        }
    }

    switch(sector->special) {
    case 0:
        sector->lightlevel = 0;
        break;

    case 1:
        // FLICKERING LIGHTS
        P_SpawnLightFlash(sector);
        break;

    case 2:
        // STROBE FAST
        P_SpawnStrobeFlash(sector, FASTDARK);
        break;

    case 3:
        // STROBE SLOW
        P_SpawnStrobeFlash(sector,SLOWDARK);
        break;

    case 8:
        // GLOWING LIGHT
        P_SpawnGlowingLight(sector, PULSENORMAL);
        break;

    case 9:
        P_SpawnGlowingLight(sector, PULSESLOW);
        break;

    case 11:
        P_SpawnGlowingLight(sector, PULSERANDOM);
        break;

    case 17:
        P_SpawnFireFlicker(sector);
        break;

    case 202:
        P_SpawnStrobeAltFlash(sector, 3);
        break;

    case 204:
        P_SpawnStrobeFlash(sector, 7);
        break;

    case 205:
        P_SpawnSequenceLight(sector, true);
        break;

    case 206:
        P_SpawnStrobeFlash(sector, 90);
        break;

    case 208:
        P_SpawnStrobeAltFlash(sector, 6);
        break;

    case 666:
        break;

    default:
        CON_Warnf("P_AddSectorSpecial: Unknown Sector Special %i\n", sector->special);
        break;
    }
}


//
// P_SpawnSpecials
//

void P_SpawnSpecials(void) {
    sector_t*   sector;
    int         i;
    mobj_t*     mo;

    // See if -TIMER needs to be used.
    levelTimer = false;

    i = M_CheckParm("-avg");
    if(i && deathmatch) {
        levelTimer = true;
        levelTimeCount = 20 * 60 * TICRATE;
    }

    i = M_CheckParm("-timer");
    if(i && deathmatch) {
        int    time;
        time = datoi(myargv[i+1]) * 60 * TICRATE;
        levelTimer = true;
        levelTimeCount = time;
    }

    //
    // [kex] reset data for animpics
    //
    for(i = 0; i < numanimdef; i++) {
        animinfo[i].delay = 0;
        animinfo[i].frame = -1;
        animinfo[i].tic = 0;
        animinfo[i].isreverse = false;
    }

    // Init special sectors
    // Might as well count all the secrets while we're at it..
    sector = sectors;
    for(i = 0; i < numsectors; i++, sector++) {
        P_AddSectorSpecial(sector);

        // [kex] 12/26/11 - don't increment stats when loading a savegame
        if(gameaction != ga_loadgame) {
            if(sector->flags & MS_SECRET) {
                totalsecret++;
            }
        }
    }

    // Get type of key used in level
    tryopentype[0] = it_bluecard;
    tryopentype[1] = it_yellowcard;
    tryopentype[2] = it_redcard;

    for(mo = mobjhead.next; mo != &mobjhead; mo = mo->next) {
        if(mo->type == MT_ITEM_BLUESKULLKEY) {
            tryopentype[0] = it_blueskull;
        }

        if(mo->type == MT_ITEM_YELLOWSKULLKEY) {
            tryopentype[1] = it_yellowskull;
        }

        if(mo->type == MT_ITEM_REDSKULLKEY) {
            tryopentype[2] = it_redskull;
        }
    }

    scrollfrac = 0;

    for(i = 0; i < numspawnlist; i++) {
        if(spawnlist[i].type == mobjinfo[MT_ITEM_BLUESKULLKEY].doomednum) {
            tryopentype[0] = it_blueskull;
        }

        if(spawnlist[i].type == mobjinfo[MT_ITEM_YELLOWSKULLKEY].doomednum) {
            tryopentype[1] = it_yellowskull;
        }

        if(spawnlist[i].type == mobjinfo[MT_ITEM_REDSKULLKEY].doomednum) {
            tryopentype[2] = it_redskull;
        }
    }

    //    Init line EFFECTs
    numlinespecials = 0;
    linespeciallist = Z_Malloc(sizeof(line_t*) * 1, PU_LEVEL, 0);
    for(i = 0; i < numlines; i++) {
        if(lines[i].flags & ML_SCROLLRIGHT || lines[i].flags & ML_SCROLLLEFT
                || lines[i].flags & ML_SCROLLUP || lines[i].flags & ML_SCROLLDOWN) {
            linespeciallist[numlinespecials++] = &lines[i];
            linespeciallist = (line_t**)Z_Realloc(linespeciallist,
                                                  (numlinespecials+1) * sizeof(line_t*), PU_LEVEL, 0);
        }
    }

    //    Init other misc stuff
    for(i = 0; i < MAXCEILINGS; i++) {
        activeceilings[i] = NULL;
    }

    for(i = 0; i < MAXPLATS; i++) {
        activeplats[i] = NULL;
    }

    for(i = 0; i < MAXBUTTONS; i++) {
        dmemset(&buttonlist[i],0,sizeof(button_t));
    }
}

