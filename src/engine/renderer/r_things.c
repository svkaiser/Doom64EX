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
// DESCRIPTION: Thing/Sprite rendering code
//
//-----------------------------------------------------------------------------

#include "r_lights.h"
#include "i_system.h"
#include "tables.h"
#include "w_wad.h"
#include "doomstat.h"
#include "z_zone.h"
#include "r_things.h"
#include "gl_texture.h"
#include "gl_main.h"
#include "r_drawlist.h"
#include "p_local.h"
#include "r_clipper.h"
#include "m_misc.h"
#include "con_console.h"

#include <stdlib.h>

#define MAX_SPRITES    1024

spritedef_t     *spriteinfo;
int             numsprites;

spriteframe_t   sprtemp[29];
int             maxframe;
char*           spritename;

static visspritelist_t visspritelist[MAX_SPRITES];
static visspritelist_t *vissprite = NULL;

CVAR_EXTERNAL(m_regionblood);
CVAR_EXTERNAL(st_flashoverlay);
CVAR_EXTERNAL(i_interpolateframes);
CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(r_rendersprites);

static void AddSpriteDrawlist(drawlist_t *dl, visspritelist_t *vis, int texid);

//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//

void R_InstallSpriteLump(int lump, unsigned frame, unsigned rotation, dboolean flipped) {
    int    r;

    if(frame >= 29 || rotation > 8) {
        I_Error("R_InstallSpriteLump: Bad frame characters in lump %i", lump);
    }

    if((int)frame > maxframe) {
        maxframe = frame;
    }

    if(rotation == 0) {
        // the lump should be used for all rotations
        if((sprtemp[frame].rotate == false)) {
            I_Error("R_InitSprites: Sprite %s frame %c has multiple rot=0 lump", spritename, 'A'+frame);
        }

        if(sprtemp[frame].rotate == true) {
            I_Error("R_InitSprites: Sprite %s frame %c has rotations and a rot=0 lump", spritename, 'A'+frame);
        }

        sprtemp[frame].rotate = false;
        for(r = 0; r < 8; r++) {
            sprtemp[frame].lump[r] = lump - s_start;
            sprtemp[frame].flip[r] = (byte)flipped;
        }
        return;
    }

    // the lump is only used for one rotation
    if(sprtemp[frame].rotate == false) {
        I_Error("R_InitSprites: Sprite %s frame %c has rotations and a rot=0 lump", spritename, 'A'+frame);
    }

    sprtemp[frame].rotate = true;

    // make 0 based
    rotation--;
    if((sprtemp[frame].lump[rotation] != -1))
        I_Error("R_InitSprites: Sprite %s : %c : %c has two lumps mapped to it",
                spritename, 'A'+frame, '1'+rotation);

    sprtemp[frame].lump[rotation] = lump - s_start;
    sprtemp[frame].flip[rotation] = (byte)flipped;
}


//
// R_InitSprites
// Pass a null terminated list of sprite names
//  (4 chars exactly) to be used.
// Builds the sprite rotation matrixes to account
//  for horizontally flipped sprites.
// Will report an error if the lumps are inconsistant.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//  a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//  letter/number appended.
// The rotation character can be 0 to signify no rotations.
//

void R_InitSprites(char** namelist) {
    char**  check;
    int     i;
    int     l;
    int     intname;
    int     frame;
    int     rotation;
    int     start;
    int     end;
    int     patched;

    // count the number of sprite names
    check = namelist;
    while(*check != NULL) {
        check++;
    }

    numsprites = check-namelist;

    if(!numsprites) {
        return;
    }

    spriteinfo = Z_Malloc(numsprites * sizeof(*spriteinfo), PU_STATIC, NULL);

    start = s_start - 1;
    end = s_end + 1;

    // scan all the lump names for each of the names,
    //  noting the highest frame letter.
    // Just compare 4 characters as ints

    for(i = 0; i < numsprites; i++) {
        spritename = namelist[i];
        dmemset(sprtemp,-1, sizeof(sprtemp));

        maxframe = -1;
        intname = *(int*)namelist[i];

        // scan the lumps,
        //  filling in the frames for whatever is found

        for(l = start + 1; l < end; l++) {
            if(*(int *)lumpinfo[l].name == intname) {
                frame = lumpinfo[l].name[4] - 'A';
                rotation = lumpinfo[l].name[5] - '0';

                patched = l;

                R_InstallSpriteLump(patched, frame, rotation, false);

                if(lumpinfo[l].name[6]) {
                    frame = lumpinfo[l].name[6] - 'A';
                    rotation = lumpinfo[l].name[7] - '0';
                    R_InstallSpriteLump(l, frame, rotation, true);
                }
            }
        }

        // check the frames that were found for completeness
        if(maxframe == -1) {
            spriteinfo[i].numframes = 0;
            continue;
        }

        maxframe++;

        for(frame = 0; frame < maxframe; frame++) {
            switch((int)sprtemp[frame].rotate) {
            case -1:
                // no rotations were found for that frame at all
                I_Error("R_InitSprites: No patches found for %s frame %c", namelist[i], frame+'A');
                break;

            case 0:
                // only the first rotation is needed
                break;

            case 1:
                // must have all 8 frames
                for(rotation = 0; rotation < 8; rotation++)
                    if(sprtemp[frame].lump[rotation] == -1)
                        I_Error("R_InitSprites: Sprite %s frame %c is missing rotations",
                                namelist[i], frame+'A');
                break;
            }
        }

        // allocate space for the frames present and copy sprtemp to it
        spriteinfo[i].numframes = maxframe;
        spriteinfo[i].spriteframes =
            Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
        dmemcpy(spriteinfo[i].spriteframes, sprtemp, maxframe*sizeof(spriteframe_t));
    }

    //
    // [kex] set regional blood if needed
    //

    if(W_CheckNumForName("BLUDA0") <= -1) {
        int lump = (int)m_regionblood.value;

        if(lump > 1) {
            lump = 1;
        }
        if(lump < 0) {
            lump = 0;
        }

        states[S_494].sprite = SPR_RBLD + lump;
        states[S_495].sprite = SPR_RBLD + lump;
        states[S_496].sprite = SPR_RBLD + lump;
        states[S_497].sprite = SPR_RBLD + lump;
    }
}

//
// R_AddSprites
//

void R_AddSprites(subsector_t *sub) {
    mobj_t* thing;

    // Handle all things in sector.
    for(thing = sub->sector->thinglist; thing; thing = thing->snext) {
        if(thing->subsector != sub) { // don't add sprite if it doesn't belong in this subsector
            continue;
        }

        if(thing->flags & MF_NOSECTOR) {
            continue;
        }

        if(vissprite - visspritelist >= MAX_SPRITES) {
            CON_Warnf("R_AddSprites: Sprite overflow");
            return;
        }

        vissprite->spr = thing;
        vissprite++;
    }
}

//
// R_ClearSprites
//

void R_ClearSprites(void) {
    vissprite = visspritelist;
}

//
// R_AddVisSprite
//

static void R_AddVisSprite(visspritelist_t* vissprite) {
    spritedef_t*    sprdef;
    spriteframe_t*  sprframe;
    angle_t         ang;
    int             spritenum;
    int             rot;
    mobj_t*         thing;

    thing = vissprite->spr;

    if(thing->sprite == SPR_SPOT &&
            !(thing->flags & MF_RENDERLASER)) {
        return;
    }

    if(thing->flags & MF_RENDERLASER) {
        spritenum = W_GetNumForName("BOLTA0") - s_start;
    }
    else {
        sprdef = &spriteinfo[thing->sprite];
        sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

        if(!sprframe) {
            return;
        }

        if(sprframe->rotate) {
            // choose a different rotation based on player view
            ang = R_PointToAngle(thing->x - viewx, thing->y - viewy);
            rot = (ang-thing->angle + (unsigned)(ANG45 / 2) * 9) >> 29;
        }
        else
            // use single rotation for all views
        {
            rot = 0;
        }

        spritenum = sprframe->lump[rot];
    }

    AddSpriteDrawlist(&drawlist[DLT_SPRITE], vissprite, spritenum);
}

//
// R_GenerateSpritePlane
//

static dboolean R_GenerateSpritePlane(visspritelist_t* vissprite, vtx_t* vertex) {
    float           x;
    float           y;
    float           z;
    spritedef_t*    sprdef;
    spriteframe_t*  sprframe;
    angle_t         ang;
    int             spritenum;
    int             rot;
    float           dx1;
    float           dx2;
    mobj_t*         thing;
    float           offs;
    float           dy1;
    float           dy2;
    float           dz1;
    float           dz2;
    float           height;
    float           z2;


    thing = vissprite->spr;
    x = vissprite->x;
    y = vissprite->y;

    sprdef = &spriteinfo[thing->sprite];
    sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

    if(sprframe->rotate) {
        // choose a different rotation based on player view
        ang = R_PointToAngle(thing->x - viewx, thing->y - viewy);
        rot = (ang-thing->angle + (unsigned)(ANG45 / 2) * 9) >> 29;
    }
    else
        // use single rotation for all views
    {
        rot = 0;
    }

    spritenum = sprframe->lump[rot];

    // flip sprite if needed
    if(sprframe->flip[rot]) {
        offs = 1.0f;
    }
    else {
        offs = 0.0f;
    }

    // [kex] nightmare things have a shade of dark green
    if(thing->flags & MF_NIGHTMARE) {
        dglSetVertexColor(vertex, D_RGBA(64, 255, 0, thing->alpha), 4);
    }
    else if((thing->frame & FF_FULLBRIGHT)) {
        dglSetVertexColor(vertex, D_RGBA(255, 255, 255, thing->alpha), 4);
    }
    else {
        R_LightToVertex(vertex,
                        thing->subsector->sector->colors[LIGHT_THING], 4);
    }

    vertex[0].a = vertex[1].a = vertex[2].a = vertex[3].a = thing->alpha;

    // setup texture mapping
    vertex[0].tu = vertex[1].tu = offs;
    vertex[2].tu = vertex[3].tu = (1.0f - offs);
    vertex[0].tv = vertex[2].tv = 1.0f;
    vertex[1].tv = vertex[3].tv = 0.0f;

    // set offset
    if(sprframe->flip[rot]) {
        dx1 = spriteoffset[spritenum] - (float)spritewidth[spritenum];
    }
    else {
        dx1 = -spriteoffset[spritenum];
    }

    dx2 = dx1 + (float)spritewidth[spritenum];

    z = vissprite->z + spritetopoffset[spritenum];

    height = (float)spriteheight[spritenum];
    z2 = z - height;

    // render as billboard?
    if(r_rendersprites.value >= 2) {
        float centerz;
        float centertop;
        float centerbottom;

        // rotate sprite's pitch from the center of the plane
        centerz = D_fabs(z - z2) * 0.5f;
        centertop = -centerz;
        centerbottom = height - centerz;

        dy1 = (centertop * viewsin[1]);
        dy2 = (centerbottom * viewsin[1]);
        dz1 = (centertop * viewcos[1]) + z - (height * 0.5f);
        dz2 = (centerbottom * viewcos[1]) + z - (height * 0.5f);
    }
    // normal rendering
    else {
        dy1 = dy2 = 0;
        dz1 = z2;
        dz2 = z;
    }

    // setup vertex coordinates
    vertex[0].x = x + (viewsin[0] * dx1 - dy1 * viewcos[0]);
    vertex[1].x = x + (viewsin[0] * dx1 - dy2 * viewcos[0]);

    vertex[0].y = y - (viewcos[0] * dx1 + dy1 * viewsin[0]);
    vertex[1].y = y - (viewcos[0] * dx1 + dy2 * viewsin[0]);

    vertex[2].x = x + (viewsin[0] * dx2 - dy1 * viewcos[0]);
    vertex[3].x = x + (viewsin[0] * dx2 - dy2 * viewcos[0]);

    vertex[2].y = y - (viewcos[0] * dx2 + dy1 * viewsin[0]);
    vertex[3].y = y - (viewcos[0] * dx2 + dy2 * viewsin[0]);

    vertex[0].z = vertex[2].z = dz1;
    vertex[1].z = vertex[3].z = dz2;

    if(R_FrustrumTestVertex(vertex, 4)) {
        return true;
    }

    return false;
}

//
// R_GenerateLaserPlane
//

static dboolean R_GenerateLaserPlane(visspritelist_t* vissprite, vtx_t* vertex) {
    float           x;
    float           y;
    float           z;
    float           dx1;
    float           dx2;
    mobj_t*         thing;
    laser_t*        laser;
    float           s;
    float           c;
    int             spritenum;

    thing = vissprite->spr;

    // must have data present
    if(!thing->extradata) {
        return false;
    }

    laser = (laser_t*)thing->extradata;

    spritenum = W_GetNumForName("BOLTA0") - s_start;

    dglSetVertexColor(vertex, D_RGBA(255, 0, 0, thing->alpha), 4);

    // setup texture mapping
    vertex[0].tu = vertex[1].tu = 0;
    vertex[2].tu = vertex[3].tu = 1;
    vertex[0].tv = vertex[1].tv = 1;
    vertex[2].tv = vertex[3].tv = 0;

    // get angles
    s = F2D3D(dsin(laser->angle + ANG90));
    c = F2D3D(dcos(laser->angle + ANG90));

    // setup vertex coordinates

    // start of laser
    x = F2D3D(laser->x1);
    y = F2D3D(laser->y1);
    z = F2D3D(laser->z1);

    dx1 = -spritetopoffset[spritenum];
    dx2 = dx1 + (float)spriteheight[spritenum];

    vertex[0].x = x + (c * dx1);
    vertex[0].y = y + (s * dx1);

    vertex[2].x = x + (c * dx2);
    vertex[2].y = y + (s * dx2);

    vertex[0].z = vertex[2].z = z;

    // end of laser
    x = F2D3D(laser->x2);
    y = F2D3D(laser->y2);
    z = F2D3D(laser->z2);

    vertex[1].x = x + (c * dx1);
    vertex[1].y = y + (s * dx1);

    vertex[3].x = x + (c * dx2);
    vertex[3].y = y + (s * dx2);

    vertex[1].z = vertex[3].z = z;

    if(R_FrustrumTestVertex(vertex, 4)) {
        return true;
    }

    return false;
}

//
// AddSpriteDrawlist
//

static void AddSpriteDrawlist(drawlist_t *dl, visspritelist_t *vis, int texid) {
    vtxlist_t *list;
    mobj_t* mobj;

    list = DL_AddVertexList(dl);
    list->data = (visspritelist_t*)vis;

    if(vis->spr->flags & MF_RENDERLASER) {
        list->callback = R_GenerateLaserPlane;
    }
    else {
        list->callback = R_GenerateSpritePlane;
    }

    mobj = vis->spr;

    if(mobj->subsector->sector->lightlevel) {
        // add sprite's gamma glow values as a flag

        list->flags |= DLF_GLOW;
        list->params = mobj->subsector->sector->lightlevel;
    }

    // hack to include info on palette indexes
    list->texid =
        (texid | ((mobj->player ? mobj->player->palette : mobj->info->palette) << 24)
         | (list->flags << 16));
}

//
// R_SetupSprites
//

void R_SetupSprites(void) {
    dboolean interpolate = (int)i_interpolateframes.value;
    visspritelist_t *vis;

    for(vis = vissprite - 1; vis >= visspritelist; vis--) {
        // Avoid from having the torch poles and fire from z-fighting
        if(vis->spr->type >= MT_PROP_POLEBASELONG &&
                vis->spr->type <= MT_PROP_FIREYELLOW) {
            angle_t ang = R_PointToAngle(vis->spr->x - viewx, vis->spr->y - viewy);

            // fire sprites are moved away from view while torches are moved towards view
            if(vis->spr->type >= MT_PROP_FIREBLUE && vis->spr->type <= MT_PROP_FIREYELLOW) {
                ang += ANG180;
            }

            // move a bit further towards view
            vis->x = F2D3D(vis->spr->x - FixedMul(FLOATTOFIXED(1.5), dcos(ang)));
            vis->y = F2D3D(vis->spr->y - FixedMul(FLOATTOFIXED(1.5), dsin(ang)));
            vis->z = F2D3D(R_Interpolate(vis->spr->z, vis->spr->frame_z, interpolate));
        }
        else {  // normal vis sprite process
            vis->x = F2D3D(R_Interpolate(vis->spr->x, vis->spr->frame_x, interpolate));
            vis->y = F2D3D(R_Interpolate(vis->spr->y, vis->spr->frame_y, interpolate));
            vis->z = F2D3D(R_Interpolate(vis->spr->z, vis->spr->frame_z, interpolate));
        }

        vis->dist = (int)((vis->x - fviewx) * viewcos[0] +
                          (vis->y - fviewy) * viewsin[0]) / 2;

        // cameras and player's self are an exception
        // unless viewing self from camera
        if((vis->spr->type == MT_PLAYER) &&
                ((vis->spr->player == renderplayer) &&
                 (renderplayer->cameratarget == renderplayer->mo))) {
            continue;
        }

        R_AddVisSprite(vis);
    }
}

//
// R_DrawPSprite
//

void R_DrawPSprite(pspdef_t *psp, sector_t* sector, player_t *player) {
    spritedef_t     *sprdef;
    spriteframe_t   *sprframe;
    int             spritenum;
    int             flip;
    rcolor          color;
    byte            alpha;
    float           x;
    float           y;
    int             width;
    int             height;
    float           u1;
    float           u2;
    float           v1;
    float           v2;
    vtx_t           v[4];

    alpha = (player->mo->alpha * psp->alpha) / 0xff;

    // get sprite frame/defs
    sprdef = &spriteinfo[psp->state->sprite];
    sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK ];

    if(psp->state->frame & FF_FULLBRIGHT || nolights) {
        color = D_RGBA(255, 255, 255, alpha);
    }
    else {
        color = R_GetSectorLight(alpha, sector->colors[LIGHT_THING]);
    }

    spritenum = sprframe->lump[0];
    flip = sprframe->flip[0];

    // setup render states
    GL_BindSpriteTexture(spritenum, 0);
    GL_SetState(GLSTATE_BLEND, 1);

    // setup vertex data

    x = (F2D3D(R_Interpolate(psp->sx, psp->frame_x, (int)i_interpolateframes.value))
         - spriteoffset[spritenum]);

    y = (F2D3D(R_Interpolate(psp->sy, psp->frame_y, (int)i_interpolateframes.value))
         - spritetopoffset[spritenum]);

    if(player->onground) {
        x += (quakeviewx >> 24);
        y += (quakeviewy >> 16);
    }

    width = spritewidth[spritenum];
    height = spriteheight[spritenum];
    u1 = (rfloat)flip;
    u2 = (rfloat)1-flip;
    v1 = (rfloat)flip;
    v2 = (rfloat)1-flip;

    GL_SetOrtho(0);
    GL_Set2DQuad(v, x, y, width, height, u1, u2, v1, v2, color);
    GL_SetTextureUnit(0, true);
    GL_CheckFillMode();

    //
    // setup texture environment for effects
    //
    if(r_texturecombiner.value) {
        float f[4];

        f[0] = f[1] = f[2] = ((float)sector->lightlevel / 255.0f);

        dglTexCombColorf(GL_TEXTURE0_ARB, f, GL_ADD);

        if(!nolights) {
            GL_UpdateEnvTexture(WHITE);
            GL_SetTextureUnit(1, true);
            dglTexCombModulate(GL_PREVIOUS, GL_PRIMARY_COLOR);
        }

        if(st_flashoverlay.value <= 0) {
            GL_SetTextureUnit(2, true);
            dglTexCombColor(GL_PREVIOUS, flashcolor, GL_ADD);
        }

        dglTexCombReplaceAlpha(GL_TEXTURE0_ARB);

        GL_SetTextureUnit(0, true);
    }
    else {
        int l = (sector->lightlevel >> 1);

        GL_SetTextureUnit(1, true);
        GL_SetTextureMode(GL_ADD);
        GL_UpdateEnvTexture(D_RGBA(l, l, l, 0xff));
        GL_SetTextureUnit(0, true);

        if(nolights) {
            GL_SetTextureMode(GL_REPLACE);
        }
    }

    // render
    dglSetVertex(v);
    dglTriangle(0, 1, 2);
    dglTriangle(3, 2, 1);
    dglDrawGeometry(4, v);

    GL_ResetViewport();

    if(devparm) {
        vertCount += 4;
    }

    GL_SetDefaultCombiner();
    GL_SetState(GLSTATE_BLEND, 0);
}

//
// R_RenderPlayerSprites
//

void R_RenderPlayerSprites(player_t *player) {
    pspdef_t*    psp;

    psp = &player->psprites[ps_weapon];
    for(psp = player->psprites; psp < &player->psprites[NUMPSPRITES]; psp++) {
        if(psp->state) {
            R_DrawPSprite(psp, player->mo->subsector->sector, player);
        }
    }
}

//
// R_DrawThingBBox
//

void R_DrawThingBBox(void) {
    float   bbox[4];
    float   z1;
    float   z2;
    int     i;
    mobj_t* thing;

#define DRAWBBOXPOLY(b1, b2, z) \
    dglVertex3f(bbox[b1], bbox[b2], z)

#define DRAWBBOXSIDE1(z) \
    dglBegin(GL_POLYGON); \
    DRAWBBOXPOLY(BOXLEFT, BOXBOTTOM, z); \
    DRAWBBOXPOLY(BOXLEFT, BOXTOP, z); \
    DRAWBBOXPOLY(BOXRIGHT, BOXTOP, z); \
    DRAWBBOXPOLY(BOXRIGHT, BOXBOTTOM, z); \
    dglEnd()

#define DRAWBBOXSIDE2(b3) \
    dglBegin(GL_POLYGON); \
    DRAWBBOXPOLY(BOXLEFT, b3, z1); \
    DRAWBBOXPOLY(BOXRIGHT, b3, z1); \
    DRAWBBOXPOLY(BOXRIGHT, b3, z2); \
    DRAWBBOXPOLY(BOXLEFT, b3, z2); \
    dglEnd()

#define DRAWBBOXSIDE3(b1) \
    dglBegin(GL_POLYGON); \
    DRAWBBOXPOLY(b1, BOXBOTTOM, z1); \
    DRAWBBOXPOLY(b1, BOXBOTTOM, z2); \
    DRAWBBOXPOLY(b1, BOXTOP, z2); \
    DRAWBBOXPOLY(b1, BOXTOP, z1); \
    dglEnd()

    GL_SetState(GLSTATE_TEXTURE0, 0);
    GL_SetState(GLSTATE_CULL, 0);
    dglDepthRange(0.0f, 0.0f);

    for(i = 0; i < (vissprite - visspritelist); i++) {
        thing = visspritelist[i].spr;

        if(thing->player && thing->player->cameratarget == thing->player->mo) {
            continue;
        }

        bbox[BOXTOP]        = F2D3D(thing->y + thing->radius);
        bbox[BOXBOTTOM]     = F2D3D(thing->y - thing->radius);
        bbox[BOXRIGHT]      = F2D3D(thing->x + thing->radius);
        bbox[BOXLEFT]       = F2D3D(thing->x - thing->radius);
        z1                  = F2D3D(thing->z);
        z2                  = F2D3D(thing->z + thing->height);

        GL_SetState(GLSTATE_BLEND, 1);
        dglColor4ub(255, 255, 255, 64);
        dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        DRAWBBOXSIDE1(z1);
        DRAWBBOXSIDE1(z2);
        DRAWBBOXSIDE2(BOXBOTTOM);
        DRAWBBOXSIDE2(BOXTOP);
        DRAWBBOXSIDE3(BOXRIGHT);
        DRAWBBOXSIDE3(BOXLEFT);

        GL_SetState(GLSTATE_BLEND, 0);
        dglColor4ub(255, 255, 255, 255);
        dglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        DRAWBBOXSIDE1(z1);
        DRAWBBOXSIDE1(z2);
        DRAWBBOXSIDE2(BOXBOTTOM);
        DRAWBBOXSIDE2(BOXTOP);
        DRAWBBOXSIDE3(BOXRIGHT);
        DRAWBBOXSIDE3(BOXLEFT);
    }

    dglDepthRange(0.0f, 1.0f);
    dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    GL_SetState(GLSTATE_TEXTURE0, 1);
    GL_SetState(GLSTATE_CULL, 1);
    GL_SetState(GLSTATE_BLEND, 0);
}

