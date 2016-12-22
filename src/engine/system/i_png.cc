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

#include <algorithm>
#include <math.h>
#include <w_wad.h>

#include "doomdef.h"
#include "i_swap.h"
#include "z_zone.h"
#include "gl_texture.h"
#include "con_console.h"

#include <imp/Image>
#include <sstream>

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

static void I_TranslatePalette(gfx::Palette &dest) {
    int i = 0;

    if (i_gamma.value == 0)
        return;

    auto color_count = dest.count();
    auto color_size = dest.traits().bytes;

    for(i = 0; i < color_count; i += color_size) {
        dest.data_ptr()[i * color_size + 0] = I_GetRGBGamma(dest.data_ptr()[i * color_size + 0]);
        dest.data_ptr()[i * color_size + 1] = I_GetRGBGamma(dest.data_ptr()[i * color_size + 1]);
        dest.data_ptr()[i * color_size + 2] = I_GetRGBGamma(dest.data_ptr()[i * color_size + 2]);
    }
}

gfx::Image I_ReadImage(int lump, dboolean palette, dboolean nopack, double alpha, int palindex)
{
    char *lcache;
    int lsize;
    int i;

    // get lump data
    lcache = reinterpret_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));
    lsize = W_LumpLength(lump);

    std::istringstream ss(std::string(lcache, lsize));
    gfx::Image image {ss}; //= Image_New_FromMemory(lcache, lsize);

    if (palindex && image.is_indexed())
    {
        int pal_bytes;
        size_t pal_count;
        const byte *oldpal;

        auto pal = image.palette();
        oldpal = pal->data_ptr();
        pal_bytes = pal->traits().alpha ? 4 : 3;
        pal_count = pal->count();

        char palname[9];
        snprintf(palname, sizeof(palname), "PAL%4.4s%d", lumpinfo[lump].name, palindex);

        gfx::Palette newpal;
        if (W_CheckNumForName(palname) != -1)
        {
            gfx::Rgb *pallump = reinterpret_cast<gfx::Rgb *>(W_CacheLumpName(palname, PU_STATIC));
            newpal = *pal;

            // swap out current palette with the new one
            for (auto& c : newpal.map<gfx::Rgba>()) {
                c = gfx::Rgba { pallump->red, pallump->green, pallump->blue, c.alpha };
                pallump++;
            }
        } else {
            newpal = gfx::Palette { pal->format(), 16, pal->data_ptr() + (16 * palindex) * pal->traits().bytes };
            pal_count = 16;
        }

        I_TranslatePalette(newpal);
        image.set_palette(newpal);
    }

    if (!palette)
        image.convert(alpha ? gfx::PixelFormat::rgba : gfx::PixelFormat::rgb);

    return image;
}

//
// I_PNGReadData
//
void *I_PNGReadData(int lump, dboolean palette, dboolean nopack, dboolean alpha,
                    int* w, int* h, int* offset, int palindex)
{
    auto image = I_ReadImage(lump, palette, nopack, alpha, palindex);

    // look for offset chunk if specified
    if(offset) {
        offset[0] = image.offsets().x;
        offset[1] = image.offsets().y;
    }

    if(w) {
        *w = image.width();
    }
    if(h) {
        *h = image.height();
    }

    auto length = image.traits().bytes * image.width() * image.height();
    auto retval = reinterpret_cast<byte*>(malloc(length));
    std::copy_n(image.data_ptr(), length, retval);

    return retval;
}
