#include <imp/Image>
#include "lump.hh"

Image wad::Buffer::as_image()
{
    auto& s = this->stream();

    return Image { s };
}

Image wad::Lump::as_image()
{ return buffer_->as_image(); }
