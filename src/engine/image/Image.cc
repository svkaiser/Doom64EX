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

#include "Image.hh"

namespace {
  UniquePtr<ImageFormatIO> image_formats_[num_image_formats] {};
  ImageFormat auto_order_[] {
      ImageFormat::png,
      ImageFormat::doom
  };

  void init_image_formats_()
  {
      auto& i = image_formats_;
      i[0] = init::image_png();
      i[1] = init::image_doom();
      i[2] = init::image_n64gfx();
      i[3] = init::image_n64texture();
      i[4] = init::image_n64sprite();
  }
}

void Image::load(std::istream &s)
{
    auto pos = s.tellg();
    for (auto fmt : auto_order_) {
        auto io = image_formats_[static_cast<int>(fmt)].get();
        auto opt = io->load(s);
        if (opt) {
            *this = std::move(*opt);
            return;
        } else s.seekg(pos);
    }

    throw std::runtime_error { "Couldn't detect image type" };
}

void Image::load(std::istream &s, ImageFormat format)
{
    auto io = image_formats_[static_cast<int>(format)].get();
    auto opt = io->load(s);
    if (opt) {
        *this = std::move(*opt);
        return;
    }

    throw std::runtime_error { "Couldn't load image" };
}

void Image::save(std::ostream &s, ImageFormat format) const
{
    auto io = image_formats_[static_cast<int>(format)].get();
    io->save(s, *this);
}

void Image::convert(PixelFormat format)
{
    if (pixel_format() == format)
        return;

    match_color(format,
    [this](auto color)
    {
        BasicImage<decltype(color)> copy { this->width(), this->height() };

        if (this->width() <= 1) {
            auto s = this->sprite_offset();
            *this = std::move(copy);
            this->sprite_offset(s);
            return;
        }

        this->match(
            [&copy](const auto& image)
            {
                for (uint16 y = 0; y < image.height(); ++y) {
                    auto from_sc = image[y];
                    auto to_sc = copy[y];
                    for (uint16 x = 0; x < image.width(); ++x) {
                        to_sc[x] = from_sc[x];
                    }
                }
            });

        auto s = this->sprite_offset();
        *this = std::move(copy);
        this->sprite_offset(s);
    });
}

void init_image()
{
    init_image_formats_();
}
