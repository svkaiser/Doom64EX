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

#include "image/image.hh"
#include "utility/endian.hh"

#include "rom_private.hh"

using namespace imp::wad::rom;

namespace {
  struct Header {
      uint16 compressed; /**< If -1, then not compressed. None of the lumps in D64 are compressed. */
      uint16 unused;
      uint16 width;
      uint16 height;
  };
}

Optional<Image> GfxLump::read_image()
{
    auto s = this->p_stream();

    Header header;
    s.read(reinterpret_cast<char*>(&header), sizeof(header));

    header.compressed = big_endian(header.compressed);
    header.width = big_endian(header.width);
    header.height = big_endian(header.height);

    // The palette is located right after the image
    auto palofs = static_cast<size_t>(s.tellg()) + pad<8>(header.width * header.height);

    I8Rgba5551Image image { pad<4>(header.width), header.height };

    for (size_t y {}; y < header.height; ++y) {
        for (size_t x {}; x < header.width; ++x) {
            auto c = s.get();
            image[y].index(x, static_cast<uint8>(c));
        }
    }

    if (info().hack == Hack::cloud) {
        for (size_t i {}; i + 16 <= 64 * 64; i += 8) {
            if (!(i & 64))
                continue;
            auto pos = image.data_ptr() + i;
            std::swap_ranges(pos, pos + 4, pos + 4);
        }
    }

    if (info().hack != Hack::fire) {
        // Seek to the palette and read it
        s.seekg(palofs);
        image.set_palette(read_n64palette(s, 256));
    }

    return Image { image };
}
