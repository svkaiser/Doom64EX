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
// DESCRIPTION: Door animation code (opening/closing)
//
//-----------------------------------------------------------------------------


#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"
#include "s_sound.h"
#include "doomstat.h"
#include "d_englsh.h"
#include "sounds.h"


//
// VERTICAL DOORS
//

//
// T_VerticalDoor
//
void T_VerticalDoor(vldoor_t* door) {
    result_e res1;
    result_e res2;

    switch(door->direction) {
    case 0:
        // WAITING
        if(!--door->topcountdown) {
            switch(door->type) {
            case blazeRaise:
                door->direction = -1; // time to go back down
                S_StartSound((mobj_t *)&door->sector->soundorg,sfx_door2dwn);
                break;

            case normal:
                door->direction = -1; // time to go back down
                S_StartSound((mobj_t *)&door->sector->soundorg,sfx_doordown);
                break;

            case close30ThenOpen:
                door->direction = 1;
                S_StartSound((mobj_t *)&door->sector->soundorg,sfx_doorup);
                break;

            default:
                break;
            }
        }
        break;

    case 2:
        //  INITIAL WAIT
        if(!--door->topcountdown) {
            switch(door->type) {
            case raiseIn5Mins:
                door->direction = 1;
                door->type = normal;
                S_StartSound((mobj_t *)&door->sector->soundorg,sfx_doorup);
                break;

            default:
                break;
            }
        }
        break;

    case -1:
        // DOWN
        res1 = T_MovePlane(door->sector, door->speed, door->initceiling, false, 1, -1);
        res2 = T_MovePlane(door->sector, door->speed, door->initceiling, false, 0, 1);

        if(res1 == pastdest && res2 == pastdest) {
            switch(door->type) {
            case blazeRaise:
            case blazeClose:
                door->sector->specialdata = NULL;
                P_RemoveThinker(&door->thinker);   // unlink and free
                S_StartSound((mobj_t *)&door->sector->soundorg,sfx_door2dwn);
                break;

            case normal:
            case doorclose:
                door->sector->specialdata = NULL;
                P_RemoveThinker(&door->thinker);   // unlink and free
                break;

            case close30ThenOpen:
                door->direction = 0;
                door->topcountdown = 30*30;
                break;

            default:
                break;
            }
        }
        else if(res1 == crushed) {
            switch(door->type) {
            case blazeClose:
            case doorclose:        // DO NOT GO BACK UP!
                break;

            default:
                door->direction = 1;
                S_StartSound((mobj_t *)&door->sector->soundorg,sfx_doorup);
                break;
            }
        }
        break;

    case 1:
        // UP
        res1 = T_MovePlane(door->sector, door->speed, door->topheight, false, 1, 1);
        res2 = T_MovePlane(door->sector, door->speed, door->bottomheight, false, 0, -1);

        if(res1 == pastdest && res2 == pastdest) {
            switch(door->type) {
            case blazeRaise:
            case normal:
                door->direction = 0; // wait at top
                door->topcountdown = door->topwait;
                break;

            case close30ThenOpen:
            case blazeOpen:
            case dooropen:
                door->sector->specialdata = NULL;
                P_RemoveThinker(&door->thinker);   // unlink and free
                break;

            default:
                break;
            }
        }
        break;
    }
}

int EV_DoDoor(line_t* line, vldoor_e type) {
    int secnum, rtn;
    sector_t* sec;
    vldoor_t* door;

    secnum = -1;
    rtn = 0;

    while((secnum = P_FindSectorFromLineTag(line,secnum)) >= 0) {
        sec = &sectors[secnum];
        if(sec->specialdata) {
            continue;
        }


        // new door thinker
        rtn = 1;
        door = Z_Malloc(sizeof(*door), PU_LEVSPEC, 0);
        P_AddThinker(&door->thinker);
        sec->specialdata = door;

        door->thinker.function.acp1 = (actionf_p1) T_VerticalDoor;
        door->sector = sec;
        door->type = type;
        door->topwait = VDOORWAIT;
        door->speed = VDOORSPEED;
        door->bottomheight = sec->floorheight;
        door->initceiling = sec->floorheight;

        switch(type) {
        case blazeClose:
            door->topheight = P_FindLowestCeilingSurrounding(sec) - 4*FRACUNIT;
            door->direction = -1;
            door->speed = VDOORBLAZESPEED;
            S_StartSound((mobj_t *)&door->sector->soundorg,
                         sfx_door2dwn);
            break;

        case doorclose:
            door->topheight = P_FindLowestCeilingSurrounding(sec) - 4*FRACUNIT;
            door->direction = -1;
            S_StartSound((mobj_t *)&door->sector->soundorg,
                         sfx_doordown);
            break;

        case close30ThenOpen:
            door->topheight = sec->ceilingheight;
            door->direction = -1;
            S_StartSound((mobj_t *)&door->sector->soundorg,
                         sfx_doordown);
            break;

        case blazeRaise:
        case blazeOpen:
            door->direction = 1;
            door->topheight = P_FindLowestCeilingSurrounding(sec) - 4*FRACUNIT;
            door->speed = VDOORBLAZESPEED;
            if(door->topheight != sec->ceilingheight)
                S_StartSound((mobj_t *)&door->sector->soundorg,
                             sfx_door2up);
            break;

        case normal:
        case dooropen:
            door->direction = 1;
            door->topheight = P_FindLowestCeilingSurrounding(sec) - 4*FRACUNIT;
            if(door->topheight != sec->ceilingheight)
                S_StartSound((mobj_t *)&door->sector->soundorg,
                             sfx_doorup);
            break;

        default:
            break;
        }

    }
    return rtn;
}


//
// EV_VerticalDoor
// open a door manually, no tag value
//

void EV_VerticalDoor(line_t* line, mobj_t* thing) {
    sector_t*   sec;
    vldoor_t*   door;
    int         side;

    side = 0;   // only front sides can be used

    // if the sector has an active thinker, use it
    sec = sides[line->sidenum[side^1]].sector;

    if(sec->specialdata) {
        door = sec->specialdata;
        switch(SPECIALMASK(line->special)) {
        case 1: // ONLY FOR "RAISE" DOORS, NOT "OPEN"s
        case 117:
            if(door->direction == -1) {
                door->direction = 1;    // go back up
            }
            else {
                if(!thing->player) {
                    return;    // JDC: bad guys never close doors
                }

                door->direction = -1;   // start going down immediately
            }
            return;
        }
    }

    // for proper sound
    switch(SPECIALMASK(line->special)) {
    case 117:   // BLAZING DOOR RAISE
    case 118:   // BLAZING DOOR OPEN
        S_StartSound((mobj_t *)&sec->soundorg,sfx_door2up);
        break;

    default:    // NORMAL DOOR SOUND
        S_StartSound((mobj_t *)&sec->soundorg,sfx_doorup);
        break;
    }


    // new door thinker
    door = Z_Malloc(sizeof(*door), PU_LEVSPEC, 0);
    P_AddThinker(&door->thinker);
    sec->specialdata = door;
    door->thinker.function.acp1 = (actionf_p1)T_VerticalDoor;
    door->sector = sec;
    door->direction = 1;
    door->speed = VDOORSPEED;
    door->topwait = VDOORWAIT;

    switch(SPECIALMASK(line->special)) {
    case 1:
        door->type = normal;
        break;

    case 31:
        door->type = dooropen;
        line->special = 0;
        break;

    case 117:   // blazing door raise
        door->type = blazeRaise;
        door->speed = VDOORSPEED*4;
        break;

    case 118:   // blazing door open
        door->type = blazeOpen;
        line->special = 0;
        door->speed = VDOORSPEED*4;
        break;
    }

    // find the top and bottom of the movement range
    door->topheight = P_FindLowestCeilingSurrounding(sec) - 4*FRACUNIT;
    door->bottomheight = P_FindLowestFloorSurrounding(sec);
    door->initceiling = sec->floorheight;

    if(door->bottomheight != sec->floorheight) {
        door->bottomheight += 4*FRACUNIT;
    }
}


