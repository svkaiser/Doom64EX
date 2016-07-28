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

#include <kex/gfx/Image>
#include <png.h>
#include <stdlib.h>

using namespace kex::gfx;

namespace {
  struct PngImage : image_type {
      const char *mimetype() const override;
      bool detect(std::istream &) const override;
      Image load(std::istream &) const override;
      void save(std::ostream &, const Image &) const override;
  };

  const char *PngImage::mimetype() const
  { return "png"; }

  bool PngImage::detect(std::istream &s) const
  {
      static auto magic = "\x89PNG\r\n\x1a\n";
      char buf[sizeof(magic) + 1] = {};

      s.read(buf, sizeof magic);
      return std::strncmp(magic, buf, sizeof magic) == 0;
  }

  Image PngImage::load(std::istream &s) const
  {
      png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
      if (png_ptr == nullptr)
          throw image_load_error("Failed to create PNG read struct");

      png_infop infop = png_create_info_struct(png_ptr);
      if (infop == nullptr)
      {
          png_destroy_read_struct(&png_ptr, nullptr, nullptr);
          throw image_load_error("Failed to create PNG info struct");
      }

      if (setjmp(png_jmpbuf(png_ptr)))
      {
          png_destroy_read_struct(&png_ptr, &infop, nullptr);
          throw image_load_error("An error occurred in libpng");
      }

      png_set_read_fn(png_ptr, &s,
                      [](auto ctx, auto area, auto size) {
                          auto s = static_cast<std::istream *>(png_get_io_ptr(ctx));
                          s->read(reinterpret_cast<char *>(area), size);
                      });

      // TODO: Add unknown chunk loading (should be simple enough for a handsome guy like you)

      png_uint_32 width, height;
      int bitDepth, colorType, interlaceMethod;
      png_read_info(png_ptr, infop);
      png_get_IHDR(png_ptr, infop, &width, &height, &bitDepth, &colorType, &interlaceMethod, nullptr, nullptr);

      png_set_strip_16(png_ptr);
      png_set_packing(png_ptr);

      if (colorType == PNG_COLOR_TYPE_GRAY)
          png_set_expand(png_ptr);

      // TODO: Check png_get_tRNS for transparency

      if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_gray_to_rgb(png_ptr);

      png_read_update_info(png_ptr, infop);
      png_get_IHDR(png_ptr, infop, &width, &height, &bitDepth, &colorType, &interlaceMethod, nullptr, nullptr);

      fmt::print(">>> bitDepth: {}, colorType: {}\n", bitDepth, colorType);

      pixel_format format;
      switch (colorType) {
          case PNG_COLOR_TYPE_RGB:
              format = pixel_format::bgr;
              break;

          case PNG_COLOR_TYPE_RGB_ALPHA:
              format = pixel_format::bgra;
              break;

          case PNG_COLOR_TYPE_PALETTE:
              switch (bitDepth) {
              case 8:
                  format = pixel_format::index8;
                  break;

              default:
                  throw image_load_error(fmt::format("Invalid PNG bit depth: {}", bitDepth));
              }
              break;

          default:
              throw image_load_error(fmt::format("Unknown PNG image color type: {}", colorType));
      }

      Image retval(width, height, format);
      png_bytep scanlines[height];
      for (size_t i = 0; i < height; i++)
          scanlines[i] = retval.scanline_ptr(i);

      if (colorType == PNG_COLOR_TYPE_PALETTE)
      {
          png_colorp pal = nullptr;
          int numPal = 0;
          png_get_PLTE(png_ptr, infop, &pal, &numPal);

          for (auto&& c : retval.palette().map<Rgb>() )
          {
              auto p = *pal++;
              c.red = p.red;
              c.green = p.green;
              c.blue = p.blue;
          }
      }

      png_read_image(png_ptr, scanlines);
      png_read_end(png_ptr, infop);

      return retval;
  }

  void PngImage::save(std::ostream &s, const Image &image) const
  {
      png_structp writep = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
      if (writep == nullptr)
          throw image_save_error("Failed getting png_structp");

      png_infop infop = png_create_info_struct(writep);
      if (infop == nullptr)
      {
          png_destroy_write_struct(&writep, nullptr);
          throw image_save_error("Failed getting png_infop");
      }

      if (setjmp(png_jmpbuf(writep)))
      {
          png_destroy_write_struct(&writep, &infop);
          throw image_save_error("Error occurred in libpng");
      }

      png_set_write_fn(writep, &s,
                       [](auto ctx, auto data, auto length) {
                           static_cast<std::ostream*>(png_get_io_ptr(ctx))->write((char*)data, length);
                       },
                       [](auto ctx) {
                           static_cast<std::ostream*>(png_get_io_ptr(ctx))->flush();
                       });

      Image copy;
      const Image *im = &image;

      int format;
      switch (image.format()) {
          case pixel_format::rgb:
              copy = image.convert_copy(pixel_format::bgr);
              im = &copy;
               [[fallthrough]];
          case pixel_format::bgr:
              format = PNG_COLOR_TYPE_RGB;
              break;

          case pixel_format::rgba:
              copy = image.convert_copy(pixel_format::bgra);
              im = &copy;
               [[fallthrough]];
          case pixel_format::bgra:
              format = PNG_COLOR_TYPE_RGB_ALPHA;
              break;

          default:
              throw image_save_error("Saving image with incompatible pixel format");
      }

      png_set_IHDR(writep, infop, im->width(), im->height(), 8,
                   format, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);
      png_write_info(writep, infop);

      const uint8_t *scanlines[im->height()];
      for (int i = 0; i < im->height(); i++)
          scanlines[i] = im->scanline_ptr(i);

      png_write_image(writep, const_cast<png_bytepp>(scanlines));
      png_write_end(writep, infop);
      png_destroy_write_struct(&writep, &infop);
  }
}

std::unique_ptr<image_type> __initialize_png()
{
    return std::make_unique<PngImage>();
}