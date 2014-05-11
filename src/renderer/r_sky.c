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
// DESCRIPTION: Sky rendering code
//
//-----------------------------------------------------------------------------

#include <stdlib.h>

#include "doomstat.h"
#include "r_lights.h"
#include "r_sky.h"
#include "w_wad.h"
#include "m_random.h"
#include "sounds.h"
#include "s_sound.h"
#include "p_local.h"
#include "z_zone.h"
#define close _close
#include "i_png.h"
#undef _close
#include "gl_texture.h"
#include "gl_draw.h"
#include "r_drawlist.h"

skydef_t*   sky;
int         skypicnum = -1;
int         skybackdropnum = -1;
int         skyflatnum = -1;
int         thunderCounter = 0;
int         lightningCounter = 0;
int         thundertic = 1;
dboolean    skyfadeback = false;
byte*       fireBuffer;
dPalette_t  firePal16[256];
int         fireLump = -1;

static word CloudOffsetY = 0;
static word CloudOffsetX = 0;
static float sky_cloudpan1 = 0;
static float sky_cloudpan2 = 0;

#define FIRESKY_WIDTH   64
#define FIRESKY_HEIGHT  64

CVAR_EXTERNAL(r_texturecombiner);
CVAR_EXTERNAL(r_skybox);

#define SKYVIEWPOS(angle, amount, x) x = -(angle / (float)ANG90 * amount); while(x < 1.0f) x += 1.0f

//
// R_CloudThunder
// Loosely based on subroutine at 0x80026418
//

static void R_CloudThunder(void) {
    if(!(gametic & ((thunderCounter & 1) ? 1 : 3))) {
        return;
    }

    if((thunderCounter - thundertic) > 0) {
        thunderCounter = (thunderCounter - thundertic);
        return;
    }

    if(lightningCounter == 0) {
        S_StartSound(NULL, sfx_thndrlow + (M_Random() & 1));
        thundertic = (1 + (M_Random() & 1));
    }

    if(!(lightningCounter < 6)) {  // Reset loop after 6 lightning flickers
        int rand = (M_Random() & 7);
        thunderCounter = (((rand << 4) - rand) << 2) + 60;
        lightningCounter = 0;
        return;
    }

    if((lightningCounter & 1) == 0) {
        sky->skycolor[0] += 0x001111;
        sky->skycolor[1] += 0x001111;
    }
    else {
        sky->skycolor[0] -= 0x001111;
        sky->skycolor[1] -= 0x001111;
    }

    thunderCounter = (M_Random() & 7) + 1;    // Do short delay loops for lightning flickers
    lightningCounter++;
}

//
// R_CloudTicker
//

static void R_CloudTicker(void) {
    CloudOffsetX -= (dcos(viewangle) >> 10);
    CloudOffsetY += (dsin(viewangle) >> 9);

    if(r_skybox.value) {
        sky_cloudpan1 += 0.00225f;
        sky_cloudpan2 += 0.00075f;
    }

    if(sky->flags & SKF_THUNDER) {
        R_CloudThunder();
    }
}

//
// R_TitleSkyTicker
//

static void R_TitleSkyTicker(void) {
    if(skyfadeback == true) {
        logoAlpha += 8;
        if(logoAlpha > 0xff) {
            logoAlpha = 0xff;
            skyfadeback = false;
        }
    }
}

//
// R_DrawSkyDome
//

static void R_DrawSkyDome(int tiles, float rows, int height,
                          int radius, float offset, float topoffs,
                          rcolor c1, rcolor c2) {
    fixed_t x, y, z;
    fixed_t lx, ly;
    int i;
    angle_t an;
    float tu1, tu2;
    int r;
    vtx_t *vtx;
    int count;

    lx = ly = count = 0;

#define NUM_SKY_DOME_FACES  32

    //
    // hack to force ortho scale back to 1
    //
    GL_SetOrthoScale(1.0f);

    //
    // setup view projection
    //
    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();
    dglViewFrustum(video_width, video_height, r_fov.value, 0.1f);
    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();
    dglPushMatrix();
    dglRotatef(-TRUEANGLES(viewpitch), 1.0f, 0.0f, 0.0f);
    dglRotatef(-TRUEANGLES(viewangle) + 90.0f, 0.0f, 0.0f, 1.0f);

    //
    // try to center view to the dome
    //
    dglTranslated(
        -((float)radius / ((float)NUM_SKY_DOME_FACES / 2.0f)),
        -((float)radius / (M_PI / 2)),
        -offset);

    //
    // front faces are drawn here, so cull the back faces
    //
    dglCullFace(GL_BACK);
    GL_SetState(GLSTATE_BLEND, 1);

    r = radius / (NUM_SKY_DOME_FACES / 4);

    //
    // set pointer for the main vertex list
    //
    dglSetVertex(drawVertex);
    vtx = drawVertex;

#define SKYDOME_VERTEX() vtx->x = F2D3D(x); vtx->y = F2D3D(y); vtx->z = F2D3D(z)
#define SKYDOME_UV(u, v) vtx->tu = u; vtx->tv = v
#define SKYDOME_LEFT(v, h)                      \
    x = lx;                                     \
    y = ly;                                     \
    z = INT2F(h);                               \
    SKYDOME_UV(-tu1, v);                        \
    SKYDOME_VERTEX();                           \
    vtx++

#define SKYDOME_RIGHT(v, h)                     \
    x = lx + FixedMul(INT2F(r), dcos((angle))); \
    y = ly + FixedMul(INT2F(r), dsin((angle))); \
    z = INT2F(h);                               \
    SKYDOME_UV(-(tu2 * (i + 1)), v);            \
    SKYDOME_VERTEX();                           \
    vtx++

    tu1 = 0;
    tu2 = (float)tiles / (float)NUM_SKY_DOME_FACES;
    an = (ANGLE_MAX / NUM_SKY_DOME_FACES);

    //
    // setup vertex data
    //
    for(i = 0; i < NUM_SKY_DOME_FACES; i++) {
        angle_t angle = an * i;

        dglSetVertexColor(&vtx[0], c2, 1);
        dglSetVertexColor(&vtx[1], c1, 1);
        dglSetVertexColor(&vtx[2], c1, 1);
        dglSetVertexColor(&vtx[3], c2, 1);

        SKYDOME_LEFT(rows, -height);
        SKYDOME_LEFT(topoffs, height);
        SKYDOME_RIGHT(topoffs, height);
        SKYDOME_RIGHT(rows, -height);

        lx = x;
        ly = y;

        dglTriangle(0+count, 1+count, 2+count);
        dglTriangle(3+count, 0+count, 2+count);
        count += 4;

        tu1 += tu2;
    }

    //
    // draw sky dome
    //
    dglDrawGeometry(count, drawVertex);

    dglPopMatrix();
    dglCullFace(GL_FRONT);

    GL_SetState(GLSTATE_BLEND, 0);

#undef SKYDOME_RIGHT
#undef SKYDOME_LEFT
#undef SKYDOME_UV
#undef SKYDOME_VERTEX
#undef NUM_SKY_DOME_FACES
}

//
// R_DrawSkyboxCloud
//

static void R_DrawSkyboxCloud(void) {
    rcolor color;
    vtx_t v[4];

#define SKYBOX_SETALPHA(c, x)           \
    c ^= (((c >> 24) & 0xff) << 24);    \
    c |= (x << 24)

    //
    // hack to force ortho scale back to 1
    //
    GL_SetOrthoScale(1.0f);

    //
    // setup view projection
    //
    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();
    dglViewFrustum(video_width, video_height, r_fov.value, 0.1f);
    dglMatrixMode(GL_MODELVIEW);
    dglLoadIdentity();
    dglPushMatrix();
    dglRotatef(-TRUEANGLES(viewpitch), 1.0f, 0.0f, 0.0f);

    //
    // set vertex pointer
    //
    dglSetVertex(v);

    //
    // disable textures for horizon effect
    //
    dglDisable(GL_TEXTURE_2D);

    //
    // draw horizon ceiling
    //
    v[0].x = -MAX_COORD;
    v[0].y = -MAX_COORD;
    v[0].z = 512;
    v[1].x = MAX_COORD;
    v[1].y = -MAX_COORD;
    v[1].z = 512;
    v[2].x = MAX_COORD;
    v[2].y = MAX_COORD;
    v[2].z = 512;
    v[3].x = -MAX_COORD;
    v[3].y = MAX_COORD;
    v[3].z = 512;

    dglSetVertexColor(&v[0], sky->skycolor[0], 4);

    dglTriangle(0, 1, 3);
    dglTriangle(2, 3, 1);
    dglDrawGeometry(4, v);

    //
    // draw horizon wall
    //
    v[0].x = -MAX_COORD;
    v[0].y = 512;
    v[0].z = 12;
    v[1].x = -MAX_COORD;
    v[1].y = 512;
    v[1].z = 512;
    v[2].x = MAX_COORD;
    v[2].y = 512;
    v[2].z = 512;
    v[3].x = MAX_COORD;
    v[3].y = 512;
    v[3].z = 12;

    dglSetVertexColor(&v[0], sky->skycolor[1], 1);
    dglSetVertexColor(&v[1], sky->skycolor[0], 1);
    dglSetVertexColor(&v[2], sky->skycolor[0], 1);
    dglSetVertexColor(&v[3], sky->skycolor[1], 1);

    dglTriangle(0, 1, 2);
    dglTriangle(3, 0, 2);
    dglDrawGeometry(4, v);
    dglEnable(GL_TEXTURE_2D);
    dglPopMatrix();

    //
    // setup model matrix for clouds
    //
    dglPushMatrix();
    dglRotatef(-TRUEANGLES(viewpitch), 1.0f, 0.0f, 0.0f);
    dglRotatef(-TRUEANGLES(viewangle) + 90.0f, 0.0f, 0.0f, 1.0f);

    //
    // bind cloud texture and set blending
    //
    GL_SetTextureUnit(0, true);
    GL_BindGfxTexture(lumpinfo[skypicnum].name, false);
    GL_SetState(GLSTATE_BLEND, 1);

    //
    // draw first cloud layer
    //
    v[0].tu = sky_cloudpan1;
    v[0].tv = sky_cloudpan1;
    v[1].tu = 16 + sky_cloudpan1;
    v[1].tv = sky_cloudpan1;
    v[2].tu = 16 + sky_cloudpan1;
    v[2].tv = 16 + sky_cloudpan1;
    v[3].tu = sky_cloudpan1;
    v[3].tv = 16 + sky_cloudpan1;
    v[0].x = -MAX_COORD;
    v[0].y = -MAX_COORD;
    v[0].z = 768;
    v[1].x = MAX_COORD;
    v[1].y = -MAX_COORD;
    v[1].z = 768;
    v[2].x = MAX_COORD;
    v[2].y = MAX_COORD;
    v[2].z = 768;
    v[3].x = -MAX_COORD;
    v[3].y = MAX_COORD;
    v[3].z = 768;

    color = sky->skycolor[2];
    SKYBOX_SETALPHA(color, 0x3f);
    dglSetVertexColor(&v[0], color, 4);

    dglTriangle(0, 1, 3);
    dglTriangle(2, 3, 1);
    dglDrawGeometry(4, v);

    //
    // draw second cloud layer
    //
    // preserve color and xy and just update
    // uv coords and z height
    //
    v[0].tu = sky_cloudpan2;
    v[0].tv = sky_cloudpan2;
    v[1].tu = 32 + sky_cloudpan2;
    v[1].tv = sky_cloudpan2;
    v[2].tu = 32 + sky_cloudpan2;
    v[2].tv = 32 + sky_cloudpan2;
    v[3].tu = sky_cloudpan2;
    v[3].tv = 32 + sky_cloudpan2;
    v[0].z = v[1].z = v[2].z = v[3].z = 1024;

    dglTriangle(0, 1, 3);
    dglTriangle(2, 3, 1);
    dglDrawGeometry(4, v);

    //
    // add more contrast to the top cloud layer
    // just draw a non-textured plane and blend it
    //
    dglDisable(GL_TEXTURE_2D);
    SKYBOX_SETALPHA(color, 0x1f);
    dglSetVertexColor(&v[0], color, 4);
    dglTriangle(0, 1, 3);
    dglTriangle(2, 3, 1);
    dglDrawGeometry(4, v);
    dglEnable(GL_TEXTURE_2D);

    dglPopMatrix();
    GL_SetState(GLSTATE_BLEND, 0);

#undef SKYBOX_SETALPHA
}

//
// R_DrawSimpleSky
//

static void R_DrawSimpleSky(int lump, int offset) {
    float pos1;
    float width;
    int height;
    int lumpheight;
    int gfxLmp;
    float row;

    gfxLmp = GL_BindGfxTexture(lumpinfo[lump].name, true);
    height = gfxheight[gfxLmp];
    lumpheight = gfxorigheight[gfxLmp];

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    SKYVIEWPOS(viewangle, 1, pos1);

    width = (float)SCREENWIDTH / (float)gfxwidth[gfxLmp];
    row = (float)lumpheight / (float)height;

    GL_SetState(GLSTATE_BLEND, 1);
    GL_SetupAndDraw2DQuad(0, (float)offset - lumpheight, SCREENWIDTH, lumpheight,
                          pos1, width + pos1, 0.006f, row, WHITE, 1);
    GL_SetState(GLSTATE_BLEND, 0);
}

//
// R_DrawVoidSky
//

static void R_DrawVoidSky(void) {
    GL_SetOrtho(1);

    dglDisable(GL_TEXTURE_2D);
    dglColor4ubv((byte*)&sky->skycolor[2]);
    dglRecti(SCREENWIDTH, SCREENHEIGHT, 0, 0);
    dglEnable(GL_TEXTURE_2D);

    GL_ResetViewport();
}

//
// R_DrawTitleSky
//

static void R_DrawTitleSky(void) {
    R_DrawSimpleSky(skypicnum, 240);
    Draw_GfxImage(63, 25, sky->backdrop, D_RGBA(255, 255, 255, logoAlpha), false);
}

//
// R_DrawClouds
//

static void R_DrawClouds(void) {
    rfloat pos = 0.0f;
    vtx_t v[4];

    GL_SetTextureUnit(0, true);
    GL_BindGfxTexture(lumpinfo[skypicnum].name, false);

    pos = (TRUEANGLES(viewangle) / 360.0f) * 2.0f;

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    dglSetVertex(v);

    if(r_texturecombiner.value > 0 && gl_max_texture_units > 2) {
        dglSetVertexColor(&v[0], sky->skycolor[0], 2);
        dglSetVertexColor(&v[2], sky->skycolor[1], 2);

        GL_UpdateEnvTexture(WHITE);

        // pass 1: texture * skycolor
        dglTexCombColor(GL_TEXTURE, sky->skycolor[2], GL_MODULATE);

        // pass 2: result * const (though the original game uses the texture's alpha)
        GL_SetTextureUnit(1, true);
        dglTexCombColor(GL_PREVIOUS, 0xFF909090, GL_MODULATE);

        // pass 3: result + fragment color
        GL_SetTextureUnit(2, true);
        dglTexCombAdd(GL_PREVIOUS, GL_PRIMARY_COLOR);
    }
    else {
        GL_Set2DQuad(v, 0, 0, SCREENWIDTH, 120, 0, 0, 0, 0, 0);
        dglSetVertexColor(&v[0], sky->skycolor[0], 2);
        dglSetVertexColor(&v[2], sky->skycolor[1], 2);

        dglDisable(GL_TEXTURE_2D);

        GL_Draw2DQuad(v, true);

        dglEnable(GL_TEXTURE_2D);

        GL_SetTextureUnit(1, true);
        GL_SetTextureMode(GL_ADD);
        GL_UpdateEnvTexture(sky->skycolor[1]);
        GL_SetTextureUnit(0, true);

        dglSetVertexColor(&v[0], sky->skycolor[2], 4);
        v[0].a = v[1].a = v[2].a = v[3].a = 0x60;
    }

    v[3].x = v[1].x = 1.1025f;
    v[0].x = v[2].x = -1.1025f;
    v[2].y = v[3].y = 0;
    v[0].y = v[1].y = 0.4315f;
    v[0].z = v[1].z = 0;
    v[2].z = v[3].z = -1.0f;
    v[0].tu = v[2].tu = F2D3D(CloudOffsetX) - pos;
    v[1].tu = v[3].tu = (F2D3D(CloudOffsetX) + 1.5f) - pos;
    v[0].tv = v[1].tv = F2D3D(CloudOffsetY);
    v[2].tv = v[3].tv = F2D3D(CloudOffsetY) + 2.0f;

    GL_SetOrthoScale(1.0f); // force ortho mode to be set

    dglMatrixMode(GL_PROJECTION);
    dglLoadIdentity();
    dglViewFrustum(SCREENWIDTH, SCREENHEIGHT, 45.0f, 0.1f);
    dglMatrixMode(GL_MODELVIEW);
    dglEnable(GL_BLEND);
    dglPushMatrix();
    dglTranslated(0.0f, 0.0f, -1.0f);
    dglTriangle(0, 1, 2);
    dglTriangle(3, 2, 1);
    dglDrawGeometry(4, v);
    dglPopMatrix();
    dglDisable(GL_BLEND);

    GL_SetDefaultCombiner();
}

//
// R_SpreadFire
//

static void R_SpreadFire(byte* src1, byte* src2, int pixel, int counter, int* rand) {
    int randIdx = 0;
    byte *tmpSrc;

    if(pixel != 0) {
        randIdx = rndtable[*rand];
        *rand = ((*rand+2) & 0xff);

        tmpSrc = (src1 + (((counter - (randIdx & 3)) + 1) & (FIRESKY_WIDTH-1)));
        *(byte*)(tmpSrc - FIRESKY_WIDTH) = pixel - ((randIdx & 1));
    }
    else {
        *(byte*)(src2 - FIRESKY_WIDTH) = 0;
    }
}

//
// R_Fire
//

static void R_Fire(byte *buffer) {
    int counter = 0;
    int rand = 0;
    int step = 0;
    int pixel = 0;
    byte *src;
    byte *srcoffset;

    rand = (M_Random() & 0xff);
    src = buffer;
    counter = 0;
    src += FIRESKY_WIDTH;

    do {  // width
        srcoffset = (src + counter);
        pixel = *(byte*)srcoffset;

        step = 2;

        R_SpreadFire(src, srcoffset, pixel, counter, &rand);

        src += FIRESKY_WIDTH;
        srcoffset += FIRESKY_WIDTH;

        do {  // height
            pixel = *(byte*)srcoffset;
            step += 2;

            R_SpreadFire(src, srcoffset, pixel, counter, &rand);

            pixel = *(byte*)(srcoffset + FIRESKY_WIDTH);
            src += FIRESKY_WIDTH;
            srcoffset += FIRESKY_WIDTH;

            R_SpreadFire(src, srcoffset, pixel, counter, &rand);

            src += FIRESKY_WIDTH;
            srcoffset += FIRESKY_WIDTH;

        }
        while(step < FIRESKY_HEIGHT);

        counter++;
        src -= ((FIRESKY_WIDTH*FIRESKY_HEIGHT)-FIRESKY_WIDTH);

    }
    while(counter < FIRESKY_WIDTH);
}

//
// R_InitFire
//

static rcolor firetexture[FIRESKY_WIDTH * FIRESKY_HEIGHT];

void R_InitFire(void) {
    int i;

    fireLump = W_GetNumForName("FIRE") - g_start;
    dmemset(&firePal16, 0, sizeof(dPalette_t)*256);
    for(i = 0; i < 16; i++) {
        firePal16[i].r = 16 * i;
        firePal16[i].g = 16 * i;
        firePal16[i].b = 16 * i;
        firePal16[i].a = 0xff;
    }

    fireBuffer = I_PNGReadData(g_start + fireLump,
                               true, true, false, 0, 0, 0, 0);

    for(i = 0; i < 4096; i++) {
        fireBuffer[i] >>= 4;
    }
}

//
// R_FireTicker
//

static void R_FireTicker(void) {
    if(leveltime & 1) {
        R_Fire(fireBuffer);
    }
}

//
// R_DrawFire
//

static void R_DrawFire(void) {
    float pos1;
    vtx_t v[4];
    dtexture t = gfxptr[fireLump];
    int i;

    //
    // copy fire pixel data to texture data array
    //
    dmemset(firetexture, 0, sizeof(int) * FIRESKY_WIDTH * FIRESKY_HEIGHT);
    for(i = 0; i < FIRESKY_WIDTH * FIRESKY_HEIGHT; i++) {
        byte rgb[3];

        rgb[0] = firePal16[fireBuffer[i]].r;
        rgb[1] = firePal16[fireBuffer[i]].g;
        rgb[2] = firePal16[fireBuffer[i]].b;

        firetexture[i] = D_RGBA(rgb[2], rgb[1], rgb[0], 0xff);
    }

    if(!t) {
        dglGenTextures(1, &gfxptr[fireLump]);
    }

    dglBindTexture(GL_TEXTURE_2D, gfxptr[fireLump]);
    GL_CheckFillMode();
    GL_SetTextureFilter();

    if(devparm) {
        glBindCalls++;
    }


    if(!t) {
        //
        // copy data if it didn't exist before
        //
        dglTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            FIRESKY_WIDTH,
            FIRESKY_HEIGHT,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            firetexture
        );
    }
    else {
        //
        // update texture data
        //
        dglTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            0,
            0,
            FIRESKY_WIDTH,
            FIRESKY_HEIGHT,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            firetexture
        );
    }

    if(r_skybox.value <= 0) {
        SKYVIEWPOS(viewangle, 4, pos1);

        //
        // adjust UV by 0.0035f units due to the fire sky showing a
        // strip of color going along the top portion of the texture
        //
        GL_Set2DQuad(v, 0, 0, SCREENWIDTH, 120,
                     pos1, 5.0f + pos1, 0.0035f, 1.0f, 0);

        dglSetVertexColor(&v[0], sky->skycolor[0], 2);
        dglSetVertexColor(&v[2], sky->skycolor[1], 2);

        GL_Draw2DQuad(v, 1);
    }
    else {
        R_DrawSkyDome(16, 1, 1024, 4096, -896, 0.0075f,
                      sky->skycolor[0], sky->skycolor[1]);
    }
}

//
// R_DrawSky
//

void R_DrawSky(void) {
    if(!sky) {
        return;
    }

    if(sky->flags & SKF_VOID) {
        R_DrawVoidSky();
    }
    else if(skypicnum >= 0) {
        if(sky->flags & SKF_CLOUD) {
            if(r_skybox.value <= 0) {
                R_DrawClouds();
            }
            else {
                R_DrawSkyboxCloud();
            }
        }
        else {
            if(r_skybox.value <= 0) {
                R_DrawSimpleSky(skypicnum, 128);
            }
            else {
                GL_SetTextureUnit(0, true);
                GL_BindGfxTexture(lumpinfo[skypicnum].name, true);

                //
                // drawer will assume that the texture's
                // dimensions is already in powers of 2
                //
                R_DrawSkyDome(4, 2, 512, 1024,
                              0, 0, WHITE, WHITE);
            }
        }
    }

    if(sky->flags & SKF_FIRE) {
        R_DrawFire();
    }

    if(skybackdropnum >= 0) {
        if(sky->flags & SKF_FADEBACK) {
            R_DrawTitleSky();
        }
        else if(sky->flags & SKF_BACKGROUND) {
            if(r_skybox.value <= 0) {
                R_DrawSimpleSky(skybackdropnum, 170);
            }
            else {
                float h;
                float origh;
                int l;
                int domeheight;
                float offset;
                float base;

                GL_SetTextureUnit(0, true);
                l = GL_BindGfxTexture(lumpinfo[skybackdropnum].name, true);

                //
                // handle the case for non-powers of 2 texture
                // dimensions. height and offset is adjusted
                // accordingly
                //
                origh = (float)gfxorigheight[l];
                h = (float)gfxheight[l];
                base = 160.0f - ((128 - origh) / 2.0f);
                domeheight = (int)(base / (origh / h));
                offset = (float)domeheight - base - 16.0f;

                R_DrawSkyDome(5, 1, domeheight, 768,
                              offset, 0.005f, WHITE, WHITE);
            }
        }
    }
}

//
// R_SkyTicker
//

void R_SkyTicker(void) {
    if(menuactive) {
        return;
    }

    if(!sky) {
        return;
    }

    if(sky->flags & SKF_CLOUD) {
        R_CloudTicker();
    }

    if(sky->flags & SKF_FIRE) {
        R_FireTicker();
    }

    if(sky->flags & SKF_FADEBACK) {
        R_TitleSkyTicker();
    }
}


