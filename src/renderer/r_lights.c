// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
// DESCRIPTION: Light rendering/blending functions
//
//-----------------------------------------------------------------------------

#include <math.h>

#include "doomstat.h"

#include "r_local.h"
#include "d_keywds.h"
#include "p_local.h"

rcolor    bspColor[5];

CVAR_CMD(i_brightness, 100) {
    R_RefreshBrightness();
}

CVAR_EXTERNAL(r_texturecombiner);

//
// R_GetSlopeLight
//

d_inline static int R_GetSlopeLight(int p, float c) {
    return (int)(p / (255 / c));
}

//
// R_LightToVertex
//

void R_LightToVertex(vtx_t *v, int idx, word c) {
    int i = 0;

    for(i = 0; i < c; i++) {
        v[i].a = 0xff;
        v[i].r = lights[idx].active_r;
        v[i].g = lights[idx].active_g;
        v[i].b = lights[idx].active_b;
    }
}

//
// R_LightGetHSV
// Set HSV values based on given RGB
//

void R_LightGetHSV(int r, int g, int b, int *h, int *s, int *v) {
    int min = r;
    int max = r;
    float delta = 0.0f;
    float j = 0.0f;
    float x = 0.0f;
    float xr = 0.0f;
    float xg = 0.0f;
    float xb = 0.0f;
    float sum = 0.0f;

    if(g < min) {
        min = g;
    }
    if(b < min) {
        min = b;
    }

    if(g > max) {
        max = g;
    }
    if(b > max) {
        max = b;
    }

    delta = ((float)max / 255.0f);

    if(dfcmp(delta, 0.0f)) {
        delta = 0;
    }
    else {
        j = ((delta - ((float)min / 255.0f)) / delta);
    }

    if(!dfcmp(j, 0.0f)) {
        xr = ((float)r / 255.0f);

        if(!dfcmp(xr, delta)) {
            xg = ((float)g / 255.0f);

            if(!dfcmp(xg, delta)) {
                xb = ((float)b / 255.0f);

                if(dfcmp(xb, delta)) {
                    sum = ((((delta - xg) / (delta - (min / 255.0f))) + 4.0f) -
                           ((delta - xr) / (delta - (min / 255.0f))));
                }
            }
            else {
                sum = ((((delta - xr) / (delta - (min / 255.0f))) + 2.0f) -
                       ((delta - (b / 255.0f)) /(delta - (min / 255.0f))));
            }
        }
        else {
            sum = (((delta - (b / 255.0f))) / (delta - (min / 255.0f))) -
                  ((delta - (g / 255.0f)) / (delta - (min / 255.0f)));
        }

        x = (sum * 60.0f);

        if(x < 0) {
            x += 360.0f;
        }
    }
    else {
        j = 0.0f;
    }

    *h = (int)((x / 360.0f) * 255.0f);
    *s = (int)(j * 255.0f);
    *v = (int)(delta * 255.0f);
}

//
// R_LightGetRGB
// Set RGB values based on given HSV
//

void R_LightGetRGB(int h, int s, int v, int *r, int *g, int *b) {
    float x = 0.0f;
    float j = 0.0f;
    float i = 0.0f;
    int table = 0;
    float xr = 0.0f;
    float xg = 0.0f;
    float xb = 0.0f;

    j = (h / 255.0f) * 360.0f;

    if(360.0f <= j) {
        j -= 360.0f;
    }

    x = (s / 255.0f);
    i = (v / 255.0f);

    if(!dfcmp(x, 0.0f)) {
        table = (int)(j / 60.0f);
        if(table < 6) {
            float t = (j / 60.0f);
            switch(table) {
            case 0:
                xr = i;
                xg = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                xb = ((1.0f - x) * i);
                break;
            case 1:
                xr = ((1.0f - (x * (t - (float)table))) * i);
                xg = i;
                xb = ((1.0f - x) * i);
                break;
            case 2:
                xr = ((1.0f - x) * i);
                xg = i;
                xb = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                break;
            case 3:
                xr = ((1.0f - x) * i);
                xg = ((1.0f - (x * (t - (float)table))) * i);
                xb = i;
                break;
            case 4:
                xr = ((1.0f - ((1.0f - (t - (float)table)) * x)) * i);
                xg = ((1.0f - x) * i);
                xb = i;
                break;
            case 5:
                xr = i;
                xg = ((1.0f - x) * i);
                xb = ((1.0f - (x * (t - (float)table))) * i);
                break;
            }
        }
    }
    else {
        xr = xg = xb = i;
    }

    *r = (int)(xr * 255.0f);
    *g = (int)(xg * 255.0f);
    *b = (int)(xb * 255.0f);
}

//
// R_SetLightFactor
//

void R_SetLightFactor(float lightfactor) {
    int l;
    light_t* light;
    float f;

    f = lightfactor / 100.0f;

    for(l = 0; l < numlights; l++) {
        light = &lights[l];

        if(l >= 256) {
            int h, s, v;

            R_LightGetHSV(light->r, light->g, light->b, &h, &s, &v);

            v = MIN((int)((float)v * f), 255);

            R_LightGetRGB(h, s, v, (int*)&light->base_r, (int*)&light->base_g, (int*)&light->base_b);
        }
        else {
            light->base_r = light->base_g = light->base_b = MIN((int)((float)l * f), 255);
        }

        light->active_r = light->base_r;
        light->active_g = light->base_g;
        light->active_b = light->base_b;
    }
}

//
// R_RefreshBrightness
//

void R_RefreshBrightness(void) {
    float factor;
    float brightness = i_brightness.value;

    factor = (((infraredFactor > brightness) ? infraredFactor : brightness) + 100.0f);
    R_SetLightFactor(factor);
}

//
// R_GetSectorLight
//

rcolor R_GetSectorLight(byte alpha, word ptr) {
    return D_RGBA((byte)lights[ptr].active_r,
                  (byte)lights[ptr].active_g, (byte)lights[ptr].active_b, alpha);
}

//
// R_SplitLineColor
//

rcolor R_SplitLineColor(seg_t *line, byte side) {
    int height=0;
    int sideheight1=0;
    int sideheight2=0;
    float r1, g1, b1;
    float r2, g2, b2;
    rcolor d3dc1=0;
    rcolor d3dc2=0;

    height = (line->frontsector->ceilingheight - line->frontsector->floorheight)/FRACUNIT;
    d3dc1 = bspColor[LIGHT_UPRWALL];
    d3dc2 = bspColor[LIGHT_LWRWALL];

    b1 = (float)((d3dc1 >> 16) & 0xff);
    g1 = (float)((d3dc1 >> 8) & 0xff);
    r1 = (float)(d3dc1 & 0xff);

    b2 = (float)((d3dc2 >> 16) & 0xff);
    g2 = (float)((d3dc2 >> 8) & 0xff);
    r2 = (float)(d3dc2 & 0xff);

    if(side == 1) {      /*TOP*/
        sideheight1 = (line->backsector->ceilingheight - line->frontsector->floorheight)/FRACUNIT;
        sideheight2 = (line->frontsector->ceilingheight - line->backsector->ceilingheight)/FRACUNIT;
    }
    else if(side == 2) {  /*BOTTOM*/
        sideheight1 = (line->backsector->floorheight - line->frontsector->floorheight)/FRACUNIT;
        sideheight2 = (line->frontsector->ceilingheight - line->backsector->floorheight)/FRACUNIT;
    }

    r1 = ((r1/height)*sideheight1);
    g1 = ((g1/height)*sideheight1);
    b1 = ((b1/height)*sideheight1);

    r2 = ((r2/height)*sideheight2);
    g2 = ((g2/height)*sideheight2);
    b2 = ((b2/height)*sideheight2);

    return D_RGBA((byte)MIN((r1+r2), 0xff),(byte)MIN((g1+g2), 0xff),(byte)MIN((b1+b2), 0xff), 0xff);
}

//
// R_SetSegLineColor
//

void R_SetSegLineColor(seg_t *line, vtx_t* v, byte side) {
    int i;
    rcolor c[4];
    byte lwr = LIGHT_LWRWALL;
    byte upr = LIGHT_UPRWALL;

    if(line->linedef->flags & ML_BLENDING) {
        if(line->backsector && side != 0) {
            if(!(line->linedef->flags & ML_BLENDFULLTOP) && side == 1) {
                c[2] = R_SplitLineColor(line, 1);
                c[3] = c[2];
            }
            else {
                if(side == 1 && line->linedef->flags & ML_INVERSEBLEND) {
                    lwr = LIGHT_UPRWALL;
                }

                c[2] = bspColor[lwr];
                c[3] = bspColor[lwr];
            }
            if(!(line->linedef->flags & ML_BLENDFULLBOTTOM) && side == 2) {
                c[0] = R_SplitLineColor(line, 2);
                c[1] = c[0];
            }
            else {
                if(side == 1 && line->linedef->flags & ML_INVERSEBLEND) {
                    upr = LIGHT_LWRWALL;
                }

                c[0] = bspColor[upr];
                c[1] = bspColor[upr];
            }
            if(side == 3) { // midtexture
                if(line->backsector->ceilingheight < line->frontsector->ceilingheight) {
                    c[0] = R_SplitLineColor(line, 1);
                    c[1] = c[0];
                }
                else {
                    c[0] = bspColor[LIGHT_UPRWALL];
                    c[1] = bspColor[LIGHT_UPRWALL];
                }
                if(line->backsector->floorheight > line->frontsector->floorheight) {
                    c[2] = R_SplitLineColor(line, 2);
                    c[3] = c[2];
                }
                else {
                    c[2] = bspColor[LIGHT_LWRWALL];
                    c[3] = bspColor[LIGHT_LWRWALL];
                }
            }
        }
        else {
            c[0] = bspColor[LIGHT_UPRWALL];
            c[1] = bspColor[LIGHT_UPRWALL];
            c[2] = bspColor[LIGHT_LWRWALL];
            c[3] = bspColor[LIGHT_LWRWALL];
        }

    }
    else {
        for(i = 0; i < 4; i++) {
            c[i] = bspColor[LIGHT_THING];
        }
    }

    for(i = 0; i < 4; i++) {
        *(rcolor*)&v[i].r = c[i];
    }
}

