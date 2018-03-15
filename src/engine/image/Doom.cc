// -*- mode: c++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2016 Zohar Malamant
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

#include "Image.hh"

namespace {
  struct Doom : ImageFormatIO {
      Optional<Image> load(std::istream&) const override;
  };

  struct Header {
      uint16 width;
      uint16 height;
      uint16 left;
      uint16 top;
  };

  static_assert(sizeof(Header) == 8, "Incorrect packing");
}

Optional<Image> Doom::load(std::istream& s) const
{
    size_t fstart = static_cast<size_t>(s.tellg());

    Header header;
    s.read(reinterpret_cast<char*>(&header), sizeof header);

    int columns[header.width];
    for (int i = 0; i < header.width; i++)
        s.read(reinterpret_cast<char*>(&columns[i]), sizeof(int));

    I8RgbImage image { header.width, header.height };

    for (int x = 0; x < header.width; x++)
    {
        int row_start = 0;
        s.seekg(fstart + columns[x], std::ios::beg);

        while (row_start != 0xff)
        {
            int pixel_count {};

            row_start = s.get();
            if (row_start == 0xff)
                break;

            pixel_count = s.get();
            s.get();

            for (int y = 0; y < pixel_count; y++) {
                image[y + row_start].index(x, s.get());
            }

            s.get();
        }
    }

    Image r { std::move(image) };
    //image.set_palette(Palette::playpal());
    //image.set_trans(0);
    r.sprite_offset({header.left, header.top});

    return make_optional<Image>(std::move(r));
}

UniquePtr<ImageFormatIO> imp::init::image_doom()
{
    return make_unique<Doom>();
}
