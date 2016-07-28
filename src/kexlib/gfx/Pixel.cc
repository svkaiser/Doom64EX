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

#include <algorithm>
#include <kex/gfx/Pixel>

using namespace kex::gfx;

namespace {
  const pixel_traits traits[] = {
      pixel_traits(),
      static_pixel_traits<Index4>{},
      static_pixel_traits<Index8>{},
      static_pixel_traits<Rgb>{},
      static_pixel_traits<Bgr>{},
      static_pixel_traits<Rgba>{},
      static_pixel_traits<Bgra>{}
  };

  size_t calc_length(const pixel_traits *traits, size_t count)
  {
      return traits ? traits->bytes * count : 0;
  }
}

bad_pixel_format::bad_pixel_format():
    std::logic_error("bad_pixel_format")
{
}

const pixel_traits& kex::gfx::get_pixel_traits(pixel_format format)
{
    for (const auto &t : traits)
        if (t.format == format)
            return t;

    return traits[0];
}

Palette::Palette(const Palette &other) noexcept:
    mColors(std::make_unique<uint8_t[]>(calc_length(other.mTraits, other.mCount))),
    mTraits(other.mTraits),
    mCount(other.mCount),
    mOffset(other.mOffset)
{
    std::copy_n(other.mColors.get(), calc_length(mTraits, mCount), mColors.get());
}

Palette& Palette::operator=(const Palette &other) noexcept
{
    mTraits = other.mTraits;
    mCount = other.mCount;

    auto length = calc_length(mTraits, mCount);
    mColors = std::make_unique<uint8_t[]>(length);
    std::copy_n(other.mColors.get(), length, mColors.get());

    return *this;
}
