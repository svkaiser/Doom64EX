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
  template <class T>
  constexpr PixelInfo pi()
  {
      using Traits = pixel_traits<T>;
      return PixelInfo {
          .format = Traits::format,
          .color = Traits::color,
          .bytes = Traits::bytes,
          .alpha = Traits::alpha,
          .pal_size = Traits::pal_size
      };
  }

  const PixelInfo traits[] = {
      PixelInfo(),
      pi<Index8>(),
      pi<Rgb>(),
      pi<Rgba>()
  };
}

PixelFormatError::PixelFormatError():
    std::logic_error("PixelFormatError") {}

const PixelInfo &gfx::get_pixel_info(PixelFormat format)
{
    for (auto &t : traits)
        if (t.format == format)
            return t;

    throw PixelFormatError("Couldn't find PixelInfo with this format");
}

Palette::Palette(const Palette &other):
    Palette()
{
    *this = other;
}

Palette::Palette(PixelFormat format, size_t count, std::unique_ptr<byte[]> data):
    mTraits(&get_pixel_info(format)),
    mCount(count)
{
    if (!mTraits->color)
        throw PixelFormatError("Can't create a palette of non-colors");

    if (data) {
        mData = std::move(data);
    } else {
        auto size = mTraits->bytes * mCount;
        mData = std::make_unique<byte[]>(size);
        std::fill_n(mData.get(), size, 0);
    }
}

Palette& Palette::operator=(const Palette &other)
{
    if (other.empty()) {
        reset();
    } else {
        mTraits = other.mTraits;
        mCount  = other.mCount;

        auto size = mTraits->bytes * mCount;
        mData = std::make_unique<byte[]>(size);
        std::copy_n(other.mData.get(), size, mData.get());
    }

    return *this;
}
