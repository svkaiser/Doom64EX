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
// DESCRIPTION: Common drawing functions
//
//-----------------------------------------------------------------------------

#include <stdarg.h>
#include "doomtype.h"
#include "doomstat.h"
#include "dgl.h"
#include "r_things.h"
#include "gl_texture.h"
#include "gl_draw.h"
#include "r_main.h"

//
// Draw_GfxImage
//

void Draw_GfxImage(int x, int y, const char* name, rcolor color, dboolean alpha) {
    int gfxIdx = GL_BindGfxTexture(name, alpha);

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

    GL_SetState(GLSTATE_BLEND, 1);
    GL_SetupAndDraw2DQuad((float)x, (float)y,
                          gfxwidth[gfxIdx], gfxheight[gfxIdx], 0, 1.0f, 0, 1.0f, color, 0);

    GL_SetState(GLSTATE_BLEND, 0);
}

//
// Draw_Sprite2D
//

void Draw_Sprite2D(int type, int rot, int frame, int x, int y,
                   float scale, int pal, rcolor c) {
    spritedef_t    *sprdef;
    spriteframe_t *sprframe;
    float flip = 0.0f;
    int w;
    int h;
    int offsetx = 0;
    int offsety = 0;

    GL_SetState(GLSTATE_BLEND, 1);

    sprdef=&spriteinfo[type];
    sprframe = &sprdef->spriteframes[frame];

    GL_BindSpriteTexture(sprframe->lump[rot], pal);

    w = spritewidth[sprframe->lump[rot]];
    h = spriteheight[sprframe->lump[rot]];

    if(scale <= 1.0f) {
        if(sprframe->flip[rot]) {
            flip = 1.0f;
        }
        else {
            flip = 0.0f;
        }

        offsetx = (int)spriteoffset[sprframe->lump[rot]];
        offsety = (int)spritetopoffset[sprframe->lump[rot]];
    }

    GL_SetOrthoScale(scale);

    GL_SetupAndDraw2DQuad(flip ? (float)(x + offsetx) - w :
                          (float)x - offsetx, (float)y - offsety, w, h,
                          flip, 1.0f - flip, 0, 1.0f, c, 0);

    GL_SetOrthoScale(1.0f);

    cursprite = -1;
    curgfx = -1;

    GL_SetState(GLSTATE_BLEND, 0);
}

//
//
// STRING DRAWING ROUTINES
//
//

static vtx_t vtxstring[MAX_MESSAGE_SIZE];

//
// Draw_Text
//

int Draw_Text(int x, int y, rcolor color, float scale,
              dboolean wrap, const char* string, ...) {
    int c;
    int i;
    int vi = 0;
    int    col;
    const float size = 0.03125f;
    float fcol, frow;
    int start = 0;
    dboolean fill = false;
    char msg[MAX_MESSAGE_SIZE];
    va_list    va;
    const int ix = x;

    va_start(va, string);
    vsprintf(msg, string, va);
    va_end(va);

    GL_SetState(GLSTATE_BLEND, 1);

    if(!r_fillmode.value) {
        dglEnable(GL_TEXTURE_2D);
        dglPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        r_fillmode.value = 1.0f;
        fill = true;
    }

    GL_BindGfxTexture("SFONT", true);

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

    GL_SetOrthoScale(scale);
    GL_SetOrtho(0);

    dglSetVertex(vtxstring);

    for(i = 0, vi = 0; i < dstrlen(msg); i++, vi += 4) {
        c = toupper(msg[i]);
        if(c == '\t') {
            while(x % 64) {
                x++;
            }
            continue;
        }
        if(c == '\n') {
            y += ST_FONTWHSIZE;
            x = ix;
            continue;
        }
        if(c == 0x20) {
            if(wrap) {
                if(x > 192) {
                    y += ST_FONTWHSIZE;
                    x = ix;
                    continue;
                }
            }
        }
        else {
            start = (c - ST_FONTSTART);
            col = start & (ST_FONTNUMSET - 1);

            fcol = (col * size);
            frow = (start >= ST_FONTNUMSET) ? 0.5f : 0.0f;

            vtxstring[vi + 0].x     = (float)x;
            vtxstring[vi + 0].y     = (float)y;
            vtxstring[vi + 0].tu    = fcol + 0.0015f;
            vtxstring[vi + 0].tv    = frow + size;
            vtxstring[vi + 1].x     = (float)x + ST_FONTWHSIZE;
            vtxstring[vi + 1].y     = (float)y;
            vtxstring[vi + 1].tu    = (fcol + size) - 0.0015f;
            vtxstring[vi + 1].tv    = frow + size;
            vtxstring[vi + 2].x     = (float)x + ST_FONTWHSIZE;
            vtxstring[vi + 2].y     = (float)y + ST_FONTWHSIZE;
            vtxstring[vi + 2].tu    = (fcol + size) - 0.0015f;
            vtxstring[vi + 2].tv    = frow + 0.5f;
            vtxstring[vi + 3].x     = (float)x;
            vtxstring[vi + 3].y     = (float)y + ST_FONTWHSIZE;
            vtxstring[vi + 3].tu    = fcol + 0.0015f;
            vtxstring[vi + 3].tv    = frow + 0.5f;

            dglSetVertexColor(vtxstring + vi, color, 4);

            dglTriangle(vi + 0, vi + 1, vi + 2);
            dglTriangle(vi + 0, vi + 2, vi + 3);

            if(devparm) {
                vertCount += 4;
            }


        }
        x += ST_FONTWHSIZE;
    }

    if(vi) {
        dglDrawGeometry(vi, vtxstring);
    }

    GL_ResetViewport();

    if(fill) {
        dglDisable(GL_TEXTURE_2D);
        dglPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        r_fillmode.value = 0.0f;
    }

    GL_SetState(GLSTATE_BLEND, 0);
    GL_SetOrthoScale(1.0f);

    return x;
}

const symboldata_t symboldata[] = {  //0x5B9BC
    { 120, 14, 13, 13 },
    { 134, 14, 9, 13 },
    { 144, 14, 14, 13 },
    { 159, 14, 14, 13 },
    { 174, 14, 16, 13 },
    { 191, 14, 13, 13 },
    { 205, 14, 13, 13 },
    { 219, 14, 14, 13 },
    { 234, 14, 14, 13 },
    { 0, 29, 13, 13 },
    { 67, 28, 14, 13 },    // -
    { 36, 28, 15, 14 },    // %
    { 28, 28, 7, 14 },    // !
    { 14, 29, 6, 13 },    // .
    { 52, 28, 13, 13 },    // ?
    { 21, 29, 6, 13 },    // :
    { 0, 0, 13, 13 },
    { 14, 0, 13, 13 },
    { 28, 0, 13, 13 },
    { 42, 0, 14, 13 },
    { 57, 0, 14, 13 },
    { 72, 0, 10, 13 },
    { 87, 0, 15, 13 },
    { 103, 0, 15, 13 },
    { 119, 0, 6, 13 },
    { 126, 0, 13, 13 },
    { 140, 0, 14, 13 },
    { 155, 0, 11, 13 },
    { 167, 0, 15, 13 },
    { 183, 0, 16, 13 },
    { 200, 0, 15, 13 },
    { 216, 0, 13, 13 },
    { 230, 0, 15, 13 },
    { 246, 0, 13, 13 },
    { 0, 14, 14, 13 },
    { 15, 14, 14, 13 },
    { 30, 14, 13, 13 },
    { 44, 14, 15, 13 },
    { 60, 14, 15, 13 },
    { 76, 14, 15, 13 },
    { 92, 14, 13, 13 },
    { 106, 14, 13, 13 },
    { 83, 31, 10, 11 },
    { 93, 31, 10, 11 },
    { 103, 31, 11, 11 },
    { 114, 31, 11, 11 },
    { 125, 31, 11, 11 },
    { 136, 31, 11, 11 },
    { 147, 31, 12, 11 },
    { 159, 31, 12, 11 },
    { 171, 31, 4, 11 },
    { 175, 31, 10, 11 },
    { 185, 31, 11, 11 },
    { 196, 31, 9, 11 },
    { 205, 31, 12, 11 },
    { 217, 31, 13, 11 },
    { 230, 31, 12, 11 },
    { 242, 31, 11, 11 },
    { 0, 43, 12, 11 },
    { 12, 43, 11, 11 },
    { 23, 43, 11, 11 },
    { 34, 43, 10, 11 },
    { 44, 43, 11, 11 },
    { 55, 43, 12, 11 },
    { 67, 43, 13, 11 },
    { 80, 43, 13, 11 },
    { 93, 43, 10, 11 },
    { 103, 43, 11, 11 },
    { 0, 95, 108, 11 },
    { 108, 95, 6, 11 },
    { 0, 54, 32, 26 },
    { 32, 54, 32, 26 },
    { 64, 54, 32, 26 },
    { 96, 54, 32, 26 },
    { 128, 54, 32, 26 },
    { 160, 54, 32, 26 },
    { 192, 54, 32, 26 },
    { 224, 54, 32, 26 },
    { 134, 97, 7, 11 },
    { 114, 95, 20, 18 },
    { 105, 80, 15, 15 },
    { 120, 80, 15, 15 },
    { 135, 80, 15, 15 },
    { 150, 80, 15, 15 },
    { 45, 80, 15, 15 },
    { 60, 80, 15, 15 },
    { 75, 80, 15, 15 },
    { 90, 80, 15, 15 },
    { 165, 80, 15, 15 },
    { 180, 80, 15, 15 },
    { 0, 80, 15, 15 },
    { 15, 80, 15, 15 },
    { 195, 80, 15, 15 },
    { 30, 80, 15, 15 },
    { 156, 96, 13, 13 },
    { 143, 96, 13, 13 },
    { 169, 96, 7, 13 },
    { -1, -1, -1, -1 }
};

//
// Center_Text
//

int Center_Text(const char* string) {
    int width = 0;
    char t = 0;
    int id = 0;
    int len = 0;
    int i = 0;
    float scale;

    len = dstrlen(string);

    for(i = 0; i < len; i++) {
        t = string[i];

        switch(t) {
        case 0x20:
            width += 6;
            break;
        case '-':
            width += symboldata[SM_MISCFONT].w;
            break;
        case '%':
            width += symboldata[SM_MISCFONT + 1].w;
            break;
        case '!':
            width += symboldata[SM_MISCFONT + 2].w;
            break;
        case '.':
            width += symboldata[SM_MISCFONT + 3].w;
            break;
        case '?':
            width += symboldata[SM_MISCFONT + 4].w;
            break;
        case ':':
            width += symboldata[SM_MISCFONT + 5].w;
            break;
        default:
            if(t >= 'A' && t <= 'Z') {
                id = t - 'A';
                width += symboldata[SM_FONT1 + id].w;
            }
            if(t >= 'a' && t <= 'z') {
                id = t - 'a';
                width += symboldata[SM_FONT2 + id].w;
            }
            if(t >= '0' && t <= '9') {
                id = t - '0';
                width += symboldata[SM_NUMBERS + id].w;
            }
            break;
        }
    }

    scale = GL_GetOrthoScale();

    if(scale != 1.0f) {
        return ((int)(160.0f / scale) - (width / 2));
    }

    return (160 - (width / 2));
}

//
// Draw_BigText
//

int Draw_BigText(int x, int y, rcolor color, const char* string) {
    int c = 0;
    int i = 0;
    int vi = 0;
    int index = 0;
    float vx1 = 0.0f;
    float vy1 = 0.0f;
    float vx2 = 0.0f;
    float vy2 = 0.0f;
    float tx1 = 0.0f;
    float tx2 = 0.0f;
    float ty1 = 0.0f;
    float ty2 = 0.0f;
    float smbwidth;
    float smbheight;
    int pic;

    if(x <= -1) {
        x = Center_Text(string);
    }

    y += 14;

    pic = GL_BindGfxTexture("SYMBOLS", true);

    smbwidth = (float)gfxwidth[pic];
    smbheight = (float)gfxheight[pic];

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);

    dglSetVertex(vtxstring);

    GL_SetState(GLSTATE_BLEND, 1);
    GL_SetOrtho(0);

    for(i = 0, vi = 0; i < dstrlen(string); i++, vi += 4) {
        vx1 = (float)x;
        vy1 = (float)y;

        c = string[i];
        if(c == '\n' || c == '\t') {
            continue;    // villsa: safety check
        }
        else if(c == 0x20) {
            x += 6;
            continue;
        }
        else {
            if(c >= '0' && c <= '9') {
                index = (c - '0') + SM_NUMBERS;
            }
            if(c >= 'A' && c <= 'Z') {
                index = (c - 'A') + SM_FONT1;
            }
            if(c >= 'a' && c <= 'z') {
                index = (c - 'a') + SM_FONT2;
            }
            if(c == '-') {
                index = SM_MISCFONT;
            }
            if(c == '%') {
                index = SM_MISCFONT + 1;
            }
            if(c == '!') {
                index = SM_MISCFONT + 2;
            }
            if(c == '.') {
                index = SM_MISCFONT + 3;
            }
            if(c == '?') {
                index = SM_MISCFONT + 4;
            }
            if(c == ':') {
                index = SM_MISCFONT + 5;
            }

            // [kex] use 'printf' style formating for special symbols
            if(c == '/') {
                c = string[++i];

                switch(c) {
                // up arrow
                case 'u':
                    index = SM_MICONS + 17;
                    break;
                // down arrow
                case 'd':
                    index = SM_MICONS + 16;
                    break;
                // right arrow
                case 'r':
                    index = SM_MICONS + 18;
                    break;
                // left arrow
                case 'l':
                    index = SM_MICONS;
                    break;
                // cursor box
                case 'b':
                    index = SM_MICONS + 1;
                    break;
                // thermbar
                case 't':
                    index = SM_THERMO;
                    break;
                // thermcursor
                case 's':
                    index = SM_THERMO + 1;
                    break;
                default:
                    return 0;
                }
            }

            vx2 = vx1 + symboldata[index].w;
            vy2 = vy1 - symboldata[index].h;

            tx1 = ((float)symboldata[index].x / smbwidth) + 0.001f;
            tx2 = (tx1 + (float)symboldata[index].w / smbwidth) - 0.002f;

            ty1 = ((float)symboldata[index].y / smbheight);
            ty2 = ty1 + (((float)symboldata[index].h / smbheight));

            vtxstring[vi + 0].x     = vx1;
            vtxstring[vi + 0].y     = vy1;
            vtxstring[vi + 0].tu    = tx1;
            vtxstring[vi + 0].tv    = ty2;
            vtxstring[vi + 1].x     = vx2;
            vtxstring[vi + 1].y     = vy1;
            vtxstring[vi + 1].tu    = tx2;
            vtxstring[vi + 1].tv    = ty2;
            vtxstring[vi + 2].x     = vx2;
            vtxstring[vi + 2].y     = vy2;
            vtxstring[vi + 2].tu    = tx2;
            vtxstring[vi + 2].tv    = ty1;
            vtxstring[vi + 3].x     = vx1;
            vtxstring[vi + 3].y     = vy2;
            vtxstring[vi + 3].tu    = tx1;
            vtxstring[vi + 3].tv    = ty1;

            dglSetVertexColor(vtxstring + vi, color, 4);

            dglTriangle(vi + 2, vi + 1, vi + 0);
            dglTriangle(vi + 3, vi + 2, vi + 0);

            if(devparm) {
                vertCount += 4;
            }

            x += symboldata[index].w;
        }
    }

    if(vi) {
        dglDrawGeometry(vi, vtxstring);
    }

    GL_ResetViewport();
    GL_SetState(GLSTATE_BLEND, 0);

    return x;
}

//
// Draw_Number
//
//

void Draw_Number(int x, int y, int num, int type, rcolor c) {
    int digits[16];
    int nx = 0;
    int count;
    int j;
    char str[2];

    for(count = 0, j = 0; count < 16; count++, j++) {
        digits[j] = num % 10;
        nx += symboldata[SM_NUMBERS + digits[j]].w;

        num /= 10;

        if(!num) {
            break;
        }
    }

    if(type == 0) {
        x -= (nx >> 1);
    }

    if(type == 0 || type == 1) {
        if(count < 0) {
            return;
        }

        while(count >= 0) {
            sprintf(str, "%i", digits[j]);
            Draw_BigText(x, y, c, str);

            x += symboldata[SM_NUMBERS + digits[j]].w;

            count--;
            j--;
        }
    }
    else {
        if(count < 0) {
            return;
        }

        j = 0;

        while(count >= 0) {
            x -= symboldata[SM_NUMBERS + digits[j]].w;

            sprintf(str, "%i", digits[j]);
            Draw_BigText(x, y, c, str);

            count--;
            j++;
        }
    }
}

static const symboldata_t confontmap[256] = {
    { 0, 1, 13, 16 },
    { 14, 1, 13, 16 },
    { 28, 1, 13, 16 },
    { 42, 1, 13, 16 },
    { 56, 1, 13, 16 },
    { 70, 1, 13, 16 },
    { 84, 1, 13, 16 },
    { 98, 1, 13, 16 },
    { 112, 1, 13, 16 },
    { 126, 1, 13, 16 },
    { 140, 1, 13, 16 },
    { 154, 1, 13, 16 },
    { 168, 1, 13, 16 },
    { 182, 1, 13, 16 },
    { 196, 1, 13, 16 },
    { 210, 1, 13, 16 },
    { 224, 1, 13, 16 },
    { 238, 1, 13, 16 },
    { 0, 18, 13, 16 },
    { 14, 18, 13, 16 },
    { 28, 18, 13, 16 },
    { 42, 18, 13, 16 },
    { 56, 18, 13, 16 },
    { 70, 18, 13, 16 },
    { 84, 18, 13, 16 },
    { 98, 18, 13, 16 },
    { 112, 18, 13, 16 },
    { 126, 18, 13, 16 },
    { 140, 18, 13, 16 },
    { 154, 18, 13, 16 },
    { 168, 18, 13, 16 },
    { 182, 18, 13, 16 },
    { 196, 18, 5, 16 },
    { 202, 18, 5, 16 },
    { 208, 18, 5, 16 },
    { 214, 18, 10, 16 },
    { 225, 18, 8, 16 },
    { 234, 18, 13, 16 },
    { 0, 35, 9, 16 },
    { 10, 35, 3, 16 },
    { 14, 35, 6, 16 },
    { 21, 35, 6, 16 },
    { 28, 35, 9, 16 },
    { 38, 35, 9, 16 },
    { 48, 35, 5, 16 },
    { 54, 35, 7, 16 },
    { 62, 35, 5, 16 },
    { 68, 35, 6, 16 },
    { 75, 35, 8, 16 },
    { 84, 35, 8, 16 },
    { 93, 35, 8, 16 },
    { 102, 35, 8, 16 },
    { 111, 35, 8, 16 },
    { 120, 35, 8, 16 },
    { 129, 35, 8, 16 },
    { 138, 35, 8, 16 },
    { 147, 35, 8, 16 },
    { 156, 35, 8, 16 },
    { 165, 35, 6, 16 },
    { 172, 35, 6, 16 },
    { 179, 35, 9, 16 },
    { 189, 35, 9, 16 },
    { 199, 35, 9, 16 },
    { 209, 35, 7, 16 },
    { 217, 35, 13, 16 },
    { 231, 35, 9, 16 },
    { 241, 35, 8, 16 },
    { 0, 52, 9, 16 },
    { 10, 52, 9, 16 },
    { 20, 52, 8, 16 },
    { 29, 52, 8, 16 },
    { 38, 52, 9, 16 },
    { 48, 52, 9, 16 },
    { 58, 52, 5, 16 },
    { 64, 52, 6, 16 },
    { 71, 52, 8, 16 },
    { 80, 52, 7, 16 },
    { 88, 52, 11, 16 },
    { 100, 52, 9, 16 },
    { 110, 52, 10, 16 },
    { 121, 52, 8, 16 },
    { 130, 52, 10, 16 },
    { 141, 52, 8, 16 },
    { 150, 52, 9, 16 },
    { 160, 52, 9, 16 },
    { 170, 52, 9, 16 },
    { 180, 52, 9, 16 },
    { 190, 52, 13, 16 },
    { 204, 52, 9, 16 },
    { 214, 52, 9, 16 },
    { 224, 52, 9, 16 },
    { 234, 52, 6, 16 },
    { 241, 52, 6, 16 },
    { 248, 52, 6, 16 },
    { 0, 69, 11, 16 },
    { 12, 69, 8, 16 },
    { 21, 69, 8, 16 },
    { 30, 69, 8, 16 },
    { 39, 69, 8, 16 },
    { 48, 69, 8, 16 },
    { 57, 69, 8, 16 },
    { 66, 69, 8, 16 },
    { 75, 69, 5, 16 },
    { 81, 69, 8, 16 },
    { 90, 69, 8, 16 },
    { 99, 69, 3, 16 },
    { 103, 69, 4, 16 },
    { 108, 69, 7, 16 },
    { 116, 69, 3, 16 },
    { 120, 69, 11, 16 },
    { 132, 69, 8, 16 },
    { 141, 69, 8, 16 },
    { 150, 69, 8, 16 },
    { 159, 69, 8, 16 },
    { 168, 69, 5, 16 },
    { 174, 69, 7, 16 },
    { 182, 69, 6, 16 },
    { 189, 69, 8, 16 },
    { 198, 69, 8, 16 },
    { 207, 69, 11, 16 },
    { 219, 69, 7, 16 },
    { 227, 69, 8, 16 },
    { 236, 69, 7, 16 },
    { 244, 69, 8, 16 },
    { 0, 86, 7, 16 },
    { 8, 86, 8, 16 },
    { 17, 86, 11, 16 },
    { 29, 86, 13, 16 },
    { 43, 86, 13, 16 },
    { 57, 86, 13, 16 },
    { 71, 86, 13, 16 },
    { 85, 86, 13, 16 },
    { 99, 86, 13, 16 },
    { 113, 86, 13, 16 },
    { 127, 86, 13, 16 },
    { 141, 86, 13, 16 },
    { 155, 86, 13, 16 },
    { 169, 86, 13, 16 },
    { 183, 86, 13, 16 },
    { 197, 86, 13, 16 },
    { 211, 86, 13, 16 },
    { 225, 86, 13, 16 },
    { 239, 86, 13, 16 },
    { 0, 103, 13, 16 },
    { 14, 103, 13, 16 },
    { 28, 103, 13, 16 },
    { 42, 103, 13, 16 },
    { 56, 103, 13, 16 },
    { 70, 103, 13, 16 },
    { 84, 103, 13, 16 },
    { 98, 103, 13, 16 },
    { 112, 103, 13, 16 },
    { 126, 103, 13, 16 },
    { 140, 103, 13, 16 },
    { 154, 103, 13, 16 },
    { 168, 103, 13, 16 },
    { 182, 103, 13, 16 },
    { 196, 103, 13, 16 },
    { 210, 103, 13, 16 },
    { 224, 103, 13, 16 },
    { 238, 103, 5, 16 },
    { 244, 103, 5, 16 },
    { 0, 120, 8, 16 },
    { 9, 120, 8, 16 },
    { 18, 120, 8, 16 },
    { 27, 120, 8, 16 },
    { 36, 120, 7, 16 },
    { 44, 120, 8, 16 },
    { 53, 120, 8, 16 },
    { 62, 120, 13, 16 },
    { 76, 120, 7, 16 },
    { 84, 120, 8, 16 },
    { 93, 120, 9, 16 },
    { 103, 120, 7, 16 },
    { 111, 120, 13, 16 },
    { 125, 120, 8, 16 },
    { 134, 120, 7, 16 },
    { 142, 120, 9, 16 },
    { 152, 120, 7, 16 },
    { 160, 120, 7, 16 },
    { 168, 120, 8, 16 },
    { 177, 120, 8, 16 },
    { 186, 120, 8, 16 },
    { 195, 120, 5, 16 },
    { 201, 120, 8, 16 },
    { 210, 120, 7, 16 },
    { 218, 120, 7, 16 },
    { 226, 120, 8, 16 },
    { 235, 120, 13, 16 },
    { 0, 137, 13, 16 },
    { 14, 137, 13, 16 },
    { 28, 137, 7, 16 },
    { 36, 137, 9, 16 },
    { 46, 137, 9, 16 },
    { 56, 137, 9, 16 },
    { 66, 137, 9, 16 },
    { 76, 137, 9, 16 },
    { 86, 137, 9, 16 },
    { 96, 137, 12, 16 },
    { 109, 137, 9, 16 },
    { 119, 137, 8, 16 },
    { 128, 137, 8, 16 },
    { 137, 137, 8, 16 },
    { 146, 137, 8, 16 },
    { 155, 137, 5, 16 },
    { 161, 137, 5, 16 },
    { 167, 137, 5, 16 },
    { 173, 137, 5, 16 },
    { 179, 137, 9, 16 },
    { 189, 137, 9, 16 },
    { 199, 137, 10, 16 },
    { 210, 137, 10, 16 },
    { 221, 137, 10, 16 },
    { 232, 137, 10, 16 },
    { 243, 137, 10, 16 },
    { 0, 154, 11, 16 },
    { 12, 154, 10, 16 },
    { 23, 154, 9, 16 },
    { 33, 154, 9, 16 },
    { 43, 154, 9, 16 },
    { 53, 154, 9, 16 },
    { 63, 154, 9, 16 },
    { 73, 154, 8, 16 },
    { 82, 154, 8, 16 },
    { 91, 154, 8, 16 },
    { 100, 154, 8, 16 },
    { 109, 154, 8, 16 },
    { 118, 154, 8, 16 },
    { 127, 154, 8, 16 },
    { 136, 154, 8, 16 },
    { 145, 154, 11, 16 },
    { 157, 154, 8, 16 },
    { 166, 154, 8, 16 },
    { 175, 154, 8, 16 },
    { 184, 154, 8, 16 },
    { 193, 154, 8, 16 },
    { 202, 154, 3, 16 },
    { 206, 154, 3, 16 },
    { 210, 154, 3, 16 },
    { 214, 154, 3, 16 },
    { 218, 154, 8, 16 },
    { 227, 154, 8, 16 },
    { 236, 154, 8, 16 },
    { 245, 154, 8, 16 },
    { 0, 171, 8, 16 },
    { 9, 171, 8, 16 },
    { 18, 171, 8, 16 },
    { 27, 171, 9, 16 },
    { 37, 171, 8, 16 },
    { 46, 171, 8, 16 },
    { 55, 171, 8, 16 },
    { 64, 171, 8, 16 },
    { 73, 171, 8, 16 },
    { 82, 171, 8, 16 },
    { 91, 171, 8, 16 },
    { 100, 171, 8, 16 }
};

//
// Draw_ConsoleText
//

float Draw_ConsoleText(float x, float y, rcolor color,
                       float scale, const char* string, ...) {
    int c = 0;
    int i = 0;
    int vi = 0;
    float vx1 = 0.0f;
    float vy1 = 0.0f;
    float vx2 = 0.0f;
    float vy2 = 0.0f;
    float tx1 = 0.0f;
    float tx2 = 0.0f;
    float ty1 = 0.0f;
    float ty2 = 0.0f;
    char msg[MAX_MESSAGE_SIZE];
    va_list    va;
    float width;
    float height;
    int pic;

    va_start(va, string);
    vsprintf(msg, string, va);
    va_end(va);

    pic = GL_BindGfxTexture("CONFONT", true);

    width = (float)gfxwidth[pic];
    height = (float)gfxheight[pic];

    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, DGL_CLAMP);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    dglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    dglSetVertex(vtxstring);

    GL_SetState(GLSTATE_BLEND, 1);
    GL_SetOrtho(0);

    for(i = 0, vi = 0; i < dstrlen(msg); i++, vi += 4) {
        vx1 = x;
        vy1 = y;

        c = msg[i];
        if(c == '\n' || c == '\t') {
            continue;    // villsa: safety check
        }
        else {
            vx2 = vx1 + ((float)confontmap[c].w * scale);
            vy2 = vy1 - ((float)confontmap[c].h * scale);

            tx1 = ((float)confontmap[c].x / width) + 0.001f;
            tx2 = (tx1 + (float)confontmap[c].w / width) - 0.002f;

            ty1 = ((float)confontmap[c].y / height);
            ty2 = ty1 + (((float)confontmap[c].h / height));

            vtxstring[vi + 0].x     = vx1;
            vtxstring[vi + 0].y     = vy1;
            vtxstring[vi + 0].tu    = tx1;
            vtxstring[vi + 0].tv    = ty2;
            vtxstring[vi + 1].x     = vx2;
            vtxstring[vi + 1].y     = vy1;
            vtxstring[vi + 1].tu    = tx2;
            vtxstring[vi + 1].tv    = ty2;
            vtxstring[vi + 2].x     = vx2;
            vtxstring[vi + 2].y     = vy2;
            vtxstring[vi + 2].tu    = tx2;
            vtxstring[vi + 2].tv    = ty1;
            vtxstring[vi + 3].x     = vx1;
            vtxstring[vi + 3].y     = vy2;
            vtxstring[vi + 3].tu    = tx1;
            vtxstring[vi + 3].tv    = ty1;

            dglSetVertexColor(vtxstring + vi, color, 4);

            dglTriangle(vi + 2, vi + 1, vi + 0);
            dglTriangle(vi + 3, vi + 2, vi + 0);

            if(devparm) {
                vertCount += 4;
            }

            x += ((float)confontmap[c].w * scale);
        }
    }

    if(vi) {
        dglDrawGeometry(vi, vtxstring);
    }

    GL_ResetViewport();
    GL_SetState(GLSTATE_BLEND, 0);

    return x;
}

