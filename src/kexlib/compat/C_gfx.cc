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

Image* Image_New(PixelFormat format, uint16 width, uint16 height)
{ return new Image(format, width, height, nullptr); }

Image* Image_New_FromData(PixelFormat format, uint16 width, uint16 height, byte *data)
{
    Image *retval = new Image(format, width, height, data);
    return retval;
}

Image* Image_New_FromMemory(const char *data, size_t size)
{
    try {
        std::istringstream ss(std::string(data, size));
        return new Image(ss);
    } catch (ImageError &e) {
        fmt::print("An exception occured when loading image: {}\n", e.what());
        return nullptr;
    }
}

int Image_Save(Image *image, StringView filename, StringView format)
{
    try {
        std::ofstream f(filename.data());
        if (!f.is_open())
            return -1;

        image->save(f, format);
    } catch (...) {
        return -1;
    }

    return 0;
}

void Image_Resize(Image *src, uint16_t new_width, uint16_t new_height)
{
    src->resize(new_width, new_height);
}

void Image_Free(Image* ptr)
{
    assert(ptr);
    delete ptr;
}

uint16 Image_GetWidth(Image *image)
{ return image->width(); }

uint16 Image_GetHeight(Image *image)
{ return image->height(); }

byte* Image_GetData(Image *image)
{ return image->data_ptr(); }

const Palette* Image_GetPalette(Image *image)
{ return image->palette().get(); }

SpriteOffsets Image_GetOffsets(Image *image)
{ return image->offsets(); }

bool Image_IsIndexed(Image *image)
{ return image->is_indexed(); }

void Image_Convert(Image *image, PixelFormat format)
{ image->convert(format); }

void Image_Scale(Image *image, uint16_t new_width, uint16_t new_height)
{ image->scale(new_width, new_height); }

byte *Palette_GetData(Palette *pal)
{ return pal->data_ptr(); }

size_t Palette_GetCount(Palette *pal)
{ return pal->count(); }

int Palette_HasAlpha(Palette *pal)
{ return pal->traits().alpha; }

} // extern "C"
