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
// DESCRIPTION:  the automap code (new and improved)
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "i_video.h"
#include "z_zone.h"
#include "doomdef.h"
#include "st_stuff.h"
#include "p_local.h"
#include "w_wad.h"
#include "t_bsp.h"
#include "m_cheat.h"
#include "m_fixed.h"
#include "i_system.h"
#include "doomstat.h"
#include "d_englsh.h"
#include "tables.h"
#include "am_map.h"
#include "am_draw.h"
#include "m_misc.h"
#include "m_random.h"
#include "gl_draw.h"
#include "g_actions.h"
#include "g_controls.h"

#ifdef _WIN32
#include "g_controls.h"
#endif

// automap flags

enum {
    AF_PANLEFT      = 1,
    AF_PANRIGHT     = 2,
    AF_PANTOP       = 4,
    AF_PANBOTTOM    = 8,
    AF_ZOOMIN       = 16,
    AF_ZOOMOUT      = 32,
    AF_PANMODE      = 64,
    AF_PANGAMEPAD   = 128
};

// automap vars

int             amCheating      = 0;        //villsa: no longer static..
dboolean        automapactive   = false;
fixed_t         automapx        = 0;
fixed_t         automapy        = 0;
fixed_t         automappanx     = 0;
fixed_t         automappany     = 0;
byte            amModeCycle     = 0;        // textured or line mode?
int             followplayer    = 1;        // specifies whether to follow the player around

static player_t *plr;                       // the player represented by an arrow
static dboolean stopped         = true;
static word     am_blink        = 0;        // player arrow blink tics
static angle_t  automapangle    = 0;
static float    scale           = 640.0f;   // todo: reset scale after changing levels
static fixed_t  am_box[4];                  // automap bounding box of level
static byte     am_flags;                   // action flags for automap. Mostly for controls
static int      mpanx;
static int      mpany;
static angle_t  autoprevangle   = 0;
static fixed_t  automapprevx    = 0;
static fixed_t  automapprevy    = 0;

void AM_Start(void);

// automap cvars

BoolProperty am_lines("am_lines", "Draw map with lines", true);
IntProperty am_nodes("am_nodes", "");
BoolProperty am_ssect("am_ssect", "");
BoolProperty am_fulldraw("am_fulldraw", "");
BoolProperty am_showkeycolors("am_showkeycolors", "Show key colours in automap");
BoolProperty am_showkeymarkers("am_showkeymarkers", "");
BoolProperty am_drawobjects("am_drawobjects", "Show objects in automap");
BoolProperty am_overlay("am_overlay", "Show automap overlay");

extern FloatProperty v_msensitivityx;
extern FloatProperty v_msensitivityy;

#ifdef _USE_XINPUT  // XINPUT
CVAR_EXTERNAL(i_rsticksensitivity);
CVAR_EXTERNAL(i_xinputscheme);
#endif

//
// CMD_Automap
//

static CMD(Automap) {
    if(gamestate != GS_LEVEL) {
        return;
    }

    if(!automapactive) {
        AM_Start();
    }
    else {
        if(++amModeCycle >= 2) {
            amModeCycle = 0;
            AM_Stop();
        }
    }
}

//
// CMD_AutomapSetFlag
//

static CMD(AutomapSetFlag) {
    if(gamestate != GS_LEVEL) {
        return;
    }

    if(!automapactive) {
        return;
    }

    if(data & PCKF_UP) {
        int64 flags = (data ^ PCKF_UP);

        am_flags &= ~flags;

        if(flags & (AF_PANMODE|AF_PANGAMEPAD)) {
            automappany = automappanx = 0;
        }
    }
    else {
        am_flags |= data;
    }
}

//
// CMD_AutomapFollow
//

static CMD(AutomapFollow) {
    if(gamestate != GS_LEVEL) {
        return;
    }

    if(!automapactive) {
        return;
    }

    followplayer = !followplayer;
    plr->message = followplayer ? AMSTR_FOLLOWON : AMSTR_FOLLOWOFF;

    if(!followplayer) {
        automapx = plr->mo->x;
        automapy = plr->mo->y;
        automapangle = plr->mo->angle;
    }
    else {
        automappany = automappanx = 0;
    }
}

//
// AM_Reset
//

void AM_Reset(void) {
    automapx        = 0;
    automapy        = 0;
    automappanx     = 0;
    automappany     = 0;
    amModeCycle     = 0;
    followplayer    = 1;
    stopped         = true;
    am_blink        = 0;
    automapangle    = 0;
    scale           = 640.0f;
    am_flags        = 0;
    mpanx           = 0;
    mpany           = 0;
    autoprevangle   = 0;
    automapprevx    = 0;
    automapprevy    = 0;
}

//
// AM_Stop
//

void AM_Stop(void) {
    automapactive = false;
    stopped = true;
    R_RefreshBrightness();
}

//
// AM_Start
//

void AM_Start(void) {
    int pnum;

    if(!stopped) {
        AM_Stop();
    }

    stopped = false;
    automapactive = true;
    amModeCycle = 0;
    am_flags = 0;
    am_blink = 0x5F | 0x100;

    // find player to center on initially
    if(!playeringame[pnum = consoleplayer]) {
        for(pnum=0; pnum<MAXPLAYERS; pnum++) {
            if(playeringame[pnum]) {
                break;
            }
        }
    }

    plr = &players[pnum];

    automapx = plr->mo->x;
    automapy = plr->mo->y;
    automapangle = plr->mo->angle;
}

//
// AM_GetBounds
// Get a dynamically resizable bounding box
// of the automap
//

static void AM_GetBounds(void) {
    int i;
    fixed_t block[8];
    fixed_t x;
    fixed_t y;
    fixed_t x1;
    fixed_t y1;
    fixed_t x2;
    fixed_t y2;
    fixed_t bx;
    fixed_t by;

    // need to manually flip the origins based on player's angle otherwise
    // the bounding box will get screwed up.

    if(automapangle >= ANG90 && automapangle <= -ANG90) {
        block[0] = ((bmapwidth<<MAPBLOCKSHIFT) + bmaporgx);
        block[1] = bmaporgy;
        block[2] = bmaporgx;
        block[3] = bmaporgy;
        block[4] = bmaporgx;
        block[5] = ((bmapheight<<MAPBLOCKSHIFT) + bmaporgy);
        block[6] = ((bmapwidth<<MAPBLOCKSHIFT) + bmaporgx);
        block[7] = ((bmapheight<<MAPBLOCKSHIFT) + bmaporgy);
    }
    else {
        block[0] = bmaporgx;
        block[1] = ((bmapheight<<MAPBLOCKSHIFT) + bmaporgy);
        block[2] = ((bmapwidth<<MAPBLOCKSHIFT) + bmaporgx);
        block[3] = ((bmapheight<<MAPBLOCKSHIFT) + bmaporgy);
        block[4] = ((bmapwidth<<MAPBLOCKSHIFT) + bmaporgx);
        block[5] = bmaporgy;
        block[6] = bmaporgx;
        block[7] = bmaporgy;
    }

    M_ClearBox(am_box);

    for(i = 0; i < 8; i+=2) {
        bx = (block[i] - automapx) / FRACUNIT;
        by = (block[i+1] - automapy) / FRACUNIT;

        x1 = bx * finecosine[(ANG90 - automapangle) >> ANGLETOFINESHIFT];
        y1 = by * finesine[(ANG90 - automapangle) >> ANGLETOFINESHIFT];
        x2 = bx * finesine[(ANG90 - automapangle) >> ANGLETOFINESHIFT];
        y2 = by * finecosine[(ANG90 - automapangle) >> ANGLETOFINESHIFT];

        x = (x1 - y1) + automapx;
        y = (x2 + y2) + automapy;

        M_AddToBox(am_box, x, y);
    }
}


//
// AM_Responder
// Handle events (user inputs) in automap mode
//

dboolean AM_Responder(event_t* ev) {
    int rc = false;

    if(am_flags & AF_PANMODE) {
        if(ev->type == ev_mouse) {
            mpanx = ev->data2;
            mpany = ev->data3;
            rc = true;
        }
        else {
            if(ev->type == ev_mousedown && ev->data1) {
                if(ev->data1 & 1) {
                    am_flags &= ~AF_ZOOMOUT;
                    am_flags |= AF_ZOOMIN;
                    rc = true;
                }
                else if(ev->data1 & 4) {
                    am_flags &= ~AF_ZOOMIN;
                    am_flags |= AF_ZOOMOUT;
                    rc = true;
                }
            }
            else {
                am_flags &= ~AF_ZOOMOUT;
                am_flags &= ~AF_ZOOMIN;
            }
        }
    }
#ifdef _USE_XINPUT  // XINPUT

    else if(ev->type == ev_gamepad) {
        //
        // user has pan button held down and is
        // moving around with the stick
        //
        if(am_flags & AF_PANGAMEPAD) {
            if(ev->data3 == XINPUT_GAMEPAD_LEFT_STICK) {
                float x;
                float y;

                x = (float)ev->data1 * i_rsticksensitivity.value / (1500.0f / scale);
                y = (float)ev->data2 * i_rsticksensitivity.value / (1500.0f / scale);

                mpanx = (int)x << 16;
                mpany = (int)y << 16;

                rc = true;
            }
        }
    }
    else if(automapactive) {
        if(ev->type == ev_keydown) {
            switch(ev->data1) {
            //
            // pan button
            //
            case BUTTON_A:
                CMD_AutomapSetFlag(AF_PANGAMEPAD, NULL);
                break;

            case BUTTON_LEFT_SHOULDER:
                if(am_flags & AF_PANGAMEPAD) {
                    CMD_AutomapSetFlag(AF_ZOOMIN, NULL);
                    rc = true;
                }
                break;

            case BUTTON_RIGHT_SHOULDER:
                if(am_flags & AF_PANGAMEPAD) {
                    CMD_AutomapSetFlag(AF_ZOOMOUT, NULL);
                    rc = true;
                }
                break;

            case BUTTON_DPAD_UP:
                if(am_flags & AF_PANGAMEPAD) {
                    CMD_AutomapSetFlag(AF_PANTOP, NULL);
                    rc = true;
                }
                break;

            case BUTTON_DPAD_DOWN:
                if(am_flags & AF_PANGAMEPAD) {
                    CMD_AutomapSetFlag(AF_PANBOTTOM, NULL);
                    rc = true;
                }
                break;

            case BUTTON_DPAD_LEFT:
                if(am_flags & AF_PANGAMEPAD) {
                    CMD_AutomapSetFlag(AF_PANLEFT, NULL);
                    rc = true;
                }
                break;

            case BUTTON_DPAD_RIGHT:
                if(am_flags & AF_PANGAMEPAD) {
                    CMD_AutomapSetFlag(AF_PANRIGHT, NULL);
                    rc = true;
                }
                break;
            }
        }
        else if(ev->type == ev_keyup) {
            switch(ev->data1) {
            case BUTTON_A:
                CMD_AutomapSetFlag(AF_PANGAMEPAD|PCKF_UP, NULL);
                break;

            case BUTTON_LEFT_SHOULDER:
                CMD_AutomapSetFlag(AF_ZOOMIN|PCKF_UP, NULL);
                break;

            case BUTTON_RIGHT_SHOULDER:
                CMD_AutomapSetFlag(AF_ZOOMOUT|PCKF_UP, NULL);
                break;

            case BUTTON_DPAD_UP:
                CMD_AutomapSetFlag(AF_PANTOP|PCKF_UP, NULL);
                break;

            case BUTTON_DPAD_DOWN:
                CMD_AutomapSetFlag(AF_PANBOTTOM|PCKF_UP, NULL);
                break;

            case BUTTON_DPAD_LEFT:
                CMD_AutomapSetFlag(AF_PANLEFT|PCKF_UP, NULL);
                break;

            case BUTTON_DPAD_RIGHT:
                CMD_AutomapSetFlag(AF_PANRIGHT|PCKF_UP, NULL);
                break;
            }
        }
    }
#endif

    return rc;

}

//
// AM_Ticker
// Updates on Game Tick
//

void AM_Ticker(void) {
    fixed_t speed;
    fixed_t oldautomapx;
    fixed_t oldautomapy;

    if(!automapactive) {
        return;
    }

    AM_GetBounds();

    if(am_flags & AF_ZOOMOUT) {
        scale += 32.0f;
        if(scale > 1500.0f) {
            scale = 1500.0f;
        }
    }
    if(am_flags & AF_ZOOMIN) {
        scale -= 32.0f;
        if(scale < 200.0f) {
            scale = 200.0f;
        }
    }

    speed = (int)(scale / 16) * FRACUNIT;
    oldautomapx = automappanx;
    oldautomapy = automappany;

    if(followplayer) {
        if(am_flags & AF_PANMODE) {
            int panscalex = (int)(v_msensitivityx / (1500.0f / scale));
            int panscaley = (int)(v_msensitivityy / (1500.0f / scale));

            automappanx += ((I_MouseAccel(mpanx)*panscalex)/128) << 16;
            automappany += ((I_MouseAccel(mpany)*panscaley)/128) << 16;
            mpanx = mpany = 0;
        }
        else {
            automapx = plr->mo->x;
            automapy = plr->mo->y;
            automapangle = plr->mo->angle;
        }
    }

#ifdef _USE_XINPUT  // XINPUT

    if(am_flags & AF_PANGAMEPAD) {
        automappanx += mpanx;
        automappany += mpany;
        mpanx = mpany = 0;
    }
    else {
        automapx = plr->mo->x;
        automapy = plr->mo->y;
        automapangle = plr->mo->angle;
    }

#endif

    if((!followplayer || (am_flags & AF_PANGAMEPAD)) &&
            am_flags & (AF_PANLEFT|AF_PANRIGHT|AF_PANTOP|AF_PANBOTTOM)) {
        if(am_flags & AF_PANTOP) {
            automappany += speed;
        }

        if(am_flags & AF_PANLEFT) {
            automappanx -= speed;
        }

        if(am_flags & AF_PANRIGHT) {
            automappanx += speed;
        }

        if(am_flags & AF_PANBOTTOM) {
            automappany -= speed;
        }
    }

    //
    // check bounding box collision
    //

    if(am_box[BOXRIGHT] < (automappanx+automapx)) {
        automappanx = oldautomapx;
    }
    else if((automappanx+automapx) < am_box[BOXLEFT]) {
        automappanx = oldautomapx;
    }

    if(am_box[BOXTOP] < (automappany+automapy)) {
        automappany = oldautomapy;
    }
    else if((automappany+automapy) < am_box[BOXBOTTOM]) {
        automappany = oldautomapy;
    }

    //
    // blinking tics
    //

    if(am_blink & 0x100) {
        if((am_blink & 0xff) == 0xff) {
            am_blink = 0xff;
        }
        else {
            am_blink += 0x10;
        }
    }
    else {
        if(am_blink < 0x5F) {
            am_blink = 0x5F | 0x100;
        }
        else {
            am_blink -= 0x10;
        }
    }
}

//
// AM_DrawMapped
//

static void AM_DrawMapped(void) {
    int i;

    //
    // draw textured subsectors for automap
    //
    AM_DrawLeafs(scale);

    //
    // draw white outlines around the subsectors for overlay mode
    //
    if(am_overlay) {
        fixed_t x1;
        fixed_t x2;
        fixed_t y1;
        fixed_t y2;
        seg_t* seg;
        int j;
        int p;

        for(j = 0; j < numsubsectors; j++) {
            if(subsectors[j].sector->flags & MS_HIDESSECTOR) {
                continue;
            }

            for(p = 0; p < subsectors[j].numlines; p++) {
                seg = &segs[subsectors[j].firstline + p];

                //
                // if the subsector has at least one seg that is mapped, then
                // draw the white outline for the entire subsector
                //
                if(seg->linedef->flags & ML_MAPPED ||
                        (plr->powers[pw_allmap] || amCheating)) {
                    for(i = 0; i < subsectors[j].numlines; i++) {
                        seg = &segs[subsectors[j].firstline + i];

                        if(!(seg->linedef->flags & ML_SECRET) && seg->linedef->backsector) {
                            continue;
                        }

                        x1 = seg->linedef->v1->x;
                        y1 = seg->linedef->v1->y;
                        x2 = seg->linedef->v2->x;
                        y2 = seg->linedef->v2->y;

                        AM_DrawLine(x1, x2, y1, y2, scale, WHITE);
                    }

                    //
                    // continue to next subsector
                    //
                    break;
                }
            }
        }
    }
}

//
// AM_DrawNodes
//

static void AM_DrawNodes(void) {
    int i;
    int x1, x2, y1, y2;
    node_t*    node;
    int        side;
    int        nodenum;

    nodenum = numnodes-1;

    if(am_nodes >= 4) {
        while(!(nodenum & NF_SUBSECTOR)) {
            node = &nodes[nodenum];
            side = R_PointOnSide(plr->mo->x, plr->mo->y, node);
            nodenum = node->children[side];
        }
    }

    for(i = 0; i < numnodes; i++) {
        if(am_nodes < 4) {
            node = &nodes[i];

            if(am_nodes == 1 || am_nodes >= 3) {
                x1 = node->bbox[0][BOXLEFT];
                y1 = node->bbox[0][BOXTOP];
                x2 = node->bbox[0][BOXRIGHT];
                y2 = node->bbox[0][BOXTOP];

                AM_DrawLine(x1, x2, y1, y2, scale, 0x00FFFFFF);

                x1 = node->bbox[0][BOXRIGHT];
                y1 = node->bbox[0][BOXTOP];
                x2 = node->bbox[0][BOXRIGHT];
                y2 = node->bbox[0][BOXBOTTOM];

                AM_DrawLine(x1, x2, y1, y2, scale, 0x00FFFFFF);

                x1 = node->bbox[0][BOXRIGHT];
                y1 = node->bbox[0][BOXBOTTOM];
                x2 = node->bbox[0][BOXLEFT];
                y2 = node->bbox[0][BOXBOTTOM];

                AM_DrawLine(x1, x2, y1, y2, scale, 0x00FFFFFF);

                x1 = node->bbox[0][BOXLEFT];
                y1 = node->bbox[0][BOXBOTTOM];
                x2 = node->bbox[0][BOXLEFT];
                y2 = node->bbox[0][BOXTOP];

                AM_DrawLine(x1, x2, y1, y2, scale, 0x00FFFFFF);

                x1 = node->bbox[1][BOXLEFT];
                y1 = node->bbox[1][BOXTOP];
                x2 = node->bbox[1][BOXRIGHT];
                y2 = node->bbox[1][BOXTOP];

                AM_DrawLine(x1, x2, y1, y2, scale, 0x00FF00FF);

                x1 = node->bbox[1][BOXRIGHT];
                y1 = node->bbox[1][BOXTOP];
                x2 = node->bbox[1][BOXRIGHT];
                y2 = node->bbox[1][BOXBOTTOM];

                AM_DrawLine(x1, x2, y1, y2, scale, 0x00FF00FF);

                x1 = node->bbox[1][BOXRIGHT];
                y1 = node->bbox[1][BOXBOTTOM];
                x2 = node->bbox[1][BOXLEFT];
                y2 = node->bbox[1][BOXBOTTOM];

                AM_DrawLine(x1, x2, y1, y2, scale, 0x00FF00FF);

                x1 = node->bbox[1][BOXLEFT];
                y1 = node->bbox[1][BOXBOTTOM];
                x2 = node->bbox[1][BOXLEFT];
                y2 = node->bbox[1][BOXTOP];

                AM_DrawLine(x1, x2, y1, y2, scale, 0x00FF00FF);
            }

            if(am_nodes == 2 || am_nodes >= 3) {
                x1 = node->x;
                y1 = node->y;
                x2 = (node->x + node->dx);
                y2 = (node->y + node->dy);

                AM_DrawLine(x1, x2, y1, y2, scale, 0xFFFF00FF);
            }
        }

        if(am_nodes >= 4) {
            break;
        }
    }
}

//
// AM_DrawWalls
// Determines visible lines, draws them.
// This is LineDef based, not LineSeg based.
//

void AM_DrawWalls(void) {
    int i;
    int x1, x2, y1, y2;

    for(i = 0; i < numlines; i++) {
        line_t *l;

        l = &lines[i];

        x1 = l->v1->x;
        y1 = l->v1->y;
        x2 = l->v2->x;
        y2 = l->v2->y;

        //
        // 20120208 villsa - re-ordered flag checks to match original game
        //

        if(l->flags & ML_DONTDRAW) {
            continue;
        }

        if((l->flags & ML_MAPPED) || am_fulldraw || plr->powers[pw_allmap] || amCheating) {
            rcolor color = D_RGBA(0x8A, 0x5C, 0x30, 0xFF);  // default color

            //
            // check for cheats
            //
            if((plr->powers[pw_allmap] || amCheating) && !(l->flags & ML_MAPPED)) {
                color = D_RGBA(0x80, 0x80, 0x80, 0xFF);
            }
            //
            // check for secret line
            //
            else if(l->flags & ML_SECRET) {
                color = D_RGBA(0xA4, 0x00, 0x00, 0xFF);
            }
            //
            // handle special line
            //
            else if(l->special && !(l->flags & ML_HIDEAUTOMAPTRIGGER)) {
                //
                // draw colored doors based on key requirement
                //
                if(am_showkeycolors) {
                    if(l->special & MLU_RED) {
                        color = D_RGBA(0xFF, 0x00, 0x00, 0xFF);
                    }
                    else if(l->special & MLU_BLUE) {
                        color = D_RGBA(0x00, 0x00, 0xFF, 0xFF);
                    }
                    else if(l->special & MLU_YELLOW) {
                        color = D_RGBA(0xFF, 0xFF, 0x00, 0xFF);
                    }
                    else {
                        //
                        // change color to green to avoid confusion with yellow key doors
                        //
                        color = D_RGBA(0x00, 0xCC, 0x00, 0xFF);
                    }
                }
                else {
                    //
                    // default color for special lines
                    //
                    color = D_RGBA(0xCC, 0xCC, 0x00, 0xFF);
                }
            }
            //
            // solid wall?
            //
            else if(!(l->flags & ML_TWOSIDED)) {
                color = D_RGBA(0xA4, 0x00, 0x00, 0xFF);
            }

            AM_DrawLine(x1, x2, y1, y2, scale, color);
        }
    }
}

//
// AM_drawPlayers
//

void AM_drawPlayers(void) {
    int         i;
    player_t*   p;
    byte        flash;

    flash = am_blink & 0xff;

    if(!netgame) {
        AM_DrawTriangle(plr->mo, scale, amModeCycle, 0, flash, 0);
        return;
    }

    else {
        for(i = 0; i < MAXPLAYERS; i++) {
            p = &players[i];

            if((deathmatch && !singledemo) && p != plr) {
                continue;
            }

            if(!playeringame[i]) {
                continue;
            }

            switch(i) {
            case 0:        // Green
                AM_DrawTriangle(p->mo, scale, amModeCycle, 0, flash, 0);
                break;
            case 1:        // Red
                AM_DrawTriangle(p->mo, scale, amModeCycle, flash, 0, 0);
                break;
            case 2:        // Aqua
                AM_DrawTriangle(p->mo, scale, amModeCycle, 0, flash, flash);
                break;
            case 3:        // Blue
                AM_DrawTriangle(p->mo, scale, amModeCycle, 0, 0, flash);
                break;
            }
        }
    }

}

//
// AM_drawThings
//

void AM_drawThings(void) {
    int     i;
    mobj_t*    t;

    for(i = 0; i < numsectors; i++) {
        t = sectors[i].thinglist;

        while(t) {
            //
            // draw thing triangles for automap cheat
            //
            if(amCheating == 2) {
                if(t->type != MT_PLAYER && am_drawobjects != 1) {
                    //
                    // shootable stuff are marked as red while normal things are blue
                    //
                    if(t->flags & MF_SHOOTABLE || t->flags & MF_MISSILE) {
                        AM_DrawTriangle(t, scale, amModeCycle, 164, 0, 0);
                    }
                    else {
                        AM_DrawTriangle(t, scale, amModeCycle, 51, 115, 179);
                    }
                }

                if(am_drawobjects) {
                    AM_DrawSprite(t, scale);
                }
            }
            //
            // draw colored keys and artifacts in automap for new players
            //
            else if(am_showkeymarkers) {
                if(t->type >= MT_ITEM_BLUECARDKEY && t->type <= MT_ITEM_ARTIFACT3) {
                    byte r, g, b;

                    switch(t->type) {
                    case MT_ITEM_BLUECARDKEY:
                    case MT_ITEM_BLUESKULLKEY:
                        r = 0;
                        g = 64;
                        b = 255;
                        break;

                    case MT_ITEM_REDCARDKEY:
                    case MT_ITEM_REDSKULLKEY:
                        r = 255;
                        g = 0;
                        b = 0;
                        break;

                    case MT_ITEM_YELLOWCARDKEY:
                    case MT_ITEM_YELLOWSKULLKEY:
                        r = 255;
                        g = 128;
                        b = 0;
                        break;

                    case MT_ITEM_ARTIFACT1:
                        r = 224;
                        g = 56;
                        b = 0;
                        break;

                    case MT_ITEM_ARTIFACT2:
                        r = 0;
                        g = 200;
                        b = 224;
                        break;

                    case MT_ITEM_ARTIFACT3:
                        r = 120;
                        g = 0;
                        b = 224;
                        break;
                    default:
                        r = g = b = 255;
                        break;
                    }

                    if(am_drawobjects != 1) {
                        AM_DrawTriangle(t, scale, amModeCycle, r, g, b);
                    }

                    if(am_drawobjects) {
                        AM_DrawSprite(t, scale);
                    }
                }
            }

            t = t->snext;
        }
    }
}

//
// AM_Drawer
//

void AM_Drawer(void) {
    fixed_t x;
    fixed_t y;
    angle_t view;
    dboolean follow = false;

    if(!automapactive) {
        return;
    }

    follow = (followplayer && !(am_flags & AF_PANMODE));

    if(follow) {
        automapprevx = automapx;
        automapprevy = automapy;
        autoprevangle = automapangle;
    }

    view = autoprevangle - ANG90;
    x = automapprevx;
    y = automapprevy;

    if(plr->onground) {
        x += (quakeviewx >> 7);
        y += quakeviewy;
    }

    AM_BeginDraw(view, x, y);

    if(!amModeCycle) {
        AM_DrawMapped();
    }
    else {
        if(am_lines) {
            AM_DrawWalls();
        }
    }

    if(am_nodes) {
        AM_DrawNodes();
    }

    AM_drawPlayers();

    if(amCheating == 2 || am_showkeymarkers) {
        AM_drawThings();
    }

    if(plr->artifacts) {
        int x = 280;

        if(plr->artifacts & (1<<ART_TRIPLE)) {
            Draw_Sprite2D(SPR_ART3, 0, 0, x, 255, 1.0f, 0, WHITEALPHA(0x80));
            x -= 40;
        }

        if(plr->artifacts & (1<<ART_DOUBLE)) {
            Draw_Sprite2D(SPR_ART2, 0, 0, x, 255, 1.0f, 0, WHITEALPHA(0x80));
            x -= 40;
        }

        if(plr->artifacts & (1<<ART_FAST)) {
            Draw_Sprite2D(SPR_ART1, 0, 0, x, 255, 1.0f, 0, WHITEALPHA(0x80));
            x -= 40;
        }
    }

    AM_EndDraw();
}

//
// AM_RegisterCvars
//

void AM_RegisterCvars(void) {
    G_AddCommand("automap", CMD_Automap, 0);
    G_AddCommand("+automap_in", CMD_AutomapSetFlag, AF_ZOOMIN);
    G_AddCommand("-automap_in", CMD_AutomapSetFlag, AF_ZOOMIN|PCKF_UP);
    G_AddCommand("+automap_out", CMD_AutomapSetFlag, AF_ZOOMOUT);
    G_AddCommand("-automap_out", CMD_AutomapSetFlag, AF_ZOOMOUT|PCKF_UP);
    G_AddCommand("+automap_left", CMD_AutomapSetFlag, AF_PANLEFT);
    G_AddCommand("-automap_left", CMD_AutomapSetFlag, AF_PANLEFT|PCKF_UP);
    G_AddCommand("+automap_right", CMD_AutomapSetFlag, AF_PANRIGHT);
    G_AddCommand("-automap_right", CMD_AutomapSetFlag, AF_PANRIGHT|PCKF_UP);
    G_AddCommand("+automap_up", CMD_AutomapSetFlag, AF_PANTOP);
    G_AddCommand("-automap_up", CMD_AutomapSetFlag, AF_PANTOP|PCKF_UP);
    G_AddCommand("+automap_down", CMD_AutomapSetFlag, AF_PANBOTTOM);
    G_AddCommand("-automap_down", CMD_AutomapSetFlag, AF_PANBOTTOM|PCKF_UP);
    G_AddCommand("+automap_freepan", CMD_AutomapSetFlag, AF_PANMODE);
    G_AddCommand("-automap_freepan", CMD_AutomapSetFlag, AF_PANMODE|PCKF_UP);
    G_AddCommand("automap_follow", CMD_AutomapFollow, 0);
}

