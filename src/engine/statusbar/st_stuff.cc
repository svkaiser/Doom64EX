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
//  Status bar code.
//  Does the face/direction indicator animatin.
//  Does palette indicators as well (red pain/berserk, bright pickup)
//    Handles hud and chat messages
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include "doomdef.h"
#include "g_game.h"
#include "st_stuff.h"
#include "r_main.h"
#include "p_local.h"
#include "m_cheat.h"
#include "s_sound.h"
#include "doomstat.h"
#include "d_englsh.h"
#include "sounds.h"
#include "m_shift.h"
#include "con_console.h"
#include "i_system.h"
#include "am_map.h"
#include "gl_texture.h"
#include "g_actions.h"
#include "z_zone.h"
#include "p_setup.h"
#include "gl_draw.h"
#include "g_demo.h"

IntProperty st_drawhud("st_drawhud", "", true);
BoolProperty st_crosshair("st_crosshair", "", false);
FloatProperty st_crosshairopacity("st_crosshairopacity", "", 80.0f);
BoolProperty st_flashoverlay("st_flashoverlay", "", false);
BoolProperty st_regionmsg("st_regionmsg", "", false);
BoolProperty m_messages("m_messages", "", true);
StringProperty m_playername("m_playername", "");
BoolProperty st_showpendingweapon("st_showpendingweapon", "", true);
BoolProperty st_showstats("st_showstats", "", false);

extern BoolProperty p_usecontext;
extern BoolProperty p_damageindicator;
extern BoolProperty r_texturecombiner;

//
// STATUS BAR DATA
//

typedef struct {
    dboolean    active;
    dboolean    doDraw;
    int         delay;
    int         times;
} keyflash_t;

static keyflash_t flashCards[NUMCARDS];    /* INFO FOR FLASHING CARDS & SKULLS */

#define    FLASHDELAY      8       /* # of tics delay (1/30 sec) */
#define FLASHTIMES      6       /* # of times to flash new frag amount (EVEN!) */

#define ST_HEALTHTEXTX  29
#define ST_HEALTHTEXTY  203

#define ST_ARMORTEXTX   253
#define ST_ARMORTEXTY   203

#define ST_KEYX         78
#define ST_KEYY         216

#define ST_JMESSAGES    45

#define ST_MSGTIMEOUT   (5*TICRATE)
#define ST_MSGFADESTART (ST_MSGTIMEOUT - (1*TICRATE))
#define ST_MSGFADETIME  5
#define ST_MSGCOLOR(x)  (D_RGBA(255, 255, 255, x))


static player_t*        plyr;   // main player in game
static int              st_msgtic = 0;
static int              st_msgalpha = 0xff;
static const char*      st_msg = NULL;
static vtx_t            st_vtx[32];
static int              st_vtxcount = 0;
static byte             st_flash_r;
static byte             st_flash_g;
static byte             st_flash_b;
static byte             st_flash_a;
static int              st_jmessages[ST_JMESSAGES];   // japan-specific messages
static dboolean         st_hasjmsg = false;
static dboolean         st_wpndisplay_show;
static byte             st_wpndisplay_alpha;
static int              st_wpndisplay_ticks;

const char* chat_macros[] = {
    HUSTR_CHATMACRO0,
    HUSTR_CHATMACRO1,
    HUSTR_CHATMACRO2,
    HUSTR_CHATMACRO3,
    HUSTR_CHATMACRO4,
    HUSTR_CHATMACRO5,
    HUSTR_CHATMACRO6,
    HUSTR_CHATMACRO7,
    HUSTR_CHATMACRO8,
    HUSTR_CHATMACRO9
};

char player_names[MAXPLAYERS][MAXPLAYERNAME] = {
    HUSTR_PLR1,
    HUSTR_PLR2,
    HUSTR_PLR3,
    HUSTR_PLR4
};

static const rcolor st_chatcolors[MAXPLAYERS] = {
    D_RGBA(192, 255, 192, 255),
    D_RGBA(255, 192, 192, 255),
    D_RGBA(128, 255, 192, 255),
    D_RGBA(192, 192, 255, 255),
};

#define MAXCHATNODES    4
#define MAXCHATTIME     256
#define MAXCHATSIZE     256
#define STCHATX         32
#define STCHATY         384

typedef struct {
    char msg[MAXCHATSIZE];
    int tics;
    rcolor color;
} stchat_t;

static stchat_t stchat[MAXCHATNODES];
static int st_chatcount = 0;
dboolean st_chatOn = false;
static char st_chatstring[MAXPLAYERS][MAXCHATSIZE];

#define STQUEUESIZE        256
static int st_chathead;
static int st_chattail;
static byte st_chatqueue[STQUEUESIZE];

#define ST_CROSSHAIRSIZE    32
int st_crosshairs = 0;

static void ST_DrawChatText(void);
static void ST_EatChatMsg(void);
static void ST_DisplayName(int playernum);

//
// STATUS BAR CODE
//

//
// DAMAGE MARKER SYSTEM
//

typedef struct damagemarker_s {
    struct damagemarker_s*  prev;
    struct damagemarker_s*  next;
    int     tics;
    mobj_t* source;
} damagemarker_t;

static damagemarker_t dmgmarkers;

//
// ST_RunDamageMarkers
//

static void ST_RunDamageMarkers(void) {
    damagemarker_t* dmgmarker;

    for(dmgmarker = dmgmarkers.next; dmgmarker != &dmgmarkers; dmgmarker = dmgmarker->next) {
        if(!dmgmarker->tics--) {
            damagemarker_t* marker = dmgmarker;
            damagemarker_t* next = marker->next;

            P_SetTarget(&marker->source, NULL);

            (next->prev = dmgmarker = marker->prev)->next = next;
            Z_Free(marker);
        }
    }
}

//
// ST_ClearDamageMarkers
//

void ST_ClearDamageMarkers(void) {
    dmgmarkers.next = dmgmarkers.prev = &dmgmarkers;
}

//
// ST_AddDamageMarker
//

void ST_AddDamageMarker(mobj_t* target, mobj_t* source) {
    damagemarker_t* dmgmarker;

    if(target->player != &players[consoleplayer]) {
        return;
    }

    dmgmarker               = (damagemarker_t*) Z_Calloc(sizeof(*dmgmarker), PU_LEVEL, 0);
    dmgmarker->tics         = 32;
    P_SetTarget(&dmgmarker->source, source);

    dmgmarkers.prev->next   = dmgmarker;
    dmgmarker->next         = &dmgmarkers;
    dmgmarker->prev         = dmgmarkers.prev;
    dmgmarkers.prev         = dmgmarker;
}

//
// ST_DrawDamageMarkers
//

static void ST_DrawDamageMarkers(void) {
    damagemarker_t* dmgmarker;

    for(dmgmarker = dmgmarkers.next; dmgmarker != &dmgmarkers; dmgmarker = dmgmarker->next) {
        static vtx_t v[3];
        player_t* p;
        float angle;
        byte alpha;

        GL_SetState(GLSTATE_BLEND, 1);
        GL_SetOrtho(0);

        alpha = (dmgmarker->tics << 3);

        if(alpha < 0) {
            alpha = 0;
        }

        v[0].x = -8;
        v[0].a = alpha;
        v[1].x = 8;
        v[1].a = alpha;
        v[2].y = 4;
        v[2].r = 255;
        v[2].a = alpha;

        p = &players[consoleplayer];

        angle = (float)TRUEANGLES(p->mo->angle -
                                  R_PointToAngle2(dmgmarker->source->x, dmgmarker->source->y,
                                          p->mo->x, p->mo->y));

        dglPushMatrix();
        dglTranslatef(160, 120, 0);
        dglRotatef(angle, 0.0f, 0.0f, 1.0f);
        dglTranslatef(0, 16, 0);
        dglDisable(GL_TEXTURE_2D);
        dglSetVertex(v);
        dglTriangle(0, 1, 2);
        dglDrawGeometry(3, v);
        dglEnable(GL_TEXTURE_2D);
        dglPopMatrix();

        GL_ResetViewport();
        GL_SetState(GLSTATE_BLEND, 0);
    }
}

//
// ST_DisplayPendingWeapon
//

void ST_DisplayPendingWeapon(void) {
    st_wpndisplay_show = true;
    st_wpndisplay_alpha = 0xC0;
    st_wpndisplay_ticks = 2*TICRATE;
}

//
// ST_DrawPendingWeapon
//

static void ST_DrawPendingWeapon(void) {
    int i;
    int x = 0;
    int wpn;

    if(!st_wpndisplay_show) {
        return;
    }

    GL_SetOrthoScale(0.5f);

    for(i = 0; i < NUMWEAPONS; i++) {
        rcolor color;

        if(plyr->weaponowned[i]) {
            color = D_RGBA(0xff, 0xff, 0x3f, st_wpndisplay_alpha);
        }
        else {
            color = WHITEALPHA(st_wpndisplay_alpha);
        }

        Draw_Number(245 + x, 400, i, 0, color);
        x += 16;

    }

    if(plyr->pendingweapon == wp_nochange) {
        wpn = plyr->readyweapon;
    }
    else {
        wpn = plyr->pendingweapon;
    }

    Draw_BigText(235 + (wpn * 16), 404, WHITEALPHA(st_wpndisplay_alpha), "/b");

    GL_SetOrthoScale(1.0f);
}

//
// ST_ClearMessage
//

void ST_ClearMessage(void) {
    st_msgtic = 0;
    st_msgalpha = 0xff;
    st_msg = NULL;
}

//
// ST_Ticker
//

void ST_Ticker(void) {
    int ind = 0;

    plyr = &players[consoleplayer];

    //
    // keycard stuff
    //

    /* */
    /* Tried to open a CARD or SKULL door? */
    /* */
    for(ind = 0; ind < NUMCARDS; ind++) {
        /* CHECK FOR INITIALIZATION */
        if(plyr->tryopen[ind]) {
            plyr->tryopen[ind] = false;
            flashCards[ind].active = true;
            flashCards[ind].delay = FLASHDELAY;
            flashCards[ind].times = FLASHTIMES+1;
            flashCards[ind].doDraw = false;
        }

        /* MIGHT AS WELL DO TICKING IN THE SAME LOOP! */
        if(flashCards[ind].active && !--flashCards[ind].delay) {
            flashCards[ind].delay = FLASHDELAY;
            flashCards[ind].doDraw ^= 1;

            if(!--flashCards[ind].times) {
                flashCards[ind].active = false;
            }

            if(flashCards[ind].doDraw && flashCards[ind].active) {
                S_StartSound(NULL,sfx_itemup);
            }
        }
    }

    //
    // messages
    //
    if(plyr->message) {
        CON_Printf(WHITE, "%s\n", plyr->message);

        ST_ClearMessage();
        st_msg = plyr->message;
        plyr->message = NULL;
    }

    if(st_msg || plyr->messagepic >= 0) {
        st_msgtic++;

        if(st_msgtic >= ST_MSGFADESTART) {
            st_msgalpha = MAX((st_msgalpha -= ST_MSGFADETIME), 0);
        }

        if(st_msgtic >= ST_MSGTIMEOUT) {
            ST_ClearMessage();
            plyr->messagepic = -1;
        }
    }

    //
    // flashes
    //
    if(plyr->cameratarget == plyr->mo || !(plyr->cheats & CF_LOCKCAM)) {
        ST_UpdateFlash();
    }

    //
    // chat stuff
    //
    for(ind = 0; ind < MAXCHATNODES; ind++) {
        if(stchat[ind].tics) {
            stchat[ind].tics--;
        }
    }

    ST_EatChatMsg();

    //
    // damage indicator
    //

    if(p_damageindicator) {
        ST_RunDamageMarkers();
    }

    //
    // pending weapon display
    //
    if(st_wpndisplay_show) {
        if(st_wpndisplay_ticks-- <= 0) {
            st_wpndisplay_alpha -= 8;
            if(st_wpndisplay_alpha <= 0) {
                st_wpndisplay_alpha = 0;
                st_wpndisplay_show = false;
            }
        }
    }
}

//
// ST_FlashingScreen
//

void ST_FlashingScreen(byte r, byte g, byte b, byte a) {
    rcolor c = D_RGBA(r, g, b, a);

    GL_SetState(GLSTATE_BLEND, 1);
    GL_SetOrtho(1);

    dglDisable(GL_TEXTURE_2D);
    dglColor4ubv((byte*)&c);
    dglRecti(SCREENWIDTH, SCREENHEIGHT, 0, 0);
    dglEnable(GL_TEXTURE_2D);

    GL_SetState(GLSTATE_BLEND, 0);
}

//
// ST_DrawStatusItem
//

static void ST_DrawStatusItem(const float xy[4][2], const float uv[4][2], rcolor color) {
    int i;

    dglTriangle(st_vtxcount + 0, st_vtxcount + 1, st_vtxcount + 2);
    dglTriangle(st_vtxcount + 0, st_vtxcount + 2, st_vtxcount + 3);

    dglSetVertexColor(st_vtx + st_vtxcount, color, 4);

    for(i = 0; i < 4; i++) {
        st_vtx[st_vtxcount + i].x  = xy[i][0];
        st_vtx[st_vtxcount + i].y  = xy[i][1];
        st_vtx[st_vtxcount + i].tu = uv[i][0];
        st_vtx[st_vtxcount + i].tv = uv[i][1];
    }

    st_vtxcount += 4;
}

//
// ST_DrawKey
//

static void ST_DrawKey(int key, const float uv[4][2], const float xy[4][2]) {
    float keydrawxy[4][2];

    if(plyr->cards[key] ||
            (flashCards[key].doDraw && flashCards[key].active)) {
        dmemcpy(keydrawxy, xy, (sizeof(float)*4) * 2);

        if(st_drawhud >= 2) {
            keydrawxy[0][0] += 20;
            keydrawxy[1][0] += 20;
            keydrawxy[2][0] += 20;
            keydrawxy[3][0] += 20;

            keydrawxy[0][1] += 78;
            keydrawxy[1][1] += 78;
            keydrawxy[2][1] += 78;
            keydrawxy[3][1] += 78;
        }

        ST_DrawStatusItem(keydrawxy, uv, D_RGBA(0xff, 0xff, 0xff, 0x80));
    }
}

//
// ST_DrawStatus
//

static const float st_healthVertex[4][2] = {
    { ST_HEALTHTEXTX, ST_HEALTHTEXTY },
    { ST_HEALTHTEXTX + 40, ST_HEALTHTEXTY },
    { ST_HEALTHTEXTX + 40, ST_HEALTHTEXTY + 6 },
    { ST_HEALTHTEXTX, ST_HEALTHTEXTY + 6 }
};

static const float st_armorVertex[4][2] = {
    { ST_ARMORTEXTX, ST_ARMORTEXTY },
    { ST_ARMORTEXTX + 36, ST_ARMORTEXTY },
    { ST_ARMORTEXTX + 36, ST_ARMORTEXTY + 6 },
    { ST_ARMORTEXTX, ST_ARMORTEXTY + 6 }
};

static const float st_key1Vertex[4][2] = {
    { ST_KEYX, ST_KEYY },
    { ST_KEYX + 9, ST_KEYY },
    { ST_KEYX + 9, ST_KEYY + 10 },
    { ST_KEYX, ST_KEYY + 10 }
};

static const float st_key2Vertex[4][2] = {
    { ST_KEYX + 10, ST_KEYY },
    { ST_KEYX + 19, ST_KEYY },
    { ST_KEYX + 19, ST_KEYY + 10 },
    { ST_KEYX + 10, ST_KEYY + 10 }
};

static const float st_key3Vertex[4][2] = {
    { ST_KEYX + 20, ST_KEYY },
    { ST_KEYX + 29, ST_KEYY },
    { ST_KEYX + 29, ST_KEYY + 10 },
    { ST_KEYX + 20, ST_KEYY + 10 }
};

static void ST_DrawStatus(void) {
    int     lump;
    float   width;
    float   height;
    float   uv[4][2];
    const rcolor color = D_RGBA(0x68, 0x68, 0x68, 0x90);

    GL_SetState(GLSTATE_BLEND, 1);
    lump = GL_BindGfxTexture("STATUS", true);

    width = (float)gfxwidth[lump];
    height = (float)gfxheight[lump];

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

    if(st_drawhud >= 2) {
        GL_SetOrthoScale(0.725f);
    }

    GL_SetOrtho(0);

    dglSetVertex(st_vtx);
    st_vtxcount = 0;

    if(st_drawhud == 1) {
        // health

        uv[0][0] = uv[3][0] = 0.0f;
        uv[0][1] = uv[1][1] = 0.0f;
        uv[1][0] = uv[2][0] = 40.0f / width;
        uv[2][1] = uv[3][1] = 6.0f / height;

        ST_DrawStatusItem(st_healthVertex, uv, color);

        // armor

        uv[0][0] = uv[3][0] = 40.0f / width;
        uv[0][1] = uv[1][1] = 0.0f;
        uv[1][0] = uv[2][0] = uv[0][0] + (36.0f / width);
        uv[2][1] = uv[3][1] = 6.0f / height;

        ST_DrawStatusItem(st_armorVertex, uv, color);
    }

    // cards

    uv[0][0] = uv[3][0] = 0.0f;
    uv[0][1] = uv[1][1] = 6.0f / height;
    uv[1][0] = uv[2][0] = 9.0f / width;
    uv[2][1] = uv[3][1] = 1.0f;

    ST_DrawKey(it_bluecard, uv, st_key1Vertex);

    uv[0][0] = uv[3][0] = 9.0f / width;
    uv[1][0] = uv[2][0] = (9.0f / width) * 2;

    ST_DrawKey(it_yellowcard, uv, st_key2Vertex);

    uv[0][0] = uv[3][0] = (9.0f / width) * 2;
    uv[1][0] = uv[2][0] = (9.0f / width) * 3;

    ST_DrawKey(it_redcard, uv, st_key3Vertex);

    // skulls

    uv[0][0] = uv[3][0] = (9.0f / width) * 3;
    uv[1][0] = uv[2][0] = (9.0f / width) * 4;

    ST_DrawKey(it_blueskull, uv, st_key1Vertex);

    uv[0][0] = uv[3][0] = (9.0f / width) * 4;
    uv[1][0] = uv[2][0] = (9.0f / width) * 5;

    ST_DrawKey(it_yellowskull, uv, st_key2Vertex);

    uv[0][0] = uv[3][0] = (9.0f / width) * 5;
    uv[1][0] = uv[2][0] = (9.0f / width) * 6;

    ST_DrawKey(it_redskull, uv, st_key3Vertex);

    dglDrawGeometry(st_vtxcount, st_vtx);

    GL_ResetViewport();
    GL_SetState(GLSTATE_BLEND, 0);

    if(st_drawhud >= 2) {
        GL_SetOrthoScale(1.0f);
    }
}

//
// ST_DrawCrosshair
//

void ST_DrawCrosshair(int x, int y, int slot, byte scalefactor, rcolor color) {
    float u;
    int index;
    int scale;

    if(slot <= 0) {
        return;
    }

    if(slot > st_crosshairs) {
        return;
    }

    index = slot - 1;

    GL_BindGfxTexture("CRSHAIRS", true);
    GL_SetState(GLSTATE_BLEND, 1);

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

    u = 1.0f / st_crosshairs;
    scale = scalefactor == 0 ? ST_CROSSHAIRSIZE : (ST_CROSSHAIRSIZE / (1 << scalefactor));

    GL_SetupAndDraw2DQuad((float)x, (float)y, scale, scale,
                          u*index, u + (u*index), 0, 1, color, 0);

    GL_SetState(GLSTATE_BLEND, 0);
}

//
// ST_DrawJMessage
//

static void ST_DrawJMessage(int pic) {
    int lump = st_jmessages[pic];

    GL_BindGfxTexture(wad::find(lump)->name.data(), true);
    GL_SetState(GLSTATE_BLEND, 1);

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    GL_SetupAndDraw2DQuad(
        20,
        20,
        gfxwidth[lump],
        gfxheight[lump],
        0,
        1,
        0,
        1,
        ST_MSGCOLOR(automapactive ? 0xff : st_msgalpha),
        false
    );

    GL_SetState(GLSTATE_BLEND, 0);
}

//
// ST_Drawer
//

void ST_Drawer(void) {
    dboolean checkautomap;

    //
    // flash overlay
    //

    if((st_flashoverlay ||
            gl_max_texture_units <= 2 ||
            !r_texturecombiner) && flashcolor) {
        ST_FlashingScreen(st_flash_r, st_flash_g, st_flash_b, st_flash_a);
    }

    if(iwadDemo) {
        return;
    }

    checkautomap = (!automapactive || am_overlay);

    //
    // draw hud
    //

    if(checkautomap && st_drawhud > 0) {
        //Status graphics
        ST_DrawStatus();

        // original hud layout
        if(st_drawhud == 1) {
            //Draw Ammo counter
            if(weaponinfo[plyr->readyweapon].ammo != am_noammo) {
                Draw_Number(160, 215, plyr->ammo[weaponinfo[plyr->readyweapon].ammo], 0, REDALPHA(0x7f));
            }

            //Draw Health
            Draw_Number(49, 215, plyr->health, 0, REDALPHA(0x7f));

            //Draw Armor
            Draw_Number(271, 215, plyr->armorpoints, 0, REDALPHA(0x7f));
        }
        // arranged hud layout
        else if(st_drawhud >= 2) {
            int wpn;

            if(plyr->pendingweapon == wp_nochange) {
                wpn = plyr->readyweapon;
            }
            else {
                wpn = plyr->pendingweapon;
            }

            // display ammo sprite
            switch(weaponinfo[wpn].ammo) {
            case am_clip:
                Draw_Sprite2D(SPR_CLIP, 0, 0, 524, 460, 0.5f, 0, WHITEALPHA(0xC0));
                break;
            case am_shell:
                Draw_Sprite2D(SPR_SHEL, 0, 0, 524, 460, 0.5f, 0, WHITEALPHA(0xC0));
                break;
            case am_misl:
                Draw_Sprite2D(SPR_RCKT, 0, 0, 524, 464, 0.5f, 0, WHITEALPHA(0xC0));
                break;
            case am_cell:
                Draw_Sprite2D(SPR_CELL, 0, 0, 524, 464, 0.5f, 0, WHITEALPHA(0xC0));
                break;
            }

            // display artifact sprites
            if(plyr->artifacts & (1<<ART_TRIPLE)) {
                Draw_Sprite2D(SPR_ART3, 0, 0, 260, 872, 0.275f, 0, WHITEALPHA(0xC0));
            }

            if(plyr->artifacts & (1<<ART_DOUBLE)) {
                Draw_Sprite2D(SPR_ART2, 0, 0, 296, 872, 0.275f, 0, WHITEALPHA(0xC0));
            }

            if(plyr->artifacts & (1<<ART_FAST)) {
                Draw_Sprite2D(SPR_ART1, 0, 0, 332, 872, 0.275f, 0, WHITEALPHA(0xC0));
            }

            // display medkit/armor
            Draw_Sprite2D(SPR_MEDI, 0, 0, 50, 662, 0.35f, 0, WHITEALPHA(0xC0));
            Draw_Sprite2D(SPR_ARM1, 0, 0, 50, 632, 0.35f, 0, WHITEALPHA(0xC0));

            GL_SetOrthoScale(0.5f);

            //Draw Health
            Draw_Number(96, 448, plyr->health, 2, REDALPHA(0xC0));
            Draw_BigText(104, 450, REDALPHA(0xC0), "%");

            //Draw Armor
            Draw_Number(96, 424, plyr->armorpoints, 2, REDALPHA(0xC0));
            Draw_BigText(104, 426, REDALPHA(0xC0), "%");

            //Draw Ammo counter
            if(weaponinfo[wpn].ammo != am_noammo) {
                Draw_Number(550, 448, plyr->ammo[weaponinfo[wpn].ammo], 1, REDALPHA(0xC0));
            }

            GL_SetOrthoScale(1.0f);
        }
    }

    //
    // draw messages
    //

    if(st_hasjmsg && st_regionmsg && plyr->messagepic >= 0) {
        ST_DrawJMessage(plyr->messagepic);
    }
    else if(st_msg && m_messages) {
        Draw_Text(20, 20, ST_MSGCOLOR(automapactive ? 0xff : st_msgalpha), 1, false, "%s", st_msg);
    }
    else if(automapactive) {
        char str[128];
        mapdef_t* map = P_GetMapInfo(gamemap);

        if(map) {
            dmemset(&str, 0, 128);

            if(map->type == 2) {
                sprintf(str, "%s", map->mapname);
            }
            else {
                sprintf(str, "Level %i: %s", gamemap, map->mapname);
            }

            Draw_Text(20, 20, ST_MSGCOLOR(0xff), 1, false, str);
        }
    }

    //
    // draw chat text and player names
    //

    if(netgame) {
        ST_DrawChatText();

        if(checkautomap) {
            int i;

            for(i = 0; i < MAXPLAYERS; i++) {
                if(playeringame[i]) {
                    ST_DisplayName(i);
                }
            }
        }
    }

    //
    // draw crosshairs
    //

    if(st_crosshairs && !automapactive) {
        int x = (SCREENWIDTH / 2) - (ST_CROSSHAIRSIZE / 8);
        int y = (SCREENHEIGHT / 2) - (ST_CROSSHAIRSIZE / 8);
        int alpha = (int)st_crosshairopacity;

        if(alpha > 0xff) {
            alpha = 0xff;
        }

        if(alpha < 0) {
            alpha = 0;
        }

        ST_DrawCrosshair(x, y, (int)st_crosshair, 2, WHITEALPHA(alpha));
    }

    //
    // use action context
    //

    if(p_usecontext) {
        if(P_UseLines(&players[consoleplayer], true)) {
            char usestring[16];
            char contextstring[32];
            float x;

#ifdef _USE_XINPUT  // XINPUT
            if(xgamepad.connected) {
                M_DrawXInputButton(140, 156, XINPUT_GAMEPAD_A);
                Draw_Text(213, 214, WHITEALPHA(0xA0), 0.75, false, "Use");
            }
            else
#endif
            {
                G_GetActionBindings(usestring, "+use");
                sprintf(contextstring, "(%s)Use", usestring);

                x = (160 / 0.75f) - ((dstrlen(contextstring) * 8) / 2);

                Draw_Text((int)x, 214, WHITEALPHA(0xA0), 0.75f, false, contextstring);
            }
        }
    }

    //
    // damage indicator
    //

    if(p_damageindicator) {
        ST_DrawDamageMarkers();
    }

    //
    // display pending weapon
    //

    if(st_showpendingweapon) {
        ST_DrawPendingWeapon();
    }

    //
    // display stats in automap
    //

    if(st_showstats && automapactive) {
        Draw_Text(20, 430, WHITE, 0.5f, false,
                  "Monsters:  %i / %i", plyr->killcount, totalkills);
        Draw_Text(20, 440, WHITE, 0.5f, false,
                  "Items:     %i / %i", plyr->itemcount, totalitems);
        Draw_Text(20, 450, WHITE, 0.5f, false,
                  "Secrets:   %i / %i", plyr->secretcount, totalsecret);
        Draw_Text(20, 460, WHITE, 0.5f, false,
                  "Time:      %2.2d:%2.2d", (leveltime / TICRATE) / 60, (leveltime / TICRATE) % 60);
    }
}

//
// ST_UpdateFlash
//

#define ST_MAXDMGCOUNT  160
#define ST_MAXSTRCOUNT  32
#define ST_MAXBONCOUNT  100

void ST_UpdateFlash(void) {
    player_t* p = &players[consoleplayer];

    flashcolor = 0;

    // invulnerability flash (white)
    if(p->powers[pw_invulnerability] > 61 || (p->powers[pw_invulnerability] & 8)) {
        flashcolor = D_RGBA(128, 128, 128, 0xff);
        st_flash_r = 255;
        st_flash_g = 255;
        st_flash_b = 255;
        st_flash_a = 64;
    }
    // bfg flash (green)
    else if(p->bfgcount) {
        flashcolor = D_RGBA(0, p->bfgcount & 0xff, 0, 0xff);
        st_flash_r = 0;
        st_flash_g = 255;
        st_flash_b = 0;
        st_flash_a = p->bfgcount;
    }
    // damage and strength flash (red)
    else if(p->damagecount || (p->powers[pw_strength] > 1)) {
        int r1 = p->damagecount;
        int r2 = p->powers[pw_strength];

        if(r1) {
            if(r1 > ST_MAXDMGCOUNT) {
                r1 = ST_MAXDMGCOUNT;
            }
        }

        if(r2 == 1) {
            r2 = 0;
        }
        else if(r2 > ST_MAXSTRCOUNT) {
            r2 = ST_MAXSTRCOUNT;
        }

        // take priority based on value
        if(r1 > r2) {
            flashcolor = D_RGBA(r1 & 0xff, 0, 0, 0xff);
            st_flash_r = 255;
            st_flash_g = 0;
            st_flash_b = 0;
            st_flash_a = r1;
        }
        else {
            flashcolor = D_RGBA(r2 & 0xff, 0, 0, 0xff);
            st_flash_r = 255;
            st_flash_g = 0;
            st_flash_b = 0;
            st_flash_a = r2;
        }
    }
    // suit flash (green/yellow)
    else if(p->powers[pw_ironfeet] > 61 || (p->powers[pw_ironfeet] & 8)) {
        flashcolor = D_RGBA(0, 32, 4, 0xff);
        st_flash_r = 0;
        st_flash_g = 255;
        st_flash_b = 31;
        st_flash_a = 64;
    }
    // bonus flash (yellow)
    else if(p->bonuscount) {
        int c1 = (p->bonuscount + 8) >> 3;
        int c2;

        if(c1 > ST_MAXBONCOUNT) {
            c1 = ST_MAXBONCOUNT;
        }

        c2 = (((c1 << 2) + c1) << 1);

        flashcolor = D_RGBA(c2 & 0xff, c2 & 0xff, c1 & 0xff, 0xff);
        st_flash_r = 255;
        st_flash_g = 255;
        st_flash_b = 0;
        st_flash_a = (p->bonuscount + 8) << 1;
    }
}

//
// ST_Init
//

void ST_Init(void) {
    int i = 0;
    int lump;

    plyr = &players[consoleplayer];

    // setup keycards

    for(i = 0; i < NUMCARDS; i++) {
        flashCards[i].active = false;
        players[consoleplayer].tryopen[i] = false;
    }

    // setup hud messages

    ST_ClearMessage();

    // setup player names

    for(i = 0; i < MAXPLAYERS; i++) {
        if(playeringame[i] && net_player_names[i][0]) {
            snprintf(player_names[i], MAXPLAYERNAME, "%s", net_player_names[i]);
        }
    }

    // setup chat text

    for(i = 0; i < MAXCHATNODES; i++) {
        stchat[i].msg[0] = 0;
        stchat[i].tics = 0;
        stchat[i].color = 0;
    }

    dmemset(st_chatstring, 0, MAXPLAYERS * MAXCHATSIZE);
    dmemset(st_chatqueue, 0, STQUEUESIZE);

    // setup crosshairs

    st_crosshairs = 0;

    if(auto lump = wad::find("CRSHAIRS")) {
        st_crosshairs = (gfxwidth[lump->index] / ST_CROSSHAIRSIZE);
    }

    dmgmarkers.next = dmgmarkers.prev = &dmgmarkers;

    // setup region-specific messages

    for(i = 0; i < ST_JMESSAGES; i++) {
        char name[9];

        sprintf(name, "JPMSG%02d", i + 1);
        st_jmessages[i] = wad::find(name)->index;

        if(st_jmessages[i] != -1) {
            st_hasjmsg = true;
        }
    }

    // setup weapon display variables

    st_wpndisplay_show = false;
    st_wpndisplay_alpha = 0;
    st_wpndisplay_ticks = 0;
}

//
// ST_AddChatMsg
//

void ST_AddChatMsg(const char *msg, int player) {
    char str[MAXCHATSIZE];

    sprintf(str, "%s: %s", player_names[player], msg);
    dmemset(stchat[st_chatcount].msg, 0, MAXCHATSIZE);
    memcpy(stchat[st_chatcount].msg, str, dstrlen(str));
    stchat[st_chatcount].tics = MAXCHATTIME;
    stchat[st_chatcount].color = st_chatcolors[player];
    st_chatcount = (st_chatcount + 1) % MAXCHATNODES;

    S_StartSound(NULL, sfx_darthit);
    CON_Printf(WHITE, str);
    CON_Printf(WHITE, "\n");
}

//
// ST_Notification
// Broadcast message to all clients
//

void ST_Notification(const char *msg) {
    int i;

    for(i = 0; i < MAXPLAYERS; i++) {
        if(playeringame[i] && i != consoleplayer) {
            ST_AddChatMsg(msg, i);
        }
    }
}

//
// ST_DrawChatText
//

static void ST_DrawChatText(void) {
    int i;
    int y = STCHATY;
    int current = (st_chatcount - 1);

    if(current < 0) {
        current = (MAXCHATNODES - 1);
    }
    else if(current >= (MAXCHATNODES - 1)) {
        current = 0;
    }

    for(i = 0; i < MAXCHATNODES; i++) {
        if(stchat[current].msg[0] && stchat[current].tics) {

            Draw_Text(STCHATX, y, stchat[current].color, 0.5f, false, stchat[current].msg);
            y -= 8;
        }

        current = (current - 1) % MAXCHATNODES;

        if(current < 0) {
            current = (MAXCHATNODES - 1);
        }
        else if(current >= (MAXCHATNODES - 1)) {
            current = 0;
        }
    }

    if(st_chatOn) {
        char tmp[MAXCHATSIZE];

        sprintf(tmp, "%s_", st_chatstring[consoleplayer]);
        Draw_Text(STCHATX, STCHATY + 8, WHITE, 0.5f, false, tmp);
    }
}

//
// ST_QueueChatChar
//

static void ST_QueueChatChar(char ch) {
    if(((st_chattail+1) & (STQUEUESIZE - 1)) == st_chathead) {
        return;    // the queue is full
    }

    st_chatqueue[st_chattail] = ch;
    st_chattail = ((st_chattail + 1) & (STQUEUESIZE - 1));
}

//
// ST_DequeueChatChar
//

char ST_DequeueChatChar(void) {
    byte temp;

    if(st_chathead == st_chattail) {
        return 0;    // queue is empty
    }

    temp = st_chatqueue[st_chathead];
    st_chathead = ((st_chathead + 1) & (STQUEUESIZE - 1));
    return temp;
}

//
// ST_FeedChatMsg
//

static dboolean st_shiftOn = false;
static void ST_FeedChatMsg(event_t *ev) {
    int c;

    if(!st_chatOn) {
        return;
    }

    if(!(c = ev->data1)) {
        return;
    }

    switch(c) {
        int len;

    case KEY_ENTER:
    case KEY_KEYPADENTER:
        ST_QueueChatChar((char)c);
        st_chatOn = false;
        break;
    case KEY_BACKSPACE:
        if(ev->type != ev_keydown) {
            return;
        }
        ST_QueueChatChar((char)c);
        break;
    case KEY_ESCAPE:
        len = dstrlen(st_chatstring[consoleplayer]);
        st_chatOn = false;
        dmemset(st_chatstring[consoleplayer], 0, len);
        break;
    case KEY_CAPS:
        if(ev->type == ev_keydown) {
            st_shiftOn ^= 1;
        }
        break;
    case KEY_SHIFT:
        if(ev->type == ev_keydown) {
            st_shiftOn = true;
        }
        else if(ev->type == ev_keyup) {
            st_shiftOn = false;
        }
        break;
    case KEY_ALT:
    case KEY_PAUSE:
    case KEY_TAB:
    case KEY_RIGHTARROW:
    case KEY_LEFTARROW:
    case KEY_UPARROW:
    case KEY_DOWNARROW:
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
    case KEY_INSERT:
    case KEY_HOME:
    case KEY_PAGEUP:
    case KEY_PAGEDOWN:
    case KEY_DEL:
    case KEY_END:
    case KEY_SCROLLLOCK:
    case KEY_NUMLOCK:
        break; // too lazy to do anything clever here..
    default:
        if(ev->type != ev_keydown) {
            return;
        }

        if(st_shiftOn) {
            c = shiftxform[c];
        }
        ST_QueueChatChar((char)c);
        break;
    }
}

//
// ST_EatChatMsg
//

static void ST_EatChatMsg(void) {
    int c;
    int len;
    int i;

    for(i = 0; i < MAXPLAYERS; i++) {
        if(!playeringame[i]) {
            continue;
        }
        if(!players[i].cmd.chatchar) {
            continue;
        }

        c = players[i].cmd.chatchar;

        len = dstrlen(st_chatstring[i]);

        switch(c) {
        case KEY_ENTER:
        case KEY_KEYPADENTER:
            ST_AddChatMsg(st_chatstring[i], i);
            dmemset(st_chatstring[i], 0, len);
            break;
        case KEY_BACKSPACE:
            st_chatstring[i][MAX(len--, 0)] = 0;
            break;
        default:
            st_chatstring[i][len] = c;
            break;
        }
    }
}

//
// ST_Responder
// Respond to keyboard input events, intercept cheats.
//

dboolean ST_Responder(event_t* ev) {
    M_CheatProcess(plyr, ev);

    if(netgame) {
        ST_FeedChatMsg(ev);

        if(st_chatOn) {
            return true;
        }

        if(ev->type == ev_keydown && ev->data1 == 't') {
            st_chatOn = true;
        }
    }

    return false;
}

//
// ST_DisplayName
//

static void ST_DisplayName(int playernum) {
    fixed_t     x;
    fixed_t     y;
    fixed_t     z;
    fixed_t     xangle;
    fixed_t     yangle;
    fixed_t     screenx;
    fixed_t     xpitch;
    fixed_t     ypitch;
    fixed_t     screeny;
    player_t*   player;
    char        name[MAXPLAYERNAME];
    rcolor      color;
    fixed_t     distance;

    // don't display self
    if(playernum == consoleplayer) {
        return;
    }

    player = &players[playernum];

    // get distance
    distance = P_AproxDistance(viewx - player->mo->x, viewy - player->mo->y);
    if(distance > (1280*FRACUNIT)) {
        return;    // too far
    }

    x = player->mo->x - viewx;
    y = player->mo->y - viewy;
    z = player->mo->z - (players[consoleplayer].viewz - (96*FRACUNIT));

    // set relative viewpoint
    xangle = (FixedMul(dsin(viewangle), x) - FixedMul(dcos(viewangle), y));
    yangle = (FixedMul(dsin(viewangle), y) + FixedMul(dcos(viewangle), x));
    xpitch = (FixedMul(dsin(viewpitch), yangle) - FixedMul(dcos(viewpitch), z));
    ypitch = (FixedMul(dsin(viewpitch), z) + FixedMul(dcos(viewpitch), yangle));

    // check x offscreen
    if(xangle < -yangle) {
        return;
    }

    // check y offscreen
    if(yangle < xangle) {
        return;
    }

    // check if behind view
    if(yangle < 0x80001) {
        return;
    }

    if(ypitch > xpitch) {
        return;
    }

    // adjust if needed
    if(yangle < 0x80000) {
        xangle += FixedMul(FixedDiv(0x80000 - yangle,
                                    xangle - yangle), yangle - xangle);
        yangle = 0x80000;
    }

    // convert to screen space
    screenx = ((FixedDiv(xangle, yangle) * (SCREENWIDTH/2)) >> FRACBITS) + (SCREENWIDTH/2);
    screeny = (((FixedDiv(ypitch, xpitch) * -(SCREENWIDTH/2)) >> FRACBITS) + (SCREENWIDTH/2)) - 40;

    if(screenx < 0) {
        screenx = 0;
    }

    if(screenx > SCREENWIDTH) {
        screenx = SCREENWIDTH;
    }

    if(screeny < 0) {
        screeny = 0;
    }

    if(screeny > SCREENHEIGHT) {
        screeny = SCREENHEIGHT;
    }

    // change colors based on health/condition
    if(player->health < 40) {
        color = RED;
    }
    else if(player->health < 80) {
        color = YELLOW;
    }
    else {
        color = WHITE;
    }

    // fade alpha based on distance. farther will mean less alpha
    distance /= FRACUNIT;

    // reset alpha and set new value
    color ^= (((color >> 24) & 0xff) << 24);
    color |= ((255 - (int)((float)distance * 0.19921875f)) << 24);

    // display player name
    dsnprintf(name, MAXPLAYERNAME, "%s", player_names[playernum]);
    Draw_Text(screenx, screeny, color, 1.0f, 0, name);
}
