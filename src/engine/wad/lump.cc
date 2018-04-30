#include <imp/Image>
#include "lump.hh"

using namespace imp::wad;

Optional<Image> Lump::read_image()
{ return m_context->read_image(); }

Optional<Palette> Lump::read_palette()
{ return m_context->read_palette(); }
