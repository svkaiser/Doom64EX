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
//      Intermission screens.
//
//-----------------------------------------------------------------------------

#include <stdio.h>

#include "z_zone.h"
#include "g_game.h"
#include "s_sound.h"
#include "doomstat.h"
#include "sounds.h"
#include "wi_stuff.h"
#include "d_englsh.h"
#include "m_password.h"
#include "p_setup.h"
#include "st_stuff.h"
#include "r_wipe.h"
#include "gl_draw.h"

#define WIALPHARED      D_RGBA(0xC0, 0, 0, 0xFF)

static int itempercent[MAXPLAYERS];
static int itemvalue[MAXPLAYERS];

static int killpercent[MAXPLAYERS];
static int killvalue[MAXPLAYERS];

static int secretpercent[MAXPLAYERS];
static int secretvalue[MAXPLAYERS];

static char timevalue[16];

static int wi_stage = 0;
static int wi_counter = 0;
static int wi_advance = 0;

//
// WI_Start
//

void WI_Start(void) {
    int i;
    int minutes = 0;
    int seconds = 0;

    // initialize player stats
    for(i = 0; i < MAXPLAYERS; i++) {
        itemvalue[i] = killvalue[i] = secretvalue[i] = -1;

        if(!totalkills) {
            killpercent[i] = 100;
        }
        else {
            killpercent[i] = (players[i].killcount * 100) / totalkills;
        }

        if(!totalitems) {
            itempercent[i] = 100;
        }
        else {
            itempercent[i] = (players[i].itemcount * 100) / totalitems;
        }

        if(!totalsecret) {
            secretpercent[i] = 100;
        }
        else {
            secretpercent[i] = (players[i].secretcount * 100) / totalsecret;
        }
    }

    // setup level time
    if(leveltime) {
        minutes = (leveltime / TICRATE) / 60;
        seconds = (leveltime / TICRATE) % 60;
    }

    dsnprintf(timevalue, 16, "%2.2d:%2.2d", minutes, seconds);

    // generate password
    if(nextmap < 34) {
        M_EncodePassword();
    }

    // clear variables
    wi_counter = 0;
    wi_stage = 0;
    wi_advance = 0;

    // start music
    S_StartMusic(mus_complete);

    allowmenu = true;
}

//
// WI_Stop
//

void WI_Stop(void) {
    wi_counter = 0;
    wi_stage = 0;
    wi_advance = 0;

    allowmenu = false;

    S_StopMusic();
    WIPE_FadeScreen(6);
}

//
// WI_Ticker
//

int WI_Ticker(void) {
    dboolean    state = false;
    player_t*   player;
    int         i;
    dboolean    next = false;

    if(wi_advance <= 3) {
        // check for button presses to skip delays
        for(i = 0, player = players; i < MAXPLAYERS; i++, player++) {
            if(playeringame[i]) {
                if(player->cmd.buttons & BT_ATTACK) {
                    if(!player->attackdown) {
                        S_StartSound(NULL, sfx_explode);
                        wi_advance++;
                    }
                    player->attackdown = true;
                }
                else {
                    player->attackdown = false;
                }

                if(player->cmd.buttons & BT_USE) {
                    if(!player->usedown) {
                        S_StartSound(NULL, sfx_explode);
                        wi_advance++;
                    }
                    player->usedown = true;
                }
                else {
                    player->usedown = false;
                }
            }
        }
    }

    // accelerate counters
    if(wi_advance == 1) {
        wi_stage = 5;
        wi_counter = 0;
        wi_advance = 2;

        for(i = 0; i < MAXPLAYERS; i++) {
            killvalue[i] = killpercent[i];
            itemvalue[i] = itempercent[i];
            secretvalue[i] = secretpercent[i];
        }
    }

    if(wi_advance == 2) {
        return 0;
    }

    if(wi_advance == 3) {
        //S_StartSound(NULL, sfx_explode);
        wi_advance = 4;
    }

    // fade out, complete world
    if(wi_advance >= 4) {
        clusterdef_t* cluster;
        clusterdef_t* nextcluster;

        cluster = P_GetCluster(gamemap);
        nextcluster = P_GetCluster(nextmap);

        if((nextcluster && cluster != nextcluster && nextcluster->enteronly) ||
            (cluster && cluster != nextcluster && !cluster->enteronly)) {
            return ga_victory;
        }

        return 1;
    }

    if(wi_counter) {
        if((gametic - wi_counter) <= 60) {
            return 0;
        }
    }

    next = true;

    // counter ticks
    switch(wi_stage) {
    case 0:
        S_StartSound(NULL, sfx_explode);
        wi_stage = 1;
        state = false;
        break;

    case 1:     // kills
        for(i = 0; i < MAXPLAYERS; i++) {
            killvalue[i] += 4;
            if(killvalue[i] > killpercent[i]) {
                killvalue[i] = killpercent[i];
            }
            else {
                next = false;
            }
        }

        if(next) {
            S_StartSound(NULL, sfx_explode);

            wi_counter = gametic;
            wi_stage = 2;
        }

        state = true;
        break;

    case 2:     // item
        for(i = 0; i < MAXPLAYERS; i++) {
            itemvalue[i] += 4;
            if(itemvalue[i] > itempercent[i]) {
                itemvalue[i] = itempercent[i];
            }
            else {
                next = false;
            }
        }

        if(next) {
            S_StartSound(NULL, sfx_explode);

            wi_counter = gametic;
            wi_stage = 3;
        }

        state = true;
        break;

    case 3:     // secret
        for(i = 0; i < MAXPLAYERS; i++) {
            secretvalue[i] += 4;
            if(secretvalue[i] > secretpercent[i]) {
                secretvalue[i] = secretpercent[i];
            }
            else {
                next = false;
            }
        }

        if(next) {
            S_StartSound(NULL, sfx_explode);

            wi_counter = gametic;
            wi_stage = 4;
        }

        state = true;
        break;

    case 4:
        if(gamemap > 33 && nextmap > 33) {
            S_StartSound(NULL, sfx_explode);
        }

        wi_counter = gametic;
        wi_stage = 5;
        state = false;
        break;
    }

    if(!wi_advance && !state) {
        if(wi_stage == 5) {
            wi_advance = 1;
        }
    }

    // play sound as counter increases
    if(state && !(gametic & 3)) {
        S_StartSound(NULL, sfx_pistol);
    }

    return 0;
}

//
// WI_Drawer
//

void WI_Drawer(void) {
    int currentmap = gamemap;

    GL_ClearView(0xFF000000);

    if(currentmap < 0) {
        currentmap = 0;
    }

    if(currentmap > 33) {
        currentmap = 33;
    }

    if(wi_advance >= 4) {
        return;
    }

    // draw background
    Draw_GfxImage(63, 25, "EVIL", WHITE, false);

    // draw 'mapname' Finished text
    Draw_BigText(-1, 20, WHITE, P_GetMapInfo(currentmap)->mapname);
    Draw_BigText(-1, 36, WHITE, "Finished");

    if(!netgame) {
        // draw kills
        Draw_BigText(57, 60, WIALPHARED, "Kills");
        Draw_BigText(248, 60, WIALPHARED, "%");
        if(wi_stage > 0 && killvalue[0] > -1) {
            Draw_Number(210, 60, killvalue[0], 1, WIALPHARED);
        }

        // draw items
        Draw_BigText(57, 78, WIALPHARED, "Items");
        Draw_BigText(248, 78, WIALPHARED, "%");
        if(wi_stage > 1 && itemvalue[0] > -1) {
            Draw_Number(210, 78, itemvalue[0], 1, WIALPHARED);
        }

        // draw secrets
        Draw_BigText(57, 99, WIALPHARED, "Secrets");
        Draw_BigText(248, 99, WIALPHARED, "%");
        if(wi_stage > 2 && secretvalue[0] > -1) {
            Draw_Number(210, 99, secretvalue[0], 1, WIALPHARED);
        }

        // draw time
        if(wi_stage > 3) {
            Draw_BigText(57, 120, WIALPHARED, "Time");
            Draw_BigText(210, 120, WIALPHARED, timevalue);
        }
    }
    else {
        int i;
        int y = 160;

        GL_SetOrthoScale(0.5f);

        Draw_BigText(180, 140, WHITE, "Kills");
        Draw_BigText(300, 140, WHITE, "Items");
        Draw_BigText(412, 140, WHITE, "Secrets");

        for(i = 0; i < MAXPLAYERS; i++) {
            if(!playeringame[i]) {
                continue;
            }

            Draw_BigText(57, y, WIALPHARED, player_names[i]);
            Draw_BigText(232, y, WIALPHARED, "%");
            Draw_BigText(352, y, WIALPHARED, "%");
            Draw_BigText(464, y, WIALPHARED, "%");

            if(wi_stage > 0 && killvalue[i] > -1) {
                Draw_Number(180, y, killvalue[i], 1, WIALPHARED);
            }

            if(wi_stage > 1 && itemvalue[i] > -1) {
                Draw_Number(300, y, itemvalue[i], 1, WIALPHARED);
            }

            if(wi_stage > 2 && secretvalue[i] > -1) {
                Draw_Number(412, y, secretvalue[i], 1, WIALPHARED);
            }

            y += 16;
        }

        // draw time
        if(wi_stage > 3) {
            Draw_BigText(248, y + 32, WIALPHARED, "Time");
            Draw_BigText(324, y + 32, WIALPHARED, timevalue);
        }

        GL_SetOrthoScale(1.0f);
    }

    // draw password and name of next map
    if(wi_stage > 4 && (P_GetMapInfo(nextmap) != NULL)) {
        char password[20];
        byte *passData;
        int i = 0;
        int y = 145;

        if(netgame) {
            y = 187;
        }

        Draw_BigText(-1, y, WHITE, "Entering");
        Draw_BigText(-1, y + 16, WHITE, P_GetMapInfo(nextmap)->mapname);

        if(netgame) {  // don't bother drawing the password on netgames
            return;
        }

        Draw_BigText(-1, 187, WHITE, "Password");

        dmemset(password, 0, 20);
        passData = passwordData;

        // draw actual password
        do {
            if(i && !((passData - passwordData) & 3)) {
                password[i++] = 0x20;
            }

            if(i >= 20) {
                break;
            }

            password[i++] = passwordChar[*(passData++)];
        }
        while(i < 20);

        password[19] = 0;

        Draw_BigText(-1, 203, WHITE, password);
    }
}

