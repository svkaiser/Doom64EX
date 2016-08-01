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

#include <sstream>
#include <fstream>
#include <kex/gfx/Image>

using namespace kex::gfx;

extern "C" {

Image* Image_New(uint16_t width, uint16_t height, pixel_format format)
{ return new Image(width, height, format); }

Image* Image_New_FromData(const uint8_t *data, uint16_t width, uint16_t height, pixel_format format)
{
    Image *retval = new Image(make_image(data, format, width, height));
    return retval;
}

Image* Image_New_FromMemory(const char *data, size_t size)
{
    try {
        std::istringstream ss(std::string(data, size));
        return new Image(ss);
    } catch (image_error &e) {
        fmt::print("An exception occured when loading image: {}\n", e.what());
        return nullptr;
    }
}

Image* Image_Resize(Image *src, uint16_t new_width, uint16_t new_height)
{
    return new Image(src->resize(new_width, new_height));
}

void Image_Free(Image* ptr)
{
    assert(ptr);
    delete ptr;
}

auto Image_GetWidth(Image *image)
{ return image->width(); }

auto Image_GetHeight(Image *image)
{ return image->height(); }

auto Image_GetData(Image *image)
{ return image->data_ptr(); }

auto Image_GetPalette(Image *image)
{ return &image->palette(); }

auto Image_GetOffsets(Image *image)
{ return image->offsets(); }

auto Image_IsIndexed(Image *image)
{ return image->is_indexed(); }

void Image_Convert(Image *image, pixel_format format)
{ image->convert(format); }

void Image_Scale(Image *image, uint16_t new_width, uint16_t new_height)
{ image->scale(new_width, new_height); }

auto Palette_GetData(Palette *pal)
{ return pal->colors_ptr(); }

int Palette_GetCount(Palette *pal)
{ return pal->size(); }

int Palette_HasAlpha(Palette *pal)
{ return pal->traits().alpha; }

} // extern "C"
