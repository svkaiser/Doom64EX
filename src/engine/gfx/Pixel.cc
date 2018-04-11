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
#include <imp/Pixel>
#include "Playpal.hh"

namespace {

  template <class T>
  constexpr PixelInfo pi()
  {
      using Traits = pixel_traits<T>;
      return PixelInfo {
          Traits::format,
          Traits::color,
          Traits::bytes,
          Traits::alpha,
          Traits::pal_size
      };
  }

  const PixelInfo traits[] = {
      PixelInfo(),
      pi<Index8>(),
      pi<Rgb>(),
      pi<Rgba>()
  };

  class CopyTransform : public DefaultPixelTransform<void> {
      const Palette *mSrcPal;
      const byte *mSrc;
      byte *mDst;

  public:
      CopyTransform(const Palette *srcPal, const byte *src, const Palette *, byte *dst):
          mSrcPal(srcPal),
          mSrc(src),
          mDst(dst) {}


      template <class SrcT, class, class DstT, class>
      void color_to_color()
      {
          auto& src = *reinterpret_cast<const SrcT*>(mSrc);
          auto& dst = *reinterpret_cast<DstT*>(mDst);

          dst = convert_pixel(src, pixel_traits<DstT>::tag());
      };

      template <class SrcT, class SrcPalT, class DstT, class>
      void index_to_color()
      {
          assert(mSrcPal);

          auto& srcIdx = *reinterpret_cast<const SrcT*>(mSrc);
          auto& src = mSrcPal->color_unsafe<SrcPalT>(srcIdx.index);
          auto& dst = *reinterpret_cast<DstT*>(mDst);

          dst = convert_pixel(src, pixel_traits<DstT>::tag());
      };
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

Palette::Palette(PixelFormat format, size_t count, const byte* data):
    mTraits(&get_pixel_info(format)),
    mCount(count)
{
    if (!mTraits->color)
        throw PixelFormatError("Can't create a palette of non-colors");

    if (data) {
        mData = std::make_unique<byte[]>(count * mTraits->bytes);
        std::copy_n(data, count * mTraits->bytes, mData.get());
    } else {
        auto size = mTraits->bytes * mCount;
        mData = std::make_unique<byte[]>(size);
        std::fill_n(mData.get(), size, 0);
    }
}

Palette::Palette(PixelFormat format, size_t count, std::unique_ptr<byte[]> data):
    mTraits(&get_pixel_info(format)),
    mCount(count)
{
    fmt::print(">> {}\n", static_cast<int>(format));

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

void imp::gfx::copy_pixel(PixelFormat srcFmt,
                          const Palette *srcPal,
                          const byte *src,
                          PixelFormat dstFmt,
                          const Palette *dstPal,
                          byte *dst)
{
    CopyTransform ct(srcPal, src, dstPal, dst);

    auto srcPalFmt = srcPal ? srcPal->format() : PixelFormat::none;
    auto dstPalFmt = dstPal ? dstPal->format() : PixelFormat::none;

    transform_pixel(srcFmt, srcPalFmt, dstFmt, dstPalFmt, ct);
}

std::shared_ptr<const Palette> Palette::black()
{
    static std::shared_ptr<const Palette> palette = nullptr;
    if (!palette)
        palette = std::make_shared<const Palette>(PixelFormat::rgb, 256, nullptr);

    return palette;
}

std::shared_ptr<const Palette> Palette::playpal()
{
    static std::shared_ptr<const Palette> palette = nullptr;
    if (!palette)
        palette = std::make_shared<const Palette>(::playpal);

    return palette;
}
