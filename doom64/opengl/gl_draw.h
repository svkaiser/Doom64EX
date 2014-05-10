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

#ifndef __GL_DRAW_H__
#define __GL_DRAW_H__

#include "gl_main.h"

void Draw_GfxImage(int x, int y, const char* name,
                   rcolor color, dboolean alpha);
void Draw_Sprite2D(int type, int rot, int frame, int x, int y,
                   float scale, int pal, rcolor c);

#define SM_FONT1        16
#define SM_FONT2        42
#define SM_MISCFONT        10
#define SM_NUMBERS        0
#define SM_SKULLS        70
#define SM_THERMO        68
#define SM_MICONS        78

typedef struct {
    int x;
    int y;
    int w;
    int h;
} symboldata_t;

extern const symboldata_t symboldata[];

#define ST_FONTWHSIZE    8
#define ST_FONTNUMSET    32    //# of fonts per row in font pic
#define ST_FONTSTART    '!'    // the first font characters
#define ST_FONTEND        '_'    // the last font characters
#define ST_FONTSIZE        (ST_FONTEND - ST_FONTSTART + 1) // Calculate # of glyphs in font.

int Draw_Text(int x, int y, rcolor color, float scale,
              dboolean wrap, const char* string, ...);
int Center_Text(const char* string);
int Draw_BigText(int x, int y, rcolor color, const char* string);
void Draw_Number(int x, int y, int num, int type, rcolor c);
float Draw_ConsoleText(float x, float y, rcolor color,
                       float scale, const char* string, ...);

#endif

