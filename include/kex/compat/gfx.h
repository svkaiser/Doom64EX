// -*- mode: c -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2016 Zohar Malamant
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

#ifndef __KEX_GFX_H__10293413
#define __KEX_GFX_H__10293413

#ifdef __cplusplus
#error "This header can only be used in C code"
#endif //__cplusplus

#include <stdint.h>

typedef struct Image Image;
typedef struct Palette Palette;

typedef enum pixel_format {
    PF_NONE,
    PF_INDEX8,
    PF_RGB,
    PF_RGBA,
} pixel_format;

typedef enum pixel_interp {
    INTERP_NEAREST
} pixel_interp;

typedef enum pixel_extrap {
    EXTRAP_NEAREST
} pixel_extrap;

typedef struct SpriteOffsets {
    int x;
    int y;
} SpriteOffsets;

Image* Image_New(uint16_t width, uint16_t height, pixel_format format);
Image* Image_New_FromData(pixel_format format, uint16_t width, uint16_t height, char *data);
Image* Image_New_FromMemory(char *data, long size);
int Image_Save(Image*, const char *filename, const char *format);
void Image_Resize(Image *src, uint16_t new_width, uint16_t new_height);
void Image_Free(Image* ptr);
uint16_t Image_GetWidth(Image*);
uint16_t Image_GetHeight(Image*);
void *Image_GetData(Image*);
const Palette *Image_GetPalette(Image*);
SpriteOffsets Image_GetOffsets(Image*);
int Image_IsIndexed(Image*);
void Image_Convert(Image*, pixel_format);
void Image_Scale(Image*, uint16_t new_width, uint16_t new_height);

void *Palette_GetData(Palette*);
int Palette_GetCount(Palette*);
int Palette_HasAlpha(Palette*);

#endif //__KEX_GFX_H__10293413
