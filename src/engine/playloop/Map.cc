#include <sstream>
#include <imp/Wad>
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

  std::vector<String> _lumps;
}

void* W_GetMapLump(int lump)
{
    return &_lumps[lump][0];
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
        _lumps.emplace_back(dir.size, 0);
        ss.read(&_lumps.back()[0], dir.size);
        ss.seekg(pos);
    }
}

void W_FreeMapLump()
{
    // nop
}

int W_MapLumpLength(int lump)
{
    return _lumps[lump].size();
}
