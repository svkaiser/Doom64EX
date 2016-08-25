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
#include <vector>
#include <cstring>

#include <kex/lib>
#include <kex/gfx/Image>

using namespace kex::gfx;

namespace {
  std::vector<std::unique_ptr<ImageFormatIO>> image_formats;

  const ImageFormatIO &find_format_by_mimetype(StringView mimetype)
  {
      for (const auto &format : image_formats)
          if (format->mimetype() == mimetype)
              return *format;

      throw ImageError("Failed to find an appropriate image format based on mimetype");
  }

  const ImageFormatIO &find_format_by_reading(std::istream &stream)
  {
      const auto pos = stream.tellg();

      for (const auto &format: image_formats)
      {
          if (format->is_format(stream))
          {
              stream.seekg(pos);
              return *format;
          }
          stream.seekg(pos);
      }

      throw ImageError("Failed to detect image type");
  }

  size_t calc_length(const PixelInfo *traits, uint16 width, uint16 height)
  {
      if (traits->format == PixelFormat::none)
          throw PixelFormatError();

      return traits->bytes * width * height;
  }

  class ResizeProcessor : public DefaultPixelProcessor<> {
      const Image &mSrc;
      Image &mDst;

  public:
      ResizeProcessor(const Image &src, Image &dst):
          mSrc(src),
          mDst(dst) {}

      template <class T, class>
      void color()
      {
          int x, y;
          int h = std::min(mSrc.height(), mDst.height());
          int w = std::min(mSrc.width(), mDst.width());

          auto srcPad = std::max(0, mSrc.width() - mDst.width());
          auto srcIt = mSrc.map<T>().cbegin();
          auto dstIt = mDst.map<T>().begin();

          for (y = 0; y < h; y++)
          {
              for (x = 0; x < w; x++)
                  *dstIt++ = convert_pixel(*srcIt++, pixel_traits<T>::tag());

              for (; x < mDst.width(); x++)
                  *dstIt++ = { };

              srcIt += srcPad;
          }

          for (; y < mDst.height(); y++)
              for (x = 0; x < mDst.width(); x++)
                  *dstIt++ = { };
      }
  };

  class ScaleProcessor : public DefaultPixelProcessor<> {
      const Image &mSrc;
      Image &mDst;

  public:
      ScaleProcessor(const Image &src, Image &dst):
          mSrc(src),
          mDst(dst) {}

      template <class T, class>
      void color()
      {
          auto srcWidth = mSrc.width();
          auto width = mDst.width();
          auto height = mDst.height();
          auto dx = static_cast<double>(mSrc.width()) / width;
          auto dy = static_cast<double>(mSrc.height()) / height;

          auto srcIt = mSrc.map<T>().cbegin();
          auto dstIt = mDst.map<T>().begin();

          for (int y = 0; y < height; y++)
          {
              for (int x = 0; x < width; x++)
              {
                  size_t pos = static_cast<size_t>(dy * y) * srcWidth + static_cast<size_t>(dx * x);
                  *dstIt++ = convert_pixel(*(srcIt + pos), pixel_traits<T>::tag());
              }
          }
      }
  };

  class CompareTransform {
      const Image &mLhs;
      const Image &mRhs;

  public:
      CompareTransform(const Image &lhs, const Image &rhs):
          mLhs(lhs),
          mRhs(rhs) {}

      template <class SrcT, class, class DstT, class>
      bool color_to_color()
      {
          auto lhsIt = mLhs.map<SrcT>().cbegin();
          auto lhsEnd = mLhs.map<SrcT>().cend();
          auto rhsIt = mRhs.map<DstT>().cbegin();

          for (; lhsIt != lhsEnd; ++lhsIt, ++rhsIt)
              if (*lhsIt != *rhsIt)
                  return false;

          return true;
      }

      template <class SrcT, class, class DstT, class DstPalT>
      bool color_to_index()
      {
          auto lhsIt = mLhs.map<SrcT>().cbegin();
          auto lhsEnd = mLhs.map<SrcT>().cend();
          auto rhsIt = mRhs.map<DstT>().cbegin();

          auto rhsPal = mRhs.palette().get();

          for (; lhsIt != lhsEnd; ++lhsIt, ++rhsIt)
              if (*lhsIt != rhsPal->color_unsafe<DstPalT>(rhsIt->index))
                  return false;

          return true;
      };

      template <class SrcT, class SrcPalT, class DstT, class>
      bool index_to_color()
      {
          auto lhsIt = mLhs.map<SrcT>().cbegin();
          auto lhsEnd = mLhs.map<SrcT>().cend();
          auto rhsIt = mRhs.map<DstT>().cbegin();

          auto lhsPal = mLhs.palette().get();

          for (; lhsIt != lhsEnd; ++lhsIt, ++rhsIt)
              if (lhsPal->color_unsafe<SrcPalT>(lhsIt->index) != *rhsIt)
                  return false;

          return true;
      };

      template <class SrcT, class SrcPalT, class DstT, class DstPalT>
      bool index_to_index()
      {
          auto lhsIt = mLhs.map<SrcT>().cbegin();
          auto lhsEnd = mLhs.map<SrcT>().cend();
          auto rhsIt = mRhs.map<DstT>().cbegin();

          auto lhsPal = mLhs.palette().get();
          auto rhsPal = mRhs.palette().get();

          for (; lhsIt != lhsEnd; ++lhsIt, ++rhsIt)
              if (lhsPal->color_unsafe<SrcPalT>(lhsIt->index) != rhsPal->color_unsafe<DstPalT>(rhsIt->index))
                  return false;

          return true;
      };
  };

  class ConvertTransform : public DefaultPixelTransform<> {
      const Image &mSrc;
      Image &mDst;

  public:
      ConvertTransform(const Image &src, Image &dst):
          mSrc(src),
          mDst(dst) {}

      template <class SrcT, class, class DstT, class>
      void color_to_color()
      {
          auto srcMap = mSrc.map<SrcT>();
          auto srcIt = srcMap.cbegin();
          auto srcEnd = srcMap.cend();
          auto dstIt = mDst.map<DstT>().begin();

          for (; srcIt != srcEnd; ++srcIt, ++dstIt)
              *dstIt = convert_pixel(*srcIt, pixel_traits<DstT>::tag());
      }

      template <class SrcT, class SrcPalT, class DstT, class>
      void index_to_color()
      {
          assert(mSrc.palette() && !mSrc.palette()->empty());

          auto srcMap = mSrc.map<SrcT>();
          auto srcIt = srcMap.cbegin();
          auto srcEnd = srcMap.cend();
          auto srcPal = mSrc.palette()->map<SrcPalT>().cbegin();
          auto dstIt = mDst.map<DstT>().begin();

          for (; srcIt != srcEnd; ++srcIt, ++dstIt)
              *dstIt = convert_pixel(*(srcPal + srcIt->index), pixel_traits<DstT>::tag());
      };
  };
}

Image::Image(PixelFormat format, uint16 width, uint16 height, byte *data):
    mTraits(&get_pixel_info(format)),
    mWidth(width),
    mHeight(height)
{
    assert(format != PixelFormat::none);
    assert(width > 0);
    assert(height > 0);

    auto size = calc_length(mTraits, mWidth, mHeight);
    mData = std::make_unique<byte[]>(size);

    if (data) {
        std::copy_n(data, size, mData.get());
    } else {
        std::fill_n(mData.get(), size, 0);
    }

    if (!mTraits->color)
        set_palette(Palette { PixelFormat::rgb, mTraits->pal_size, nullptr });
}

Image::Image(PixelFormat format, uint16 width, uint16 height, std::unique_ptr<byte[]> data):
        mTraits(&get_pixel_info(format)),
        mWidth(width),
        mHeight(height)
{
    assert(format != PixelFormat::none);
    assert(width > 0);
    assert(height > 0);

    if (data) {
        mData = std::move(data);
    } else {
        auto size = calc_length(mTraits, mWidth, mHeight);
        mData = std::make_unique<byte[]>(size);
        std::fill_n(mData.get(), size, 0);
    }

    if (!mTraits->color)
        set_palette(Palette { PixelFormat::rgb, mTraits->pal_size, nullptr });
}

Image::Image(PixelFormat format, uint16 width, uint16 height, noinit_tag):
    mTraits(&get_pixel_info(format)),
    mWidth(width),
    mHeight(height)
{
    assert(format != PixelFormat::none);
    assert(width > 0);
    assert(height > 0);

    auto length = calc_length(mTraits, mWidth, mHeight);
    mData = std::make_unique<byte[]>(length);
}

Image::Image(std::istream &stream):
    Image(find_format_by_reading(stream).load(stream)) {}

Image::Image(std::istream &stream, StringView mimetype):
    Image(find_format_by_mimetype(mimetype).load(stream)) {}

/* Delegate the work to some ImageFormatIO object */
void Image::load(std::istream &stream)
{ *this = find_format_by_reading(stream).load(stream); }

/* Delegate the work to some ImageFormatIO object */
void Image::load(std::istream &stream, StringView mimetype)
{ *this = find_format_by_mimetype(mimetype).load(stream); }

/* Delegate the work to some ImageFormatIO object */
void Image::save(std::ostream &s, StringView mimetype) const
{ find_format_by_mimetype(mimetype).save(s, *this); }

Image &Image::convert(PixelFormat format)
{
    if (mTraits->format == format)
        return *this;

    assert(format != PixelFormat::index8);

    Image copy(format, mWidth, mHeight, noinit_tag());
    copy.mOffsets = mOffsets;

    ConvertTransform ct(*this, copy);

    transform_pixel(this->format(), this->palette_format(),
                    copy.format(), copy.palette_format(), ct);

    return (*this = std::move(copy));
}

Image& Image::resize(uint16 width, uint16 height)
{
    if (mWidth == width && mHeight == height)
        return *this;

    Image copy(mTraits->format, width, height, noinit_tag());
    copy.mPalette = mPalette;
    copy.mOffsets = mOffsets;

    ResizeProcessor rp(*this, copy);

    process_pixel(format(), palette_format(), rp);

    return (*this = std::move(copy));
}

Image& Image::scale(uint16 width, uint16 height)
{
    if (mWidth == width && mHeight == height)
        return *this;

    Image copy(mTraits->format, width, height, noinit_tag());
    copy.mPalette = mPalette;
    copy.set_offsets(offsets());

    ScaleProcessor sp(*this, copy);

    process_pixel(format(), palette_format(), sp);

    return (*this = std::move(copy));
}

byte *Image::scanline_ptr(uint16 index)
{
    return data_ptr() + mWidth * mTraits->bytes * index;
}

const byte *Image::scanline_ptr(uint16 index) const
{
    return data_ptr() + mWidth * mTraits->bytes * index;
}

byte *Image::pixel_ptr(uint16 x, uint16 y)
{
    return data_ptr() + (mWidth * y + x) * mTraits->bytes;
}

const byte *Image::pixel_ptr(uint16 x, uint16 y) const
{
    return data_ptr() + (mWidth * y + x) * mTraits->bytes;
}

Image& Image::operator=(const Image &other)
{
    mTraits = other.mTraits;
    mWidth = other.mWidth;
    mHeight = other.mHeight;
    mPalette = other.mPalette;
    mOffsets = other.mOffsets;

    auto length = calc_length(mTraits, mWidth, mHeight);
    mData = std::make_unique<byte[]>(length);
    std::copy_n(other.mData.get(), length, mData.get());

    return *this;
}

bool kex::gfx::operator==(const Image &lhs, const Image &rhs)
{
    if (&lhs == &rhs)
        return true;

    if (lhs.width() != rhs.width() || lhs.height() != rhs.height())
        return false;

    CompareTransform ct(lhs, rhs);

    return transform_pixel<CompareTransform, bool>(lhs.format(), lhs.palette_format(),
                                 rhs.format(), rhs.palette_format(), ct);
}

bool kex::gfx::operator!=(const Image &lhs, const Image &rhs)
{ return !(lhs == rhs); }

std::unique_ptr<ImageFormatIO> __initialize_png();

void kex::lib::init_image()
{
    image_formats.emplace_back(__initialize_png());
}
