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
//    PNG image system
//
//-----------------------------------------------------------------------------

#include <math.h>

#include "doomdef.h"
#include "doomtype.h"
#include "i_system.h"
#include "i_swap.h"
#include "m_fixed.h"
#include "z_zone.h"
#include "w_wad.h"
#include "gl_texture.h"
#include "con_console.h"
#include "i_png.h"

static byte*    pngWriteData;
static byte*    pngReadData;
static size_t   pngWritePos = 0;

CVAR_CMD(i_gamma, 0) {
    GL_DumpTextures();
}

//
// I_PNGRowSize
//

d_inline static size_t I_PNGRowSize(int width, byte bits) {
    if(bits >= 8) {
        return ((width * bits) >> 3);
    }
    else {
        return (((width * bits) + 7) >> 3);
    }
}

//
// I_PNGReadFunc
//

static void I_PNGReadFunc(png_structp ctx, byte* area, size_t size) {
    dmemcpy(area, pngReadData, size);
    pngReadData += size;
}

//
// I_PNGFindChunk
//

static int I_PNGFindChunk(png_struct* png_ptr, png_unknown_chunkp chunk) {
    int *dat;

    if(!dstrncmp((char*)chunk->name, "grAb", 4) && chunk->size >= 8) {
        dat = (int*)png_get_user_chunk_ptr(png_ptr);
        dat[0] = I_SwapBE32(*(int*)(chunk->data));
        dat[1] = I_SwapBE32(*(int*)(chunk->data + 4));
        return 1;
    }

    return 0;
}

//
// I_GetRGBGamma
//

d_inline static byte I_GetRGBGamma(int c) {
    return (byte)MIN(pow((float)c, (1.0f + (0.01f * i_gamma.value))), 255);
}

//
// I_TranslatePalette
// Increases the palette RGB based on gamma settings
//

static void I_TranslatePalette(png_colorp dest) {
    int i = 0;

    for(i = 0; i < 256; i++) {
        dest[i].red     = I_GetRGBGamma(dest[i].red);
        dest[i].green   = I_GetRGBGamma(dest[i].green);
        dest[i].blue    = I_GetRGBGamma(dest[i].blue);
    }
}

//
// I_PNGReadData
//

byte* I_PNGReadData(int lump, dboolean palette, dboolean nopack, dboolean alpha,
                    int* w, int* h, int* offset, int palindex) {
    png_structp png_ptr;
    png_infop   info_ptr;
    png_uint_32 width;
    png_uint_32 height;
    int         bit_depth;
    int         color_type;
    int         interlace_type;
    int         pixel_depth;
    byte*       png;
    byte*       out;
    size_t      row;
    size_t      rowSize;
    byte**      row_pointers;

    // get lump data
    png = W_CacheLumpNum(lump, PU_STATIC);
    pngReadData = png;

    // setup struct
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(png_ptr == NULL) {
        I_Error("I_PNGReadData: Failed to read struct");
        return NULL;
    }

    // setup info struct
    info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL) {
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        I_Error("I_PNGReadData: Failed to create info struct");
        return NULL;
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        I_Error("I_PNGReadData: Failed on setjmp");
        return NULL;
    }

    // setup callback function for reading data
    png_set_read_fn(png_ptr, NULL, I_PNGReadFunc);

    // look for offset chunk if specified
    if(offset) {
        offset[0] = 0;
        offset[1] = 0;

        png_set_read_user_chunk_fn(png_ptr, offset, I_PNGFindChunk);
    }

    // read png information
    png_read_info(png_ptr, info_ptr);

    // get IHDR chunk
    png_get_IHDR(
        png_ptr,
        info_ptr,
        &width,
        &height,
        &bit_depth,
        &color_type,
        &interlace_type,
        NULL,
        NULL);

    if(usingGL && !alpha) {
        int num_trans = 0;
        png_get_tRNS(png_ptr, info_ptr, NULL, &num_trans, NULL);
        if(num_trans)
            //if(usingGL && !alpha && info_ptr->num_trans)
        {
            I_Error("I_PNGReadData: RGB8 PNG image (%s) has transparency", lumpinfo[lump].name);
        }
    }

    // if the data will be outputted as palette index data (non RGB(A))
    if(palette) {
        if(bit_depth == 4 && nopack) {
            png_set_packing(png_ptr);
        }
    }
    else {  // data will be outputted as RGB(A) data
        if(bit_depth == 16) {
            png_set_strip_16(png_ptr);
        }

        if(color_type == PNG_COLOR_TYPE_PALETTE) {
            int i = 0;
            png_colorp pal = NULL; //info_ptr->palette;
            int num_pal = 0;
            png_get_PLTE(png_ptr, info_ptr, &pal, &num_pal);  // FIXME: num_pal not used??

            if(palindex) {
                // palindex specifies each row (16 colors per row) in the palette for 4 bit color textures
                if(bit_depth == 4) {
                    for(i = 0; i < 16; i++) {
                        dmemcpy(&pal[i], &pal[(16 * palindex) + i], sizeof(png_color));
                    }
                }
                else if(bit_depth >= 8) {  // 8 bit and up requires an external palette lump
                    png_colorp pallump;
                    char palname[9];

                    sprintf(palname, "PAL");
                    dstrncpy(palname + 3, lumpinfo[lump].name, 4);
                    sprintf(palname + 7, "%i", palindex);

                    // villsa 12/04/13: don't abort if external palette is not found
                    if(W_CheckNumForName(palname) != -1) {
                        pallump = W_CacheLumpName(palname, PU_STATIC);

                        // swap out current palette with the new one
                        for(i = 0; i < 256; i++) {
                            pal[i].red = pallump[i].red;
                            pal[i].green = pallump[i].green;
                            pal[i].blue = pallump[i].blue;
                        }

                        Z_Free(pallump);
                    }
                    // villsa 12/04/13: if we're loading texture palette as normal
                    // but palindex is not zero, then just copy out a single row from the
                    // palette in case world textures have a 8-32 bit depth color table
                    else {
                        for(i = 0; i < 16; i++) {
                            dmemcpy(&pal[i], &pal[(16 * palindex) + i], sizeof(png_color));
                        }
                    }
                }
            }

            I_TranslatePalette(pal);
            png_set_palette_to_rgb(png_ptr);
        }

        if(alpha) {
            // add alpha values to the RGB data
            png_set_swap_alpha(png_ptr);
            png_set_add_alpha(png_ptr, 0xff, 0);
        }
    }

    // refresh png information
    png_read_update_info(png_ptr, info_ptr);

    // refresh data in IHDR chunk
    png_get_IHDR(
        png_ptr,
        info_ptr,
        &width,
        &height,
        &bit_depth,
        &color_type,
        &interlace_type,
        NULL,
        NULL);

    // get the size of each row
    pixel_depth = bit_depth;

    if(color_type == PNG_COLOR_TYPE_RGB) {
        pixel_depth *= 3;
    }
    else if(color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        pixel_depth *= 4;
    }

    rowSize = I_PNGRowSize(width, pixel_depth /*info_ptr->pixel_depth*/);

    if(w) {
        *w = width;
    }
    if(h) {
        *h = height;
    }

    // allocate output and row pointers
    out = (byte*)Z_Calloc(rowSize * height, PU_STATIC, 0);
    row_pointers = (byte**)Z_Malloc(sizeof(byte*)*height, PU_STATIC, 0);

    for(row = 0; row < height; row++) {
        row_pointers[row] = out + (row * rowSize);
    }

    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, info_ptr);

    if(alpha) {
        size_t i;
        int* check = (int*)out;

        // need to reverse the bytes to ABGR format
        for(i = 0; i < (height * rowSize) / 4; i++) {
            int c = *check;

            byte b = (byte)((c >> 24) & 0xff);
            byte g = (byte)((c >> 16) & 0xff);
            byte r = (byte)((c >> 8) & 0xff);
            byte a = (byte)(c & 0xff);

            *check = ((a<<24)|(b<<16)|(g<<8)|r);

            check++;
        }
    }

    //cleanup
    Z_Free(row_pointers);
    Z_Free(png);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    return out;
}

//
// I_PNGWriteFunc
//

static void I_PNGWriteFunc(png_structp png_ptr, byte* data, size_t length) {
    pngWriteData = (byte*)Z_Realloc(pngWriteData, pngWritePos + length, PU_STATIC, 0);
    dmemcpy(pngWriteData + pngWritePos, data, length);
    pngWritePos += length;
}

//
// I_PNGCreate
//

byte* I_PNGCreate(int width, int height, byte* data, int* size) {
    png_structp png_ptr;
    png_infop   info_ptr;
    int         i = 0;
    byte*       out;
    byte*       pic;
    byte**      row_pointers;
    size_t      j = 0;
    size_t      row;

    // setup png pointer
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if(png_ptr == NULL) {
        I_Error("I_PNGCreate: Failed getting png_ptr");
        return NULL;
    }

    // setup info pointer
    info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr,  NULL);
        I_Error("I_PNGCreate: Failed getting info_ptr");
        return NULL;
    }

    // what does this do again?
    if(setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        I_Error("I_PNGCreate: Failed on setjmp");
        return NULL;
    }

    // setup custom data writing procedure
    png_set_write_fn(png_ptr, NULL, I_PNGWriteFunc, NULL);

    // setup image
    png_set_IHDR(
        png_ptr,
        info_ptr,
        width,
        height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_DEFAULT);

    // add png info to data
    png_write_info(png_ptr, info_ptr);

    row_pointers = (byte**)Z_Malloc(sizeof(byte*) * height, PU_STATIC, 0);
    row = I_PNGRowSize(width, 24 /*info_ptr->pixel_depth*/);
    pic = data;

    for(i = 0; i < height; i++) {
        row_pointers[i] = NULL;
        row_pointers[i] = (byte*)Z_Malloc(row, PU_STATIC, 0);

        for(j = 0; j < row; j++) {
            row_pointers[i][j] = *pic;
            pic++;
        }
    }

    Z_Free(data);

    png_write_image(png_ptr, row_pointers);

    // cleanup
    png_write_end(png_ptr, info_ptr);

    for(i = 0; i < height; i++) {
        Z_Free(row_pointers[i]);
    }

    Z_Free(row_pointers);
    row_pointers = NULL;

    png_destroy_write_struct(&png_ptr, &info_ptr);

    // allocate output
    out = (byte*)Z_Malloc(pngWritePos, PU_STATIC, 0);
    dmemcpy(out, pngWriteData, pngWritePos);
    *size = pngWritePos;

    Z_Free(pngWriteData);
    pngWriteData = NULL;
    pngWritePos = 0;

    return out;
}
