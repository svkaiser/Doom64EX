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
//      Archiving: SaveGame I/O.
//      Thinker, Ticker.
//
//-----------------------------------------------------------------------------

#include "doomstat.h"
#include "z_zone.h"
#include "p_local.h"
#include "p_macros.h"
#include "st_stuff.h"
#include "am_map.h"
#include "r_main.h"
#include "s_sound.h"
#include "m_random.h"
#include "con_console.h"
#include "r_wipe.h"
#include "p_setup.h"
#include "g_demo.h"

CVAR_EXTERNAL(i_interpolateframes);
CVAR_EXTERNAL(p_damageindicator);
CVAR_EXTERNAL(r_wipe);

int     leveltime;

void G_PlayerFinishLevel(int player);
void G_DoReborn(int playernum);

//
// THINKERS
//
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//
// Mobjs are now kept seperate for more optimal
// list processing.
//

thinker_t   thinkercap;      // Both the head and tail of the thinker list.
mobj_t      mobjhead;        // Both the head and tail of the mobj list.
mobj_t      *currentmobj;
thinker_t   *currentthinker;


//
// P_InitThinkers
//

void P_InitThinkers(void) {
    thinkercap.prev = thinkercap.next  = &thinkercap;
    mobjhead.next = mobjhead.prev = &mobjhead;
}

//
// P_AddThinker
// Adds a new thinker at the end of the list.
//

void P_AddThinker(thinker_t* thinker) {
    thinkercap.prev->next = thinker;
    thinker->next = &thinkercap;
    thinker->prev = thinkercap.prev;
    thinkercap.prev = thinker;
}

//
// P_UnlinkThinker
//

static void P_UnlinkThinker(thinker_t* thinker) {
    thinker_t* next = currentthinker->next;
    (next->prev = currentthinker = thinker->prev)->next = next;

    Z_Free(thinker);
}

//
// P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//

void P_RemoveThinker(thinker_t* thinker) {
    thinker->function.acp1 = P_UnlinkThinker;
    P_MacroDetachThinker(thinker);
}

//
// P_LinkMobj
//

void P_LinkMobj(mobj_t* mobj) {
    mobjhead.prev->next = mobj;
    mobj->next = &mobjhead;
    mobj->prev = mobjhead.prev;
    mobjhead.prev = mobj;
}

//
// P_UnlinkMobj
//

void P_UnlinkMobj(mobj_t* mobj) {
    /* Remove from main mobj list */
    mobj_t* next = currentmobj->next;

    /* Note that currentmobj is guaranteed to point to us,
    * and since we're freeing our memory, we had better change that. So
    * point it to mobj->prev, so the iterator will correctly move on to
    * mobj->prev->next = mobj->next */
    (next->prev = currentmobj = mobj->prev)->next = next;
}

//
// P_RunMobjs
//

void P_RunMobjs(void) {
    for(currentmobj = mobjhead.next; currentmobj != &mobjhead; currentmobj = currentmobj->next) {
        if(!currentmobj) {
            CON_Warnf("P_RunMobjs: Null mobj in linked list!\n");
            break;
        }

        // Special case only
        if(currentmobj->flags & MF_NOSECTOR) {
            continue;
        }

        if(gameflags & GF_LOCKMONSTERS && !currentmobj->player && currentmobj->flags & MF_COUNTKILL) {
            continue;
        }

        if(!currentmobj->player) {
            // [kex] don't bother if about to be removed
            if(currentmobj->mobjfunc != P_SafeRemoveMobj) {
                // [kex] don't clear callback if mobj is going to be respawning
                if(currentmobj->mobjfunc != P_RespawnSpecials) {
                    currentmobj->mobjfunc = NULL;
                }

                P_MobjThinker(currentmobj);
            }

            if(currentmobj->mobjfunc) {
                currentmobj->mobjfunc(currentmobj);
            }
        }
    }
}

//
// P_RunThinkers
//

void P_RunThinkers(void) {
    for(currentthinker = thinkercap.next;
            currentthinker != &thinkercap;
            currentthinker = currentthinker->next) {
        if(currentthinker->function.acp1) {
            currentthinker->function.acp1(currentthinker);
        }
    }
}

//
// P_UpdateFrameStates
//

angle_t frame_angle = 0;
angle_t frame_pitch = 0;
fixed_t frame_viewx = 0;
fixed_t frame_viewy = 0;
fixed_t frame_viewz = 0;

static void P_UpdateFrameStates(void) {
    player_t    *player = &players[displayplayer];
    pspdef_t    *psp;
    mobj_t      *viewcamera;
    angle_t     pitch;
    mobj_t      *mobj;
    int         i;

    viewcamera = player->cameratarget;
    pitch = viewcamera->pitch + ANG90;

    if(viewcamera == player->mo) {
        pitch += player->recoilpitch;
    }

    //
    // update player position/view for interpolation
    //
    frame_viewx = player->cameratarget->x;
    frame_viewy = player->cameratarget->y;
    frame_viewz = (viewcamera == player->mo ? player->viewz : viewcamera->z) + quakeviewy;
    frame_angle = (player->cameratarget->angle + quakeviewx) + viewangleoffset;
    frame_pitch = pitch;

    //
    // update player sprites for interpolation
    //
    psp = &player->psprites[ps_weapon];

    player->psprites[ps_weapon].frame_x = psp->sx;
    player->psprites[ps_weapon].frame_y = psp->sy;
    player->psprites[ps_flash].frame_x = psp->sx;
    player->psprites[ps_flash].frame_y = psp->sy;

    //
    // update sector frames for interpolation
    //
    for(i = 0; i < numsectors; i++) {
        sector_t* sector = &sectors[i];

        sector->frame_z1[0] = sector->floorheight;
        sector->frame_z2[0] = sector->ceilingheight;
        sector->frame_z1[1] = sector->frame_z1[0];
        sector->frame_z2[1] = sector->frame_z1[0];
    }

    //
    // update mobj frames for interpolation
    //
    for(mobj = mobjhead.next; mobj != &mobjhead; mobj = mobj->next) {
        if(!mobj) {
            CON_Warnf("P_UpdateFrameStates: Null mobj in linked list!\n");
            continue;
        }

        // Special case only
        if(mobj->flags & MF_NOSECTOR) {
            continue;
        }

        mobj->frame_x = mobj->x;
        mobj->frame_y = mobj->y;
        mobj->frame_z = mobj->z;
    }
}

//
// P_Start
//

void P_Start(void) {
    int i;
    mapdef_t* map;

    map = P_GetMapInfo(gamemap);

    for(i = 0; i < MAXPLAYERS; i++) {
        // players can't be hurt on title map
        if(map->forcegodmode) {
            players[i].cheats |= CF_GODMODE;
        }
        // turn off godmode on hectic map
        else if(map->clearchts) {
            players[i].cheats &= ~CF_GODMODE;
        }
        else {
            break;
        }
    }

    // turn off/clear some things
    AM_Reset();
    AM_Stop();
    M_ClearRandom();

    // do a nice little fade in effect
    P_FadeInBrightness();

    // autoactivate line specials
    P_ActivateLineByTag(999, players[0].mo);

    // enable menu and set gamestate
    allowmenu = true;
    gamestate = GS_LEVEL;

    S_StartMusic(map->music);
}

//
// P_Stop
//

void P_Stop(void) {
    int i = 0;
    int action = gameaction;

    //
    // [d64] stop plasma buzz
    //
    S_StopSound(NULL, sfx_electric);

    for(i = 0; i < MAXPLAYERS; i++) {
        // take away cards and stuff
        if(playeringame[i]) {
            G_PlayerFinishLevel(i);
        }
    }

    // [kex] reset damage indicators
    if(p_damageindicator.value) {
        ST_ClearDamageMarkers();
    }

    // free level tags
    Z_FreeTags(PU_LEVEL, PU_PURGELEVEL-1);

    if(automapactive) {
        AM_Stop();
    }

    // music continues on exit if defined
    if(!P_GetMapInfo(gamemap)->contmusexit) {
        S_StopMusic();
    }

    // end iwad demo playback here
    if(demoplayback && iwadDemo) {
        demoplayback = false;
        iwadDemo = false;
    }

    // do wipe/melt effect
    if(gameaction != ga_loadgame) {
        if(r_wipe.value) {
            if(gameaction != ga_warpquick) {
                WIPE_MeltScreen();
            }
            else {
                S_StopMusic();
                WIPE_FadeScreen(8);
            }
        }
        else {
            if(gameaction == ga_warpquick) {
                S_StopMusic();
            }
        }
    }

    S_ResetSound();

    // action is warpquick only because the user
    // cancelled demo playback...
    // boot the user back to the title screen
    if(gameaction == ga_warpquick && demoplayback) {
        gameaction = ga_title;
        demoplayback = false;
    }
    else {
        gameaction = action;
    }
}

//
// P_Drawer
//

void P_Drawer(void) {
    // [kex] don't draw on first tic
    if(!leveltime) {
        return;
    }

    GL_ClearView(0xFF000000);

    if(!automapactive || am_overlay.value) {
        R_RenderPlayerView(&players[displayplayer]);
    }

    AM_Drawer();
    ST_Drawer();
}

//
// P_Ticker
//

int P_Ticker(void) {
    int i;

    if(i_interpolateframes.value) {
        P_UpdateFrameStates();
    }

    if(paused) {
        return 0;
    }

    // pause if in menu and at least one tic has been run
    if(!netgame && menuactive &&
            !demoplayback && players[consoleplayer].viewz != 1) {
        return 0;
    }

    for(i = 0; i < MAXPLAYERS; i++) {
        if(playeringame[i]) {
            // do player reborns if needed
            if(players[i].playerstate == PST_REBORN) {
                G_DoReborn(i);
            }

            P_PlayerThink(&players[i]);
        }
    }

    P_RunThinkers();
    P_ScanSights();
    P_RunMobjs();
    P_UpdateSpecials();
    P_RunMacros();

    ST_Ticker();
    AM_Ticker();

    // for par times
    leveltime++;

    return gameaction;
}

