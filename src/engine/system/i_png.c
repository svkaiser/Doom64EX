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
#include <w_wad.h>

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

//
// I_PNGRowSize
//

// FIXME: What is this for?
static inline size_t I_PNGRowSize(int width, byte bits) {
    if(bits >= 8) {
        return ((width * bits) >> 3);
    }
    else {
        return (((width * bits) + 7) >> 3);
    }
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

static void I_TranslatePalette(byte *dest, size_t color_size) {
    int i = 0;

    for(i = 0; i < 256; i += color_size) {
        dest[i * color_size + 0] = I_GetRGBGamma(dest[i * color_size + 0]);
        dest[i * color_size + 1] = I_GetRGBGamma(dest[i * color_size + 1]);
        dest[i * color_size + 2] = I_GetRGBGamma(dest[i * color_size + 2]);
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
    assert(image);

    // look for offset chunk if specified
    if(offset) {
        offset[0] = Image_GetOffsets(image).x;
        offset[1] = Image_GetOffsets(image).y;
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

        I_TranslatePalette(pal, pal_bytes);
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
