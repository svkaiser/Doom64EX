#ifndef __SECTION__76973969
#define __SECTION__76973969

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

    constexpr size_t num_sections = 5;
  }

  inline StringView to_string(wad::Section s)
  {
      using Section = wad::Section;
      switch (s) {
      case Section::normal:
          return "normal"_sv;

      case Section::textures:
          return "textures"_sv;

      case Section::graphics:
          return "graphics"_sv;

      case Section::sprites:
          return "sprites"_sv;

      case Section::sounds:
          return "sounds"_sv;

      default:
          return ""_sv;
      }
  }
}

#endif //__SECTION__76973969
