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
#include <ostream>

#include "Image.hh"

namespace {
  struct N64Sprite : ImageFormatIO {
      Optional<Image> load(std::istream &) const override;
  };

  struct Header {
      int16 tiles;		///< how many tiles the sprite is divided into
      int16 compressed;	///< >=0 = 'two for one' byte compression, -1 = not compressed
      int16 cmpsize;		// actual compressed size (0 if not compressed)
      int16 xoffs;		///< draw x offset
      int16 yoffs;		///< draw y offset
      int16 width;		///< draw width
      int16 height;

      friend std::ostream &operator<<(std::ostream &os, const Header &header)
      {
          os << "tiles: " << header.tiles << " compressed: " << header.compressed << " cmpsize: " << header.cmpsize
             << " xoffs: " << header.xoffs << " yoffs: " << header.yoffs << " width: " << header.width << " height: "
             << header.height << " tileheight: " << header.tileheight;
          return os;
      }

      ///< draw height
      int16 tileheight;	///< y height per tile piece
  };
  static_assert(sizeof(Header) == 16, "N64Sprite header must be 16 bytes");
}

Optional<Image> N64Sprite::load(std::istream &s) const
{
    Header header;
    s.read(reinterpret_cast<char*>(&header), sizeof(header));

    header.tiles = big_endian(header.tiles);
    header.compressed = big_endian(header.compressed);
    header.cmpsize = big_endian(header.cmpsize);
    header.xoffs = big_endian(header.xoffs);
    header.yoffs = big_endian(header.yoffs);
    header.width = big_endian(header.width);
    header.height = big_endian(header.height);
    header.tileheight = big_endian(header.tileheight);

    I8Rgba5551Image image;

    assert((header.width > 0) && (header.width <= 256) && (header.height > 0) && (header.height <= 256));

    if (header.compressed >= 0) {
        image = { header.width, header.height, 16 };

        for (size_t y {}; y < image.height(); ++y) {
            for (size_t x {}; x < image.width(); x += 2) {
                auto c = s.get();
                image[y].index(x, static_cast<uint8>((c & 0xf0) >> 4));
                image[y].index(x+1, static_cast<uint8>(c & 0x0f));
            }
        }

        image.palette({ 16 });
    } else {
        image = { header.width, header.height, 8 };

        for (size_t y {}; y < image.height(); ++y) {
            for (size_t x {}; x < image.width(); ++x) {
                auto c = s.get();
                image[y].index(x, static_cast<uint8>(c));
            }
        }

        image.palette({ 256 });
    }

    image.sprite_offset({ header.xoffs, header.yoffs });

    return image;
}

std::unique_ptr<ImageFormatIO> init::image_n64sprite()
{
    return std::make_unique<N64Sprite>();
}
