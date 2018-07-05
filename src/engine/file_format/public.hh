#ifndef __PUBLIC__63056608
#define __PUBLIC__63056608

#include "lump.hh"

namespace imp {
  namespace wad {
    Optional<Lump> open(Section section, StringView lname);

    MetaSectionIterator list(Section section);

    inline Optional<Lump> open(StringView lname)
    { return open(Section::normal, lname); }
  }
}

#endif //__PUBLIC__63056608
