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

#include <ostream>
#include <set>
#include <map>

#include "image/palette_cache.hh"
#include "utility/endian.hh"

#include "rom_private.hh"

using namespace imp::wad::rom;

namespace {
  struct Header {
      int16 tiles;		///< how many tiles the sprite is divided into
      int16 compressed;	///< >=0 = 'two for one' byte compression, -1 = not compressed
      int16 cmpsize;		// actual compressed size (0 if not compressed)
      int16 xoffs;		///< draw x offset
      int16 yoffs;		///< draw y offset
      int16 width;		///< draw width
      int16 height;

      ///< draw height
      int16 tileheight;	///< y height per tile piece
  };
  static_assert(sizeof(Header) == 16, "Sprite header must be 16 bytes");

  Header read_header(std::istream& s)
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

      return header;
  }

  constexpr size_t g_image_pitch(size_t width, size_t align, size_t pixel_size)
  {
      width *= pixel_size;
      return static_cast<uint16>(width + ((align - (width & (align - 1))) & (align - 1)));
  }
}

SharedPtr<Palette> SpriteLump::m_palette()
{
    if (m_palette_ptr) {
        return m_palette_ptr;
    }

    if (m_palette_lump) {
        return m_palette_lump->m_palette();
    }

    auto s = p_stream();
    auto header = read_header(s);

    assert(header.compressed < 0);

    /* Jump to palette, which comes after the bitmap */
    auto image_size = pad<8>(header.width) * header.height;

    s.seekg(image_size, s.cur);

    auto palette = read_n64palette(s, 256);
    m_palette_ptr = std::make_shared<Palette>(std::move(palette));

    return m_palette_ptr;
}

Optional<Image> SpriteLump::read_image()
{
    auto s = this->p_stream();

    auto header = read_header(s);

    assert((header.width >= 2) && (header.width <= 256) && (header.height >= 2) && (header.height <= 256));

    uint16 align = header.compressed == -1 ? 8_u16 : 16_u16;

    I8Rgba5551Image image { static_cast<uint16>(header.width), static_cast<uint16>(header.height), align };
    Palette palette;

    if (header.compressed >= 0) {
        for (size_t y {}; y < image.height(); ++y) {

            for (size_t x {}; x + 2 <= image.pitch(); x += 2) {
                auto c = s.get();
                image[y].index(x, static_cast<uint8>((c & 0xf0) >> 4));
                image[y].index(x+1, static_cast<uint8>(c & 0x0f));
            }
        }

        palette = read_n64palette(s, 16);
    } else {
        for (size_t y = 0; y < image.height(); ++y)
            for (size_t x = 0; x < image.pitch(); ++x)
                s.get(image[y].data_ptr()[x]);

       if (m_is_weapon || this->section() == wad::Section::graphics) {
           palette = *m_palette();
       } else {
           auto name = fmt::format("PAL{}0", this->name().substr(0, 4));
           palette = cache::palette(name);
       }
    }

    int id {};
    int inv {};
    for (size_t y {}; y < image.height(); ++y, ++id) {
        if (id == header.tileheight) {
            id = 0;
            inv = 0;
        }

        if (inv) {
            for (size_t x{}; x + align <= image.pitch(); x += align) {
                auto data = image[y].data() + x;
                std::swap_ranges(data, data + align/2, data + align/2);
            }
        }
        inv ^= 1;
    }

   if (m_is_weapon) {
       header.xoffs -= 160;
       header.yoffs -= 208;
   }

    image.sprite_offset({ header.xoffs, header.yoffs });

    Image ret { std::move(image) };
    if (palette) {
        ret.set_palette(palette);
    }

    assert(ret.is_indexed());

    return ret;
}
