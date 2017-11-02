#include <unordered_map>

#include "wad/RomWad.hh"
#include "wad/Mount.hh"
#include "PaletteCache.hh"
#include "Image.hh"

namespace {
  std::unordered_map<String, Palette> palettes_;

  Palette default_palette_()
  {
      static Palette data {};
      if (data) return data;

      RgbPalette pal { 256 };
      uint8 i {};
      for (auto &c: pal)
          c.red = c.green = c.blue = i++;

      data = std::move(pal);
      return data;
  }
}

Palette cache::palette(StringView name)
{
    auto it = palettes_.find(name);
    if (it != palettes_.cend())
        return it->second;

    Palette pal {};
    if (auto lump = wad::find(name)) {
        if (lump->source().type == wad::Mount::Type::rom) {
            auto& s = lump->stream();
            s.seekg(8, s.cur);
            auto n64pal = read_n64palette(s, 256);
            pal = n64pal;
        } else {
            RgbaPalette rgbpal { 256 };
            auto& s = lump->stream();
            s.read(rgbpal.data_ptr(), 256 * 4);
            pal = std::move(rgbpal);
        }
    } else {
        pal = default_palette_();
    }

    palettes_[name] = pal;
    return pal;
}
