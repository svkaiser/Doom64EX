// -*- mode: c++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2017 Zohar Malamant
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

#include <istream>
#include <utility/endian.hh>
#include <easy/profiler.h>

#include "Image.hh"

Rgba5551Palette imp::read_n64palette(std::istream &s, size_t count)
{
    EASY_FUNCTION(profiler::colors::Green50);
    constexpr size_t r_mask = 0b0000'0000'0011'1110;
    constexpr size_t r_shr  = 1;
    constexpr size_t g_mask = 0b0000'0111'1100'0000;
    constexpr size_t g_shr  = 6;
    constexpr size_t b_mask = 0b1111'1000'0000'0000;
    constexpr size_t b_shr  = 11;
    constexpr size_t a_mask = 0b0000'0000'0000'0001;
    constexpr size_t a_shr  = 0;

    Rgba5551Palette pal { count };
    for (auto& c : pal) {
        uint16 color;
        s.read(reinterpret_cast<char*>(&color), 2);
        color = swap_bytes(color);
        size_t d = color;
        c.blue  = (d & r_mask) >> r_shr;
        c.green = (d & g_mask) >> g_shr;
        c.red   = (d & b_mask) >> b_shr;
        c.alpha = (d & a_mask) >> a_shr;
    }

    return pal;
}
