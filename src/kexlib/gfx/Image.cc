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

  template <class Src, class Dst>
  void convert_indexed_to_normal(const Image &src, Image &dst)
  {
      assert(src.palette() && !src.palette()->empty());

      auto srcMap = src.map<Index8>();
      auto srcIt = srcMap.cbegin();
      auto srcEnd = srcMap.cend();
      auto srcPal = src.palette()->map<Src>().cbegin();
      auto dstIt = dst.map<Dst>().begin();

      for (; srcIt != srcEnd; ++srcIt, ++dstIt)
          *dstIt = convert_pixel(*(srcPal + srcIt->index), pixel_traits<Dst>::tag());
  };

  template <class Src, class Dst>
  void convert_normal_to_normal(const Image &src, Image &dst)
  {
      auto srcMap = src.map<Src>();
      auto srcIt = srcMap.cbegin();
      auto srcEnd = srcMap.cend();
      auto dstIt = dst.map<Dst>().begin();

      for (; srcIt != srcEnd; ++srcIt, ++dstIt)
          *dstIt = convert_pixel(*srcIt, pixel_traits<Dst>::tag());
  }

  template <class Src>
  struct convert_normal_to_unknown : default_pixel_processor {
      template <class Dst>
      void color(const Image &src, Image &dst)
      {
          convert_normal_to_normal<Src, Dst>(src, dst);
      }
  };

  template <class PalSrc>
  struct convert_indexed_to_unknown : default_pixel_processor {
      template <class Dst>
      void color(const Image &src, Image &dst)
      {
          convert_indexed_to_normal<PalSrc, Dst>(src, dst);
      }
  };

  struct convert_unknown_indexed_to_unknown : default_pixel_processor {
      template <class Src>
      void color(const Image &src, Image &dst)
      {
          process_pixel(convert_indexed_to_unknown<Src>(), dst.format(), src, dst);
      }
  };

  struct convert_unknown_to_unknown {
      template <class Src>
      void color(const Image &src, Image &dst)
      {
          process_pixel(convert_normal_to_unknown<Src>(), dst.format(), src, dst);
      }

      template <class Src>
      void index(const Image &src, Image &dst)
      {
          process_pixel(convert_unknown_indexed_to_unknown(), src.palette()->format(), src, dst);
      }
  };

  void tconvert(const Image &src, Image &dst)
  {
      process_pixel(convert_unknown_to_unknown(), src.format(), src, dst);
  }

  struct resize_processor : default_pixel_processor {
      template <class T>
      void color(const Image &src, Image &dst)
      {
          int x, y;
          int h = std::min(src.height(), dst.height());
          int w = std::min(src.width(), dst.width());

          auto srcIt = src.map<T>().cbegin();
          auto dstIt = dst.map<T>().begin();

          for (y = 0; y < h; y++)
          {
              for (x = 0; x < w; x++)
                  *dstIt++ = convert_pixel(*srcIt++, pixel_traits<T>::tag());

              for (; x < dst.width(); x++)
                  *dstIt++ = { };
          }

          for (; y < dst.height(); y++)
              for (x = 0; x < dst.width(); x++)
                  *dstIt++ = { };
      }
  };

  struct scale_processor : default_pixel_processor {
      template <class T>
      void color(const Image &src, Image &dst)
      {
          auto srcWidth = src.width();
          auto width = dst.width();
          auto height = dst.height();
          auto dx = static_cast<double>(src.width()) / width;
          auto dy = static_cast<double>(src.height()) / height;

          auto srcIt = src.map<T>().cbegin();
          auto dstIt = dst.map<T>().begin();

          for (int y = 0; y < height; y++)
          {
              for (int x = 0; x < width; x++)
              {
                  size_t pos = static_cast<size_t>(dy * y) * srcWidth + static_cast<size_t>(dx * x);
                  *dstIt++ = convert_pixel(*(srcIt++ + pos), pixel_traits<T>::tag());
              }
          }
      }
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

    Image copy(format, mWidth, mHeight, noinit_tag());
    copy.mPalette = mPalette;
    copy.mOffsets = mOffsets;
    tconvert(*this, copy);

    return (*this = std::move(copy));
}

Image& Image::resize(uint16 width, uint16 height)
{
    if (mWidth == width && mHeight == height)
        return *this;

    Image copy(mTraits->format, width, height, noinit_tag());
    copy.mPalette = mPalette;
    copy.mOffsets = mOffsets;

    process_pixel(resize_processor(), format(), *this, copy);

    return (*this = std::move(copy));
}

Image& Image::scale(uint16 width, uint16 height)
{
    if (mWidth == width && mHeight == height)
        return *this;

    Image copy(mTraits->format, width, height, noinit_tag());
    copy.mPalette = mPalette;
    copy.set_offsets(offsets());

    process_pixel(scale_processor(), format(), *this, copy);

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

std::unique_ptr<ImageFormatIO> __initialize_png();

void kex::lib::init_image()
{
    image_formats.emplace_back(__initialize_png());
}
