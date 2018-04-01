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

#include "doomdef.h"
#include "i_swap.h"
#include "z_zone.h"
#include "gl_texture.h"

#include <imp/Image>
#include <sstream>
#include <core/cvar.hh>
#include <image/PaletteCache.hh>
#include <wad.hh>

FloatCvar i_gamma("i_gamma", "", 0.0f, 0,
                      [](const FloatCvar&, float, float&)
                      {
                          GL_DumpTextures();
                      });

//
// I_GetRGBGamma
//

#if 0
d_inline static byte I_GetRGBGamma(int c) {
    return (byte)MIN(pow((float)c, (1.0f + (0.01f * i_gamma))), 255);
}
#endif

//
// I_TranslatePalette
// Increases the palette RGB based on gamma settings
//

/*
static void I_TranslatePalette(Palette &dest) {
    if (i_gamma == 0)
        return;

    auto color_count = dest.count();
    auto color_size = dest.pixel_info().width;

    for(size_t i = 0; i < color_count; i += color_size) {
        dest.data_ptr()[i + 0] = I_GetRGBGamma(dest.data_ptr()[i + 0]);
        dest.data_ptr()[i + 1] = I_GetRGBGamma(dest.data_ptr()[i + 1]);
        dest.data_ptr()[i + 2] = I_GetRGBGamma(dest.data_ptr()[i + 2]);
    }
}
*/

Image I_ReadImage(int lump, dboolean palette, dboolean nopack, double alpha, int palindex)
{
    // get lump data
    auto l = wad::open(lump).value();

    auto image = l.read_image().value();

    if (palindex && image.is_indexed())
    {
        auto pal = image.palette();

        char palname[9];
        snprintf(palname, sizeof(palname), "PAL%4.4s%d", l.name().data(), palindex);

        if (wad::exists(palname)) {
            image.palette(cache::palette(palname));
        } else {
            /*
            Palette newpal = { pal.pixel_format(), 16 };
            std::copy_n(pal.data_ptr(), 16 * palindex * pal.pixel_info().width, newpal.data_ptr());

            I_TranslatePalette(newpal);
            image.palette(std::move(newpal));
            */
        }
    }

    if (!palette)
        image.convert(alpha ? PixelFormat::rgba : PixelFormat::rgb);

    return image;
}
