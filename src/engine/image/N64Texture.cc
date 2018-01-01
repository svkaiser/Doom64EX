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

#include <utility/endian.hh>

#include <imp/Wad>
#include <easy/profiler.h>
#include "Image.hh"

namespace {
  struct N64Texture : ImageFormatIO {
      Optional<Image> load(wad::Lump&) const override;
  };

  struct Header {
      uint16 id;
      uint16 numpal;
      uint16 wshift;
      uint16 hshift;
  };
}

Optional<Image> N64Texture::load(wad::Lump& lump) const
{
    EASY_FUNCTION(profiler::colors::Green50);
    auto& s = lump.stream();
    Header header;
    s.read(reinterpret_cast<char*>(&header), sizeof(header));

    header.hshift = big_endian(header.hshift);
    header.id = big_endian(header.id);
    header.numpal = big_endian(header.numpal);
    header.wshift = big_endian(header.wshift);

    uint16 width = static_cast<uint16>(1 << header.wshift);
    uint16 height = static_cast<uint16>(1 << header.hshift);

    I8Rgba5551Image image { width, height };

    for (size_t i = 0; i + 2 <= image.size(); i += 2) {
        auto p = image.data_ptr() + i;
        auto c = s.get();
        p[0] = static_cast<uint8>((c & 0xf0) >> 4);
        p[1] = static_cast<uint8>(c & 0x0f);
    }

    auto mask = image.width() / 8;
    for (size_t i {}; i + 16 <= image.size(); i += 16) {
        if ((i / 8) & mask) {
            auto pos = image.data_ptr() + i;
            std::swap_ranges(pos, pos + 8, pos + 8);
        }
    }

    auto palsize = static_cast<size_t>(header.numpal) * 16;
    image.palette(read_n64palette(s, palsize));

    return Image { image };
}

std::unique_ptr<ImageFormatIO> init::image_n64texture()
{
    return std::make_unique<N64Texture>();
}
