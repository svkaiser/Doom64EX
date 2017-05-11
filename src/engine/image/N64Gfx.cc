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

#include "Image.hh"

namespace {
  template<size_t Pad>
  constexpr size_t int_pad(size_t x)
  {
      return x + ((Pad - (x & (Pad - 1))) & (Pad - 1));
  }

  struct N64Gfx : ImageFormatIO {
      Optional<Image> load(std::istream &) const override;
  };

  struct Header {
      uint16 compressed; /**< If -1, then not compressed. None of the lumps in D64 are compressed. */
      uint16 unused;
      uint16 width;
      uint16 height;
  };
}

Optional<Image> N64Gfx::load(std::istream &s) const
{
    Header header;
    s.read(reinterpret_cast<char*>(&header), sizeof(header));

    header.compressed = big_endian(header.compressed);
    header.width = big_endian(header.width);
    header.height = big_endian(header.height);

    assert(header.compressed == 0xffff);

    I8Rgba5551Image image { header.width, header.height };
    auto palofs = s.tellg() + int_pad<8>(header.width * header.height);

    for (size_t y {}; y < header.height; ++y) {
        for (size_t x {}; x < header.width; ++x) {
            auto c = s.get();
            if (x < image.width() && y < image.height())
                image[y].index(x, static_cast<uint8>(c));
        }
    }

    s.seekg(palofs);

    constexpr size_t r_mask = 0b0000'0000'0011'1110;
    constexpr size_t r_shr  = 1;
    constexpr size_t g_mask = 0b0000'0111'1100'0000;
    constexpr size_t g_shr  = 6;
    constexpr size_t b_mask = 0b1111'1000'0000'0000;
    constexpr size_t b_shr  = 11;
    constexpr size_t a_mask = 0b0000'0000'0000'0001;
    constexpr size_t a_shr  = 0;

    Rgba5551Palette pal { 256 };
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
    image.palette(std::move(pal));

    return image;
}

std::unique_ptr<ImageFormatIO> init::image_n64gfx()
{
    return std::make_unique<N64Gfx>();
}
