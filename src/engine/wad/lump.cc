#include <imp/Image>
#include "lump.hh"

using namespace imp::wad;

Optional<Image> Lump::read_image()
{ return context_->read_image(); }

Optional<Palette> Lump::read_palette()
{ return context_->read_palette(); }
