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

    I8Rgba5551Image image { header.width, header.height, 4 };

    for (size_t y {}; y < image.height(); ++y) {
        for (size_t x {}; x < image.width(); ++x) {
            auto c = s.get();
            image[y].index(x, static_cast<uint8>(c));
        }
    }

    Rgba5551Palette pal { 256 };
    for (auto& c : pal) {
        char texc[2];
        s.read(texc, 2);
        std::swap(texc[0], texc[1]);
        c = *reinterpret_cast<Rgba5551*>(texc);
        c.alpha = 1;
    }
    image.palette(std::move(pal));

    return image;
}

std::unique_ptr<ImageFormatIO> init::image_n64gfx()
{
    return std::make_unique<N64Gfx>();
}
