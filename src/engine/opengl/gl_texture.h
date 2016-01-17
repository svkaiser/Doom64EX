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

#ifndef __GL_TEXTURE_H__
#define __GL_TEXTURE_H__

#include "gl_main.h"

extern int                  curtexture;
extern int                  cursprite;
extern int                    curtrans;
extern int                  curgfx;

extern word*                texturewidth;
extern word*                textureheight;
extern dtexture**           textureptr;
extern int                  t_start;
extern int                  t_end;
extern int                  swx_start;
extern int                  numtextures;
extern word*                texturetranslation;
extern word*                palettetranslation;

extern int                  g_start;
extern int                  g_end;
extern int                  numgfx;
extern dtexture*            gfxptr;
extern word*                gfxwidth;
extern word*                gfxorigwidth;
extern word*                gfxheight;
extern word*                gfxorigheight;

extern int                  s_start;
extern int                  s_end;
extern dtexture**           spriteptr;
extern int                  numsprtex;
extern word*                spritewidth;
extern float*               spriteoffset;
extern float*               spritetopoffset;
extern word*                spriteheight;

void        GL_InitTextures(void);
void        GL_UnloadTexture(dtexture* texture);
void        GL_SetTextureUnit(int unit, dboolean enable);
void        GL_SetTextureMode(int mode);
void        GL_SetCombineState(int combine);
void        GL_SetCombineStateAlpha(int combine);
void        GL_SetEnvColor(float* param);
void        GL_SetCombineSourceRGB(int source, int target);
void        GL_SetCombineSourceAlpha(int source, int target);
void        GL_SetCombineOperandRGB(int operand, int target);
void        GL_SetCombineOperandAlpha(int operand, int target);
void        GL_BindWorldTexture(int texnum, int *width, int *height);
void        GL_BindSpriteTexture(int spritenum, int pal);
int         GL_BindGfxTexture(const char* name, dboolean alpha);
int         GL_PadTextureDims(int size);
void        GL_SetNewPalette(int id, byte palID);
void        GL_DumpTextures(void);
void        GL_ResetTextures(void);
void        GL_BindDummyTexture(void);
void        GL_UpdateEnvTexture(rcolor color);
void        GL_BindEnvTexture(void);
dtexture    GL_ScreenToTexture(void);
void        GL_ResampleTexture(unsigned int *in, int inwidth, int inheight,
                               unsigned int *out, int outwidth, int outheight,
                               int type);

#endif