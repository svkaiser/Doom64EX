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

#include <cstring>
#include <png.h>
#include <utility/endian.hh>

#include "Image.hh"

namespace {
  constexpr const char magic[] = "\x89PNG\r\n\x1a\n";

  struct Png : ImageFormatIO {
      Optional<Image> load(std::istream& s) const override;
      void save(std::ostream&, const Image&) const override;
  };

  Optional<Image> Png::load(std::istream& s) const
  {
      png_structp png_ptr = nullptr;
      png_infop infop = nullptr;
      auto _guard = make_guard([&png_ptr, &infop]()
                               {
                                   png_structpp png_ptrp = png_ptr ? &png_ptr : nullptr;
                                   png_infopp infopp = infop ? &infop : nullptr;
                                   png_destroy_read_struct(png_ptrp, infopp, nullptr);
                               });

      // Check if the starts with a PNG magic
      {
          auto start = s.tellg();
          char test[sizeof(magic)];
          s.read(test, sizeof(test));
          if (!std::equal(test, test + sizeof(test), magic))
              return nullopt;
          s.seekg(start);
      }

      png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
      if (png_ptr == nullptr)
          throw std::runtime_error { "Failed to create PNG read struct" };

      infop = png_create_info_struct(png_ptr);
      if (infop == nullptr)
      {
          png_destroy_read_struct(&png_ptr, nullptr, nullptr);
          throw std::runtime_error { "Failed to create PNG info struct" };
      }

      if (setjmp(png_jmpbuf(png_ptr)))
      {
          png_destroy_read_struct(&png_ptr, &infop, nullptr);
          throw std::runtime_error { "An error occurred in libpng" };
      }

      png_set_read_fn(png_ptr, &s,
                      [](png_structp ctx, png_bytep area, png_size_t size) {
                          auto st = static_cast<std::istream *>(png_get_io_ptr(ctx));
                          st->read(reinterpret_cast<char *>(area), size);
                      });

      /* Grab offset information if available. This seems like a hack since this is
       * probably only used by Doom64EX and not a general thing as this file might imply. */
      SpriteOffset offset;
      auto chunk_fn = [](png_structp p, png_unknown_chunkp chunk) -> int {
          if (std::strncmp((char*)chunk->name, "grAb", 4) == 0 && chunk->size >= 8) {
              auto so = static_cast<SpriteOffset*>(png_get_user_chunk_ptr(p));
              auto data = reinterpret_cast<int*>(chunk->data);
              so->x = big_endian(data[0]);
              so->y = big_endian(data[1]);
              return 1;
          }

          return 0;
      };
      png_set_read_user_chunk_fn(png_ptr, &offset, chunk_fn);

      png_uint_32 width, height;
      int bitDepth, colorType, interlaceMethod;
      png_read_info(png_ptr, infop);
      png_get_IHDR(png_ptr, infop, &width, &height, &bitDepth, &colorType, &interlaceMethod, nullptr, nullptr);

      png_uint_32 sizeLimit = std::numeric_limits<uint16>::max();
      if (width < 1 || width > sizeLimit || height < 1 || height > sizeLimit)
          throw std::runtime_error { fmt::format("Invalid image dimensions: {}x{}", width, height) };

      png_set_strip_16(png_ptr);
      png_set_packing(png_ptr);
      png_set_interlace_handling(png_ptr);

      if (colorType == PNG_COLOR_TYPE_GRAY)
          png_set_expand(png_ptr);

      if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
          png_set_gray_to_rgb(png_ptr);

      png_read_update_info(png_ptr, infop);
      png_get_IHDR(png_ptr, infop, &width, &height, &bitDepth, &colorType, &interlaceMethod, nullptr, nullptr);

      PixelFormat format;
      switch (colorType) {
          case PNG_COLOR_TYPE_RGB:
              format = PixelFormat::rgb;
              break;

          case PNG_COLOR_TYPE_RGB_ALPHA:
              format = PixelFormat::rgba;
              break;

          case PNG_COLOR_TYPE_PALETTE:
              switch (bitDepth) {
              case 8:
                  format = PixelFormat::index8;
                  break;

              default:
                  throw std::runtime_error { fmt::format("Invalid PNG bit depth: {}", bitDepth) };
              }
              break;

          default:
              throw std::runtime_error { fmt::format("Unknown PNG image color type: {}", colorType) };
      }

      Image retval { format, static_cast<uint16>(width), static_cast<uint16>(height) };

      if (colorType == PNG_COLOR_TYPE_PALETTE)
      {
          if (png_get_valid(png_ptr, infop, PNG_INFO_tRNS))
          {
              png_bytep alpha;
              png_colorp colors;
              int palNum, transNum;
              png_get_tRNS(png_ptr, infop, &alpha, &transNum, nullptr);
              png_get_PLTE(png_ptr, infop, &colors, &palNum);
              RgbaPalette palette(palNum);

              int i = 0;
              for (auto &c : palette)
              {
                  c.red   = colors[i].red;
                  c.green = colors[i].green;
                  c.blue  = colors[i].blue;
                  c.alpha = i < transNum ? alpha[i] : 0xff;

                  i++;
              }

              retval.set_palette(std::move(palette));
          } else {
              png_colorp pal = nullptr;
              int num_colors = 0;
              png_get_PLTE(png_ptr, infop, &pal, &num_colors);
              RgbPalette palette { static_cast<size_t>(num_colors) };

              for (auto &c : palette)
              {
                  auto p = *pal++;
                  c.red = p.red;
                  c.green = p.green;
                  c.blue = p.blue;
              }

              retval.set_palette(std::move(palette));
          }
      }

      retval.sprite_offset(offset);

	  auto scanlines = std::make_unique<byte*[]>(height);
      for (size_t i = 0; i < height; i++)
          scanlines[i] = reinterpret_cast<byte*>(retval[i].data_ptr());

      Image x = retval;

      png_read_image(png_ptr, scanlines.get());
      png_read_end(png_ptr, infop);

      return make_optional<Image>(retval);
  }

  void Png::save(std::ostream &s, const Image &image) const
  {
      png_structp writep = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
      if (writep == nullptr)
          throw std::runtime_error { "Failed getting png_structp" };

      png_infop infop = png_create_info_struct(writep);
      if (infop == nullptr)
      {
          png_destroy_write_struct(&writep, nullptr);
          throw std::runtime_error { "Failed getting png_infop"};
      }

      if (setjmp(png_jmpbuf(writep)))
      {
          png_destroy_write_struct(&writep, &infop);
          throw std::runtime_error { "Error occurred in libpng"};
      }

      png_set_write_fn(writep, &s,
                       [](png_structp ctx, png_bytep data, png_size_t length) {
                           static_cast<std::ostream*>(png_get_io_ptr(ctx))->write((char*)data, length);
                       },
                       [](png_structp ctx) {
                           static_cast<std::ostream*>(png_get_io_ptr(ctx))->flush();
                       });

      Image copy;

      int format;
      switch (image.pixel_format()) {
          case PixelFormat::rgb:
              format = PNG_COLOR_TYPE_RGB;
              break;

          case PixelFormat::rgba:
              format = PNG_COLOR_TYPE_RGB_ALPHA;
              break;

          default:
              throw std::runtime_error { "Saving image with incompatible pixel format" };
      }

      png_set_IHDR(writep, infop, image.width(), image.height(), 8,
                   format, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);
      png_write_info(writep, infop);

      auto scanlines = std::make_unique<png_bytep[]>(image.height());
      for (int i = 0; i < image.height(); i++) {
          scanlines[i] = new png_byte[image.pitch()];
          std::copy_n(reinterpret_cast<const byte*>(image[i].data_ptr()), image.pitch(), scanlines[i]);
      }

      png_write_image(writep, scanlines.get());
      png_write_end(writep, infop);
      png_destroy_write_struct(&writep, &infop);
  }
}

std::unique_ptr<ImageFormatIO> init::image_png()
{
    return std::make_unique<Png>();
}
