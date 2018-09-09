// -*- mode: c++ -*-
//-----------------------------------------------------------------------------
//
// Copyright(C) 2018 dotfloat
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//-----------------------------------------------------------------------------

#ifndef __ROM_PRIVATE__42659186
#define __ROM_PRIVATE__42659186

#include <string>
#include <istream>
#include <sstream>

#include "wad/wad.hh"
#include "image/image.hh"

namespace imp {
  namespace wad {
    namespace rom {
      std::string deflate(std::istream& s);
      std::string lzss(std::istream& s);

      enum struct Compression {
          none,
          lzss,
          deflate
      };

      enum struct Hack {
          none,
          cloud,
          fire
      };

      class Device;

      struct Info {
          String name;
          Section section;
          std::streampos pos;
          Hack hack {};
      };

      class Lump : public wad::ILump {
          Device& device_;
          Info info_;

      protected:
          std::istringstream p_stream();

          const Info& info() const
          { return info_; }

      public:
          Lump(Device& device, Info info):
              device_(device),
              info_(info) {}

          UniquePtr<std::istream> stream() override
          { throw std::logic_error(fmt::format("rom::Lump::stream() Not implemented for '{}'", info_.name)); }

          IDevice& device() override;

          String name() const override
          { return info_.name; }

          Section section() const override
          { return info_.section; }

          String real_name() const override
          { return info_.name; }
      };

      class TextureLump : public Lump {
      public:
          using Lump::Lump;
          Optional<Image> read_image() override;
      };

      class SpriteLump : public Lump {
          bool m_is_weapon;
          SpriteLump* m_palette_lump;
          SharedPtr<Palette> m_palette_ptr;

          SharedPtr<Palette> m_palette();

      public:
          SpriteLump(Device& device, Info info, bool is_weapon, SpriteLump* palette_lump):
              Lump(device, info),
              m_is_weapon(is_weapon),
              m_palette_lump(palette_lump) {}

          Optional<Image> read_image() override;
      };

      class GfxLump : public Lump {
      public:
          using Lump::Lump;
          Optional<Image> read_image() override;
      };

      class PaletteLump : public Lump {
      public:
          using Lump::Lump;

          Optional<Palette> read_palette() override;
      };

      class SoundLump : public Lump {
          int track_;

      public:
          SoundLump(Device& device, Info info, int track):
              Lump(device, info),
              track_(track) {}

          UniquePtr<std::istream> stream() override;
      };

      class NormalLump : public Lump {
      public:
          using Lump::Lump;

          UniquePtr<std::istream> stream() override
          { return std::make_unique<std::istringstream>(p_stream()); }
      };

      Rgba5551Palette read_n64palette(std::istream &s, size_t count);
    }
  }
}

#endif //__ROM_PRIVATE__42659186
