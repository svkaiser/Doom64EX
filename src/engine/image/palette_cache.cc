#include <unordered_map>

#include "palette_cache.hh"
#include "image.hh"

#include "wad/wad.hh"

template <class T1, class T2>
struct mutable_pair {
    T1 first;
    mutable T2 second;
};

template <class Key, class T, class Hash = boost::hash<Key>, class KeyEq = std::equal_to<>>
using HashMap = std::unordered_map<Key, T, Hash, KeyEq>;

namespace {
  HashMap<String, Palette> palettes_;

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
    auto sname = name.to_string();
    auto it = palettes_.find(sname);
    if (it != palettes_.cend())
        return it->second;

    Optional<wad::Lump> optlump;
    Optional<Palette> optpal;

    Palette pal {};
    if ((optlump = wad::open(name)) && (optpal = optlump->read_palette())) {
        pal = optpal.value();
    } else {
        pal = default_palette_();
    }

    palettes_[sname] = pal;
    return pal;
}
