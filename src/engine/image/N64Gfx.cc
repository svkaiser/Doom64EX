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

#include <imp/util/Endian>

#include <imp/Wad>
#include <easy/profiler.h>
#include "Image.hh"

namespace {
  struct N64Gfx : ImageFormatIO {
      Optional<Image> load(wad::Lump&) const override;
  };

  struct Header {
      uint16 compressed; /**< If -1, then not compressed. None of the lumps in D64 are compressed. */
      uint16 unused;
      uint16 width;
      uint16 height;
  };
}

Optional<Image> N64Gfx::load(wad::Lump& lump) const
{
    EASY_FUNCTION(profiler::colors::Green50);
    bool is_fire = lump.lump_name() == "FIRE";
    bool is_cloud = lump.lump_name() == "CLOUD";
    auto& s = lump.stream();

    Header header;
    s.read(reinterpret_cast<char*>(&header), sizeof(header));

    header.compressed = big_endian(header.compressed);
    header.width = big_endian(header.width);
    header.height = big_endian(header.height);

    assert(is_fire || header.compressed == 0xffff);

    // The palette is located right after the image
    auto palofs = static_cast<size_t>(s.tellg()) + pad<8>(header.width * header.height);

    I8Rgba5551Image image { header.width, header.height };

    for (size_t y {}; y < header.height; ++y) {
        for (size_t x {}; x < header.width; ++x) {
            auto c = s.get();
            image[y].index(x, static_cast<uint8>(c));
        }
    }

    if (is_cloud) {
        constexpr size_t mask = 64;
        for (size_t i {}; i + 16 <= 64 * 64; i += 8) {
            if (!(i & mask))
                continue;
            auto pos = image.data_ptr() + i;
            std::swap_ranges(pos, pos + 4, pos + 4);
        }
    }

    if (!is_fire) {
        // Seek to the palette and read it
        s.seekg(palofs);
        image.palette(read_n64palette(s, 256));
    }

    return Image { image };
}

std::unique_ptr<ImageFormatIO> init::image_n64gfx()
{
    return std::make_unique<N64Gfx>();
}
