#include <sstream>
#include <imp/Wad>
#include <map>
#include "Map.hh"

namespace {
  template <class T>
  void read_into(std::istream& s, T& x)
  {
      s.read(reinterpret_cast<char*>(&x), sizeof(T));
  }

  struct Header {
      char id[4];
      uint32 numlumps;
      uint32 infotableofs;
  };

  struct Directory {
      uint32 filepos;
      uint32 size;
      char name[8];
  };

  struct MapLump {
      String data;
      wad::Mount& source;
  };

  std::vector<MapLump> _lumps;

  std::map<int, int> texturehashlist_;
}

void* W_GetMapLump(int lump)
{
    return &_lumps[lump].data[0];
}

void W_CacheMapLump(int map)
{
    auto file = wad::find(format("MAP{:02d}", map));

    if (!file) {
        fatal("Could not find MAP{:02d}", map);
    }

    _lumps.clear();

    std::istringstream ss(file->as_bytes());

    Header header;
    read_into(ss, header);

    if (memcmp(header.id, "IWAD", 4) != 0 && memcmp(header.id, "PWAD", 4)) {
        fatal("MAP{:02d} is an invalid WAD", map);
    }

    std::size_t numlumps = header.numlumps;
    ss.seekg(header.infotableofs);
    for (std::size_t i = 0; !ss.eof() && i < numlumps; ++i) {
        Directory dir;
        read_into(ss, dir);

        auto pos = ss.tellg();
        ss.seekg(dir.filepos);
        String str(dir.size, 0);
        ss.read(&str[0], dir.size);
        ss.seekg(pos);

        _lumps.push_back({ std::move(str), file->source() });
    }
}

void W_FreeMapLump()
{
    // nop
}

int W_MapLumpLength(int lump)
{
    return _lumps[lump].data.size();
}

//
// P_InitTextureHashTable
//

extern Vector<String> rom_textures;
void P_InitTextureHashTable(void) {
    for (size_t i = 0; i < rom_textures.size(); ++i) {
        auto& s = rom_textures[i];
        auto a = wad::find(s).value().section_index();
        texturehashlist_.emplace(i, a);
    }
    auto section = wad::section(wad::Section::textures);
    for(int i = 0; section; ++section, ++i) {
        auto lump = *section;
        texturehashlist_.emplace(wad::LumpHash { lump.lump_name() }.get(), i);
    }
}

//
// P_GetTextureHashKey
//

uint32 P_GetTextureHashKey(int hash) {
    auto it = texturehashlist_.find(hash);
    if (it == texturehashlist_.end()) {
        return 0;
    } else {
        return it->second;
    }
}
