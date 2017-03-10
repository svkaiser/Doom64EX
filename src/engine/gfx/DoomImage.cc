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

#include <imp/Image>

namespace {

  struct DoomImage : ImageFormatIO {
      StringView mimetype() const override;
      bool is_format(std::istream&) const override;
      Image load(std::istream&) const override;
      void save(std::ostream&, const Image&) const override;
  };

  StringView DoomImage::mimetype() const
  { return "doom"; }

  bool DoomImage::is_format(std::istream &) const
  { return true; }

  struct Header {
      uint16 width;
      uint16 height;
      uint16 left;
      uint16 top;
  };
  static_assert(sizeof(Header) == 8, "Incorrect packing");

  Image DoomImage::load(std::istream &stream) const
  {
      size_t fstart = stream.tellg();

      Header header;
      stream.read(reinterpret_cast<char*>(&header), sizeof header);

      int columns[header.width];
      for (int i = 0; i < header.width; i++)
          stream.read(reinterpret_cast<char*>(&columns[i]), sizeof(int));

      Image image(PixelFormat::index8, header.width, header.height, nullptr);
      auto it = image.map<Index8>().begin();

      for (int x = 0; x < header.width; x++)
      {
          uint8 rowStart = 0;
          stream.seekg(fstart + columns[x], std::ios::beg);

          while (rowStart != 0xff)
          {
              uint8 pixelCount {};
              char dummy;

              stream.read(reinterpret_cast<char*>(&rowStart), 1);
              if (rowStart == 0xff)
                  break;

              stream.read(reinterpret_cast<char*>(&pixelCount), 1);
              stream.read(reinterpret_cast<char*>(&dummy), 1);

              for (int y = 0; y < pixelCount; y++)
              {
                  uint8 px;
                  stream.read(reinterpret_cast<char*>(&px), 1);
                  *(it + (y + rowStart) * header.width + x) = Index8 { px };
              }

              stream.read(reinterpret_cast<char*>(&dummy), 1);
          }
      }

      image.set_palette(Palette::playpal());
      image.set_trans(0);
      image.set_offsets({header.left, header.top});

      return image;
  }

  void DoomImage::save(std::ostream&, const Image&) const
  { throw ImageSaveError("Saving isn't implemented for Doom Pictures"); }
}

UniquePtr<ImageFormatIO> __initialize_doom_image()
{ return std::make_unique<DoomImage>(); }
