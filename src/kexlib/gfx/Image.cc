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
  std::vector<std::unique_ptr<image_type>> image_types;

  const image_type *get_mimetype(const char *mimetype)
  {
      for (auto&& x : image_types)
          if (std::strcmp(x->mimetype(), mimetype) == 0)
              return x.get();

      return nullptr;
  }

  const image_type *detect(std::istream &s)
  {
      auto pos = s.tellg();

      for (auto&& type : image_types)
      {
          if (type->detect(s))
          {
              s.seekg(pos);
              return type.get();
          }
          s.seekg(pos);
      }

      throw image_load_error("Failed to detect image type");
  }

  size_t calc_length(const pixel_traits *traits, uint16_t width, uint16_t height)
  {
      if (traits->format == pixel_format::none)
          throw bad_pixel_format();

      return traits->bytes * width * height / traits->pack;
  }

  auto alloc_pixels(const pixel_traits *traits, uint16_t width, uint16_t height)
  { return std::make_unique<uint8_t[]>(calc_length(traits, width, height)); }

  template <class SrcT, class DstT>
  void tconvert2(const Image &src, Image &dst)
  {
      auto srcMap = src.map<SrcT>();
      auto srcIt = srcMap.cbegin();
      auto srcEnd = srcMap.cend();
      auto dstIt = dst.map<DstT>().begin();

      for (; srcIt != srcEnd; ++srcIt, ++dstIt)
          copy_pixel(*srcIt, *dstIt);
  };

  template <class SrcT, class DstT>
  void tconvertpal2(const Image &src, Image &dst)
  {
      auto srcMap = src.map<Index8>();
      auto srcIt = srcMap.cbegin();
      auto srcEnd = srcMap.cend();
      auto srcPal = src.palette().map<SrcT>().cbegin();
      auto dstIt = dst.map<DstT>().begin();

      for (; srcIt != srcEnd; ++srcIt, ++dstIt)
          copy_pixel(*(srcPal + srcIt->index), *dstIt);
  };

  template <class DstT>
  void tconvertpal(const Image &src, Image &dst)
  {
      switch (src.palette().format())
      {
      case pixel_format::rgb:
          tconvertpal2<Rgb, DstT>(src, dst);
          break;

      case pixel_format::bgr:
          tconvertpal2<Bgr, DstT>(src, dst);
          break;

      case pixel_format::rgba:
          tconvertpal2<Rgba, DstT>(src, dst);
          break;

      case pixel_format::bgra:
          tconvertpal2<Bgra, DstT>(src, dst);
          break;

      default:
          throw bad_pixel_format();
      }
  };

  /*!
   * \brief Templated image conversion. (the first 't' stands for 'template')
   * \param src The source image
   * \param dst The destination image
   */
  template <class DstT>
  void tconvert(const Image &src, Image &dst)
  {
      switch (src.format())
      {
      case pixel_format::index8:
          tconvertpal<DstT>(src, dst);
          break;

      case pixel_format::rgb:
          tconvert2<Rgb, DstT>(src, dst);
          break;

      case pixel_format::bgr:
          tconvert2<Bgr, DstT>(src, dst);
          break;

      case pixel_format::rgba:
          tconvert2<Rgba, DstT>(src, dst);
          break;

      case pixel_format::bgra:
          tconvert2<Bgra, DstT>(src, dst);
          break;

      default:
          throw bad_pixel_format();
      }
  }
}

Image kex::gfx::make_image(const uint8_t data[], pixel_format format, uint16_t width, uint16_t height)
{
    Image retval(width, height, format);

    auto length = calc_length(&retval.traits(), width, height);
    std::copy_n(data, length, retval.data_ptr());

    return retval;
}

Image::Image(const Image &other):
        mTraits(other.mTraits),
        mWidth(other.mWidth),
        mHeight(other.mHeight),
        mPalette(other.mPalette),
        mData(std::make_unique<uint8_t[]>(calc_length(mTraits, mWidth, mHeight)))
{
    std::copy_n(other.mData.get(), calc_length(mTraits, mWidth, mHeight), mData.get());
}

Image::Image(uint16_t width, uint16_t height, pixel_format format):
        mTraits(&get_pixel_traits(format)),
        mWidth(width),
        mHeight(height),
        mPalette(make_palette<Rgb>(format)),
        mData(alloc_pixels(mTraits, width, height)) {}

Image::Image(std::istream &s):
        Image(detect(s)->load(s)) {}

Image::Image(std::istream &s, const char *format):
        Image(get_mimetype(format)->load(s)) {}

void Image::load(std::istream &s)
{
    *this = detect(s)->load(s);
}

void Image::load(std::istream &s, const char *mimetype)
{
    *this = get_mimetype(mimetype)->load(s);
}

void Image::save(std::ostream &s, const char *mimetype) const
{
    auto type = get_mimetype(mimetype);
    type->save(s, *this);
}

void Image::convert(pixel_format format)
{
    if (mTraits->format == format)
        return;

    *this = convert_copy(format);
}

Image Image::convert_copy(pixel_format format) const
{
    if (mTraits->format == format)
        return *this;

    Image newImage(mWidth, mHeight, format);
    switch (format) {
        case pixel_format::rgb:
            tconvert<Rgb>(*this, newImage);
            break;

        case pixel_format::bgr:
            tconvert<Bgr>(*this, newImage);
            break;

        case pixel_format::rgba:
            tconvert<Rgba>(*this, newImage);
            break;

        case pixel_format::bgra:
            tconvert<Bgra>(*this, newImage);
            break;

        default:
            throw bad_pixel_format();
    }

    return newImage;
}

Image Image::crop_fill(uint16_t width, uint16_t height) const
{
    Image copy { width, height, mTraits->format };
    copy.palette() = palette();

    return copy;
}

uint8_t *Image::scanline_ptr(uint16_t index)
{
    return data_ptr() + mWidth * mTraits->bytes * index / mTraits->pack;
}

const uint8_t *Image::scanline_ptr(uint16_t index) const
{
    return data_ptr() + mWidth * mTraits->bytes * index / mTraits->pack;
}

Image& Image::operator=(const Image &other)
{
    auto length = calc_length(mTraits, mWidth, mHeight);
    mTraits = other.mTraits;
    mWidth = other.mWidth;
    mHeight = other.mHeight;
    mPalette = other.mPalette;
    mData = std::make_unique<uint8_t[]>(length);
    std::copy_n(other.mData.get(), length, mData.get());
    return *this;
}

std::unique_ptr<image_type> __initialize_png();

void kex::lib::init_image()
{
    image_types.emplace_back(__initialize_png());
}
