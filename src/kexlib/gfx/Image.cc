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
  void convert_indexed_to_normal(const Image &src, Image &dst)
  {
      auto srcMap = src.map<Index8>();
      auto srcIt = srcMap.cbegin();
      auto srcEnd = srcMap.cend();
      auto srcPal = src.palette().map<SrcT>().cbegin();
      auto dstIt = dst.map<DstT>().begin();

      for (; srcIt != srcEnd; ++srcIt, ++dstIt)
          copy_pixel(*(srcPal + srcIt->index), *dstIt);
  };

  template <class Src, class Dst>
  void convert_normal_to_normal(const Image &src, Image &dst)
  {
      auto srcMap = src.map<Src>();
      auto srcIt = srcMap.cbegin();
      auto srcEnd = srcMap.cend();
      auto dstIt = dst.map<Dst>().begin();

      for (; srcIt != srcEnd; ++srcIt, ++dstIt)
          copy_pixel(*srcIt, *dstIt);
  }

  template <class Src>
  struct convert_normal_to_unknown : default_pixel_processor {
      template <class Dst>
      void normal(const Image &src, Image &dst)
      {
          convert_normal_to_normal<Src, Dst>(src, dst);
      }
  };

  template <class PalSrc>
  struct convert_indexed_to_unknown : default_pixel_processor {
      template <class Dst>
      void normal(const Image &src, Image &dst)
      {
          convert_indexed_to_normal<PalSrc, Dst>(src, dst);
      }
  };

  struct convert_unknown_indexed_to_unknown : default_pixel_processor {
      template <class Src>
      void normal(const Image &src, Image &dst)
      {
          process_pixel(convert_indexed_to_unknown<Src>(), dst.format(), src, dst);
      }
  };

  struct convert_unknown_to_unknown {
      template <class Src>
      void normal(const Image &src, Image &dst)
      {
          process_pixel(convert_normal_to_unknown<Src>(), dst.format(), src, dst);
      }

      template <class Src>
      void indexed(const Image &src, Image &dst)
      {
          process_pixel(convert_unknown_indexed_to_unknown(), src.palette().format(), src, dst);
      }
  };

  void tconvert(const Image &src, Image &dst)
  {
      process_pixel(convert_unknown_to_unknown(), src.format(), src, dst);
  }

  template <class Src, class Dst>
  void resize_normal_to_normal(const Image &src, Image &dst)
  {
      int h = std::min(src.height(), dst.height());
      int w = std::min(src.width(), dst.width());
      auto srcLinePad = (src.width() < dst.width()) ? 0 : src.width() - w;
      auto dstLinePad = (src.width() < dst.width()) ? dst.width() - w : 0;

      auto srcIt = src.map<Src>().cbegin();
      auto dstIt = dst.map<Dst>().begin();

      for (int y = 0; y < h; y++)
      {
          for (int x = 0; x < w; x++)
          {
              copy_pixel(*srcIt, *dstIt);
              ++srcIt, ++dstIt;
          }

          srcIt += srcLinePad;
          dstIt += dstLinePad;
      }
  };

  template <class Src>
  struct resize_normal_to_unknown : default_pixel_processor {
      template <class Dst>
      void normal(const Image &src, Image &dst)
      {
          resize_normal_to_normal<Src, Dst>(src, dst);
      }
  };

  struct resize_unknown_to_unknown : default_pixel_processor {
      template <class Src>
      void normal(const Image &src, Image &dst)
      {
          process_pixel(resize_normal_to_unknown<Src>(), dst.format(), src, dst);
      }
  };

  void tresize(const Image &src, Image &dst)
  {
      process_pixel(resize_unknown_to_unknown(), src.format(), src, dst);
  }

  template <class Src, class Dst>
  void scale_normal_to_normal(const Image &src, Image &dst)
  {
      auto srcWidth = src.width();
      auto width = dst.width();
      auto height = dst.height();
      auto dx = static_cast<double>(src.width()) / width;
      auto dy = static_cast<double>(src.height()) / height;

      auto srcIt = src.map<Src>().cbegin();
      auto dstIt = dst.map<Dst>().begin();

      for (int y = 0; y < height; y++)
      {
          for (int x = 0; x < width; x++)
          {
              size_t pos = static_cast<size_t>(dy * y) * srcWidth + static_cast<size_t>(dx * x);
              copy_pixel(*(srcIt + pos), *dstIt);
              ++dstIt;
          }
      }
  }

  template <class Src>
  struct scale_normal_to_unknown : default_pixel_processor {
      template <class Dst>
      void normal(const Image &src, Image &dst)
      {
          scale_normal_to_normal<Src, Dst>(src, dst);
      }
  };

  struct scale_unknown_to_unknown : default_pixel_processor {
      template <class Src>
      void normal(const Image &src, Image &dst)
      {
          process_pixel(scale_normal_to_unknown<Src>(), dst.format(), src, dst);
      }
  };

  void tscale(const Image &src, Image &dst)
  {
      process_pixel(scale_unknown_to_unknown(), src.format(), src, dst);
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
    offsets(other.offsets());
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

Image& Image::convert(pixel_format format)
{
    if (mTraits->format == format)
        return *this;

    Image copy { mWidth, mHeight, format };
    copy.palette() = palette();
    copy.offsets(offsets());

    tconvert(*this, copy);

    return (*this = std::move(copy));
}

Image& Image::resize(uint16_t width, uint16_t height)
{
    if (mWidth == width && mHeight == height)
        return *this;

    Image copy { width, height, mTraits->format };
    copy.palette() = palette();
    copy.offsets(offsets());

    tresize(*this, copy);

    return (*this = std::move(copy));
}

Image& Image::scale(uint16_t width, uint16_t height)
{
    if (mWidth == width && mHeight == height)
        return *this;

    Image copy { width, height, mTraits->format };
    copy.palette() = palette();
    copy.offsets(offsets());

    tscale(*this, copy);

    return (*this = std::move(copy));
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
    mTraits = other.mTraits;
    mWidth = other.mWidth;
    mHeight = other.mHeight;
    mPalette = other.mPalette;

    auto length = calc_length(mTraits, mWidth, mHeight);
    mData = std::make_unique<uint8_t[]>(length);
    std::copy_n(other.mData.get(), length, mData.get());
    offsets(other.offsets());
    return *this;
}

std::unique_ptr<image_type> __initialize_png();

void kex::lib::init_image()
{
    image_types.emplace_back(__initialize_png());
}
