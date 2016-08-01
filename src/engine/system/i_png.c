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
#include <kex/compat/gfx.h>
#include <w_wad.h>
#include <SDL_timer.h>

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

CVAR_CMD(i_gamma, 0) {
    GL_DumpTextures();
}

struct chunk_read_io {
    byte *chunk;
    size_t pos;
};

typedef struct chunk_read_io chunk_read_io;

//
// I_PNGRowSize
//

static inline size_t I_PNGRowSize(int width, byte bits) {
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
    chunk_read_io *io = png_get_io_ptr(ctx);

    dmemcpy(area, (io->chunk + io->pos), size);
    io->pos += size;
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

Image *I_PNGReadData(int lump, dboolean palette, dboolean nopack, dboolean alpha,
                    int* w, int* h, int* offset, int palindex) {
    Image *image;
    void *lcache;
    int lsize;
    int i;

    // get lump data
    lcache = W_CacheLumpNum(lump, PU_STATIC);
    lsize = W_LumpLength(lump);

    image = Image_New_FromMemory(lcache, lsize);

    // look for offset chunk if specified
    if(offset) {
        offset[0] = Image_GetOffsets(image)[0];
        offset[1] = Image_GetOffsets(image)[1];
    }

    if (palindex && Image_IsIndexed(image))
    {
        Palette *palette;
        int pal_bytes;
        int pal_count;
        byte *pal;

        palette = Image_GetPalette(image);
        pal = Palette_GetData(palette);
        pal_bytes = Palette_HasAlpha(palette) ? 4 : 3;
        pal_count = Palette_GetCount(palette);

        char palname[9];
        snprintf(palname, sizeof(palname), "PAL%4.4s%d", lumpinfo[lump].name, palindex);

        if (W_CheckNumForName(palname) != -1)
        {
            byte *pallump = W_CacheLumpName(palname, PU_STATIC);

            // swap out current palette with the new one
            for (i = 0; i < pal_count; i++)
            {
                pal[i * pal_bytes + 0] = pallump[i * 3 + 0];
                pal[i * pal_bytes + 1] = pallump[i * 3 + 1];
                pal[i * pal_bytes + 2] = pallump[i * 3 + 2];
            }

            Z_Free(pallump);
        } else {
            dmemcpy(pal, pal + (16 * palindex) * pal_bytes, 16 * pal_bytes);
        }

        I_TranslatePalette(pal);
    }

    if (!palette)
        Image_Convert(image, alpha ? PF_RGBA : PF_RGB);

    if(w) {
        *w = Image_GetWidth(image);
    }
    if(h) {
        *h = Image_GetHeight(image);
    }

    return image;
}

//
// I_PNGWriteFunc
//

static void I_PNGWriteFunc(png_structp png_ptr, byte* data, size_t length) {
//    pngWriteData = (byte*)Z_Realloc(pngWriteData, pngWritePos + length, PU_STATIC, 0);
//    dmemcpy(pngWriteData + pngWritePos, data, length);
//    pngWritePos += length;
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
//    out = (byte*)Z_Malloc(pngWritePos, PU_STATIC, 0);
//    dmemcpy(out, pngWriteData, pngWritePos);
//    *size = pngWritePos;
//
//    Z_Free(pngWriteData);
//    pngWriteData = NULL;
//    pngWritePos = 0;

    return out;
}
