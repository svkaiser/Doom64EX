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
  struct N64Texture : ImageFormatIO {
      Optional<Image> load(std::istream &) const override;
  };

  struct Header {
      uint16 id;
      uint16 numpal;
      uint16 wshift;
      uint16 hshift;
  };
}

Optional<Image> N64Texture::load(std::istream &s) const
{
    Header header;
    s.read(reinterpret_cast<char*>(&header), sizeof(header));

    header.hshift = big_endian(header.hshift);
    header.id = big_endian(header.id);
    header.numpal = big_endian(header.numpal);
    header.wshift = big_endian(header.wshift);

    I8Rgba5551Image image
        { static_cast<uint16>(1 << header.wshift),
          static_cast<uint16>(1 << header.hshift) };

    println("> {}x{}", image.width(), image.height());

    for (size_t y {}; y < image.height(); ++y) {
        for (size_t x {}; x < image.width(); x += 2) {
            auto c = s.get();
            image[y].index(x, static_cast<uint8>((c & 0xf0) >> 4));
            image[y].index(x+1, static_cast<uint8>(c & 0x0f));
        }
    }

    auto palsize = static_cast<size_t>(header.numpal) * 16;
    Rgba5551Palette pal { palsize };
    for (auto& c : pal) {
        char texc[2];
        s.read(texc, 2);
        std::swap(texc[0], texc[1]);
        c = *reinterpret_cast<Rgba5551*>(texc);
    }

    image.palette(std::move(pal));

    return image;
}

std::unique_ptr<ImageFormatIO> init::image_n64texture()
{
    return std::make_unique<N64Texture>();
}
