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

#include "image.hh"

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
  }
}

void Image::load(std::istream& s)
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

void Image::load(std::istream& s, ImageFormat format)
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

namespace {
}

void Image::scale(size_t new_width, size_t new_height)
{
    if (this->width() == new_width && this->height() == new_height)
        return;

    if (this->width() == 0 || this->height() == 0) {
        // If the new size is also 0 pixels
        if (new_width == 0 || new_height == 0) {
            return;
        }

        *this = Image(pixel_format(), new_width, new_height);
        return;
    }

    auto image = this->view_as<Rgb>();
    RgbImage copy(new_width, new_height);

    double sy = 0;
    double dx = static_cast<double>(image.width()) / new_width;
    double dy = static_cast<double>(image.height()) / new_height;

    for (size_t y = 0; y < new_height; ++y, sy += dy) {
        auto zy = static_cast<size_t>(sy);
        double sx = 0;
        for (size_t x = 0; x < new_width; ++x, sx += dx) {
            auto zx = static_cast<size_t>(sx);
            double box_r {};
            double box_g {};
            double box_b {};
            size_t box_siz {};
            for (size_t box_y {}; box_y < static_cast<size_t>(dy); ++box_y) {
                for (size_t box_x {}; box_x < static_cast<size_t>(dx); ++box_x) {
                    auto c = image[static_cast<size_t>(sy + box_y)].get(static_cast<size_t>(sx + box_x));
                    box_r += c.red;
                    box_g += c.green;
                    box_b += c.blue;
                    box_siz++;
                }
            }
            copy[y].get(x) = Rgb(box_r / box_siz, box_g / box_siz, box_b / box_siz);
        }
    }

    *this = copy;
}

void Image::canvas(size_t new_width, size_t new_height)
{
    Image copy(pixel_format(), new_width, new_height);

    auto copyx = std::min(width(), copy.width()) * pixel_info().width;
    auto copyy = std::min(height(), copy.height());

    for (size_t y = 0; y < copyy; ++y) {
        std::copy_n((*this)[y].data_ptr(), copyx, copy[y].data_ptr());
    }

    *this = copy;
}

void Image::flip_vertical()
{
    auto half_height = height() / 2;

    for (size_t y = 0; y < half_height; ++y) {
        auto top = (*this)[y].data_ptr();
        auto bot = (*this)[height() - y - 1].data_ptr();

        std::swap_ranges(top, top + pitch(), bot);
    }
}

void init_image()
{
    init_image_formats_();
}
