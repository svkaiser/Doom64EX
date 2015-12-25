// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "doomstat.h"
#include "gl_main.h"
#include "gl_texture.h"
#include "r_local.h"
#include "r_sky.h"
#include "r_drawlist.h"

CVAR_EXTERNAL(i_interpolateframes);
CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(r_fog);
CVAR_EXTERNAL(r_rendersprites);
CVAR_EXTERNAL(st_flashoverlay);

//
// ProcessWalls
//

static dboolean ProcessWalls(vtxlist_t* vl, int* drawcount) {
    seg_t* seg = (seg_t*)vl->data;
    sector_t* sec = seg->frontsector;

    bspColor[LIGHT_FLOOR]    = R_GetSectorLight(0xff, sec->colors[LIGHT_FLOOR]);
    bspColor[LIGHT_CEILING] = R_GetSectorLight(0xff, sec->colors[LIGHT_CEILING]);
    bspColor[LIGHT_THING]    = R_GetSectorLight(0xff, sec->colors[LIGHT_THING]);
    bspColor[LIGHT_UPRWALL] = R_GetSectorLight(0xff, sec->colors[LIGHT_UPRWALL]);
    bspColor[LIGHT_LWRWALL] = R_GetSectorLight(0xff, sec->colors[LIGHT_LWRWALL]);

    if(!vl->callback(seg, &drawVertex[*drawcount])) {
        return false;
    }

    dglTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
    dglTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

    *drawcount += 4;

    return true;
}

//
// ProcessFlats
//

static dboolean ProcessFlats(vtxlist_t* vl, int* drawcount) {
    int j;
    fixed_t tx;
    fixed_t ty;
    leaf_t* leaf;
    subsector_t* ss;
    sector_t* sector;
    int count;

    ss      = (subsector_t*)vl->data;
    leaf    = &leafs[ss->leaf];
    sector  = ss->sector;
    count   = *drawcount;

    for(j = 0; j < ss->numleafs - 2; j++) {
        dglTriangle(count, count + 1 + j, count + 2 + j);
    }

    // need to keep texture coords small to avoid
    // floor 'wobble' due to rounding errors on some cards
    // make relative to first vertex, not (0,0)
    // which is arbitary anyway

    tx = (leaf->vertex->x >> 6) & ~(FRACUNIT - 1);
    ty = (leaf->vertex->y >> 6) & ~(FRACUNIT - 1);

    for(j = 0; j < ss->numleafs; j++) {
        int idx;
        vtx_t *v = &drawVertex[count];

        if(vl->flags & DLF_CEILING) {
            leaf = &leafs[(ss->leaf + (ss->numleafs - 1)) - j];
        }
        else {
            leaf = &leafs[ss->leaf + j];
        }

        v->x = F2D3D(leaf->vertex->x);
        v->y = F2D3D(leaf->vertex->y);

        if(vl->flags & DLF_CEILING) {
            if(i_interpolateframes.value) {
                v->z = F2D3D(sector->frame_z2[1]);
            }
            else {
                v->z = F2D3D(sector->ceilingheight);
            }
        }
        else {
            if(i_interpolateframes.value) {
                v->z = F2D3D(sector->frame_z1[1]);
            }
            else {
                v->z = F2D3D(sector->floorheight);
            }
        }

        v->tu = F2D3D((leaf->vertex->x >> 6) - tx);
        v->tv = -F2D3D((leaf->vertex->y >> 6) - ty);

        // set the mapping offsets for scrolling floors/ceilings
        if((!(vl->flags & DLF_CEILING) && sector->flags & MS_SCROLLFLOOR) ||
                (vl->flags & DLF_CEILING && sector->flags & MS_SCROLLCEILING)) {
            v->tu   += F2D3D(sector->xoffset >> 6);
            v->tv   += F2D3D(sector->yoffset >> 6);
        }

        v->a = 0xff;

        if(vl->flags & DLF_CEILING) {
            idx = sector->colors[LIGHT_CEILING];
        }
        else {
            idx = sector->colors[LIGHT_FLOOR];
        }

        R_LightToVertex(v, idx, 1);

        //
        // water layer 1
        //
        if(vl->flags & DLF_WATER1) {
            v->tv -= F2D3D(scrollfrac >> 6);
            v->a = 0xA0;
        }

        //
        // water layer 2
        //
        if(vl->flags & DLF_WATER2) {
            v->tu += F2D3D(scrollfrac >> 6);
        }

        count++;
    }

    *drawcount = count;

    return true;
}

//
// ProcessSprites
//

static dboolean ProcessSprites(vtxlist_t* vl, int* drawcount) {
    visspritelist_t* vis;
    mobj_t* mobj;

    vis = (visspritelist_t*)vl->data;
    mobj = vis->spr;

    if(!mobj) {
        return false;
    }

    if(!vl->callback(vis, &drawVertex[*drawcount])) {
        return false;
    }

    GL_SetState(GLSTATE_CULL, !(mobj->flags & MF_RENDERLASER));

    dglTriangle(*drawcount + 0, *drawcount + 1, *drawcount + 2);
    dglTriangle(*drawcount + 3, *drawcount + 2, *drawcount + 1);

    *drawcount += 4;

    return true;
}

//
// SetupFog
//
// Sky flats determine how fog is rendered. this includes
// fog color, distance and density. The factor for fog
// density is based on values from the original N64 version.
//

static void SetupFog(void) {
    dglFogi(GL_FOG_MODE, GL_LINEAR);

    // don't render fog in wireframe mode
    if(r_fillmode.value <= 0) {
        return;
    }

    if(!skyflatnum) {
        dglDisable(GL_FOG);
    }
    else if(r_fog.value) {
        rfloat color[4] = { 0, 0, 0, 0 };
        rcolor fogcolor = 0;
        int fognear = 0;
        int fogfactor;

        // density factors range from 990 to 900
        // each step in density is suppose to be (128 * 1000)
        // 985 is the default if no sky is present at all

        fognear = sky ? sky->fognear : 985;
        fogfactor = (1000 - fognear);

        if(fogfactor <= 0) {
            fogfactor = 1;
        }

        dglEnable(GL_FOG);

        // do exponential fog if color is black
        if(sky && (sky->fogcolor & 0xFFFFFF) != 0) {
            int min;
            int max;

            max = 128000 / fogfactor;
            min = ((fognear - 500) * 256) / fogfactor;

            fogcolor = sky->fogcolor;
            dglFogi(GL_FOG_MODE, GL_EXP);
            dglFogf(GL_FOG_DENSITY, 14.0f / (max + min));
        }
        // do linear rendering for colored fog
        else {
            float min;
            float max;
            float position;

            position = ((float)fogfactor / 1000.0f);

            if(position <= 0.0f) {
                position = 0.00001f;
            }

            min = 5.0f / position;
            max = 30.0f / position;

            dglFogf(GL_FOG_START, min);
            dglFogf(GL_FOG_END, max);
        }

        dglGetColorf(fogcolor, color);
        dglFogfv(GL_FOG_COLOR, color);
    }
}

//
// R_SetViewMatrix
//

void R_SetViewMatrix(void) {
    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();
    dglViewFrustum(video_width, video_height, r_fov.value, 0.1f);
    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();
    dglRotatef(-TRUEANGLES(viewpitch), 1.0f, 0.0f, 0.0f);
    dglRotatef(-TRUEANGLES(viewangle) + 90.0f, 0.0f, 0.0f, 1.0f);
    dglTranslatef(-fviewx, -fviewy, -fviewz);
}

//
// R_RenderWorld
//

void R_RenderWorld(void) {
    SetupFog();

    dglEnable(GL_DEPTH_TEST);

    DL_BeginDrawList(r_fillmode.value >= 1, r_texturecombiner.value >= 1);

    // setup texture environment for effects
    if(r_texturecombiner.value) {
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
        GL_SetTextureUnit(1, true);
        GL_SetTextureMode(GL_ADD);
        GL_SetTextureUnit(0, true);

        if(nolights) {
            GL_SetTextureMode(GL_REPLACE);
        }
    }

    dglEnable(GL_ALPHA_TEST);

    // begin draw list loop

    // -------------- Draw walls (segs) --------------------------

    DL_ProcessDrawList(DLT_WALL, ProcessWalls);

    // -------------- Draw floors/ceilings (leafs) ---------------

    GL_SetState(GLSTATE_BLEND, 1);
    DL_ProcessDrawList(DLT_FLAT, ProcessFlats);

    // -------------- Draw things (sprites) ----------------------

    if(devparm) {
        spriteRenderTic = I_GetTimeMS();
    }

    if(r_rendersprites.value) {
        R_SetupSprites();
    }

    dglDepthMask(GL_FALSE);
    DL_ProcessDrawList(DLT_SPRITE, ProcessSprites);

    // -------------- Restore states -----------------------------

    dglDisable(GL_ALPHA_TEST);
    dglDepthMask(GL_TRUE);
    dglDisable(GL_FOG);
    dglDisable(GL_DEPTH_TEST);

    GL_SetOrthoScale(1.0f);
    GL_SetState(GLSTATE_BLEND, 0);
    GL_SetState(GLSTATE_CULL, 1);
    GL_SetDefaultCombiner();

    // villsa 12152013 - make sure we're using the default blend function
    dglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

