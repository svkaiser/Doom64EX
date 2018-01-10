// -*- mode: c++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2018 dotfloat
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//-----------------------------------------------------------------------------

#include "rom_private.hh"

using namespace imp;

/* From Wadgen's wad.c
 *
 * Based off of JaguarDoom's decompression algorithm.
 * This is a rather simple LZSS-type algorithm that was used
 * on all lumps in JaguardDoom and PSXDoom.
 * Doom64 only uses this on the Sprites and GFX lumps, but uses
 * a more sophisticated algorithm for everything else.
 *
 * ---
 *
 * As with other members of the LZ- family of compression algorithms, the
 * compressed data is a stream of "codes". Each code is either a character
 * literal, or a dictionary pointer. The dictionary is the currently
 * decompressed buffer, with the rightmost element being the latest
 * decompressed character. The pointer is an offset-length pair, with the
 * offset being from the right. Ie. an offset of 0 refers to the latest char.
 *
 * This compression scheme prefixes a bitset of size 8 (a byte) which encodes
 * the type of the next 8 codes. If the bit is 0 then it's a character
 * literal, otherwise it's a dictionary pointer.
 *
 * A character literal is an 1-byte code, which is written as is to the output
 * buffer.
 *
 * A dictionary pointer is a 2-byte code with the first 12 bits being the
 * offset, and the next 4 being the length. If these 4 bits are all 0s, then
 * the decompression terminates (EOS). There is little point in encoding a
 * single character with a dictionary pointer, so the length is incremented by
 * 1. We get a possible length of [2, 16].
 */
std::string rom::lzss(std::istream &in)
{
    std::string out;

    int getidbyte {};
    int idbyte {};

    for (;;) {
        if (getidbyte == 0) {
            idbyte = in.get();
        }

        /* assign a new idbyte every 8th loop */
        getidbyte = (getidbyte + 1) & 7;

        if (idbyte & 1) {
            /* dictionary pointer */
            int off = (in.get() << 4u) | (in.peek() >> 4u);
            int len = in.get() & 0xfu;

            /* if length == 0, then we've reached end of stream */
            if (len == 0)
                break;

            auto beg = out.size() - off - 1;
            auto end = beg + len + 1;

            /* copy dictionary into output */
            for (; beg < end; ++beg)
                out.push_back(out[beg]);
        } else {
            /* character literal */
            char c;
            in.get(c);
            out.push_back(c);
        }

        /* shift to next bit and begin the check at the beginning */
        idbyte >>= 1;
    }

    return out;
}
