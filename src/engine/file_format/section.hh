#ifndef __SECTION__66618357
#define __SECTION__66618357

#include <prelude.hh>

namespace imp {
  namespace wad {
    enum struct Section {
        normal,
        textures,
        graphics,
        sprites,
        sounds
    };
  }

  inline StringView to_string(wad::Section s)
  {
      using E = wad::Section;
      switch (s) {
      case E::normal:
          return "normal"_sv;

      case E::textures:
          return "textures"_sv;

      case E::graphics:
          return "graphics"_sv;

      case E::sprites:
          return "sprites"_sv;

      case E::sounds:
          return "sounds"_sv;
      }
  }
}

#endif //__SECTION__66618357
