#include <fstream>
#include <sstream>
#include "Mount.hh"

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

  static_assert(sizeof(Header) == 12, "WAD Header must have a sizeof of 12 bytes");
  static_assert(sizeof(Directory) == 16, "WAD Directory must have a sizeof of 16 bytes");

  struct Info {
      size_t filepos;
      size_t size;
      std::istringstream cache;

      Info(size_t filepos, size_t size):
          filepos(filepos),
          size(size) {}
  };

  class DoomLump : public wad::LumpBuffer {
      std::istream &stream_;

  public:
      DoomLump(std::istream& stream):
          stream_(stream) {}

      std::istream& stream() override
      {
          stream_.seekg(0);
          return stream_;
      }
  };

  class DoomFormat : public wad::Mount {
      std::ifstream stream_;
      Vector<Info> table_;

  public:
      DoomFormat(StringView path):
          Mount(Type::doom),
          stream_(path.to_string(), std::ios::binary)
      {
          stream_.exceptions(stream_.failbit | stream_.badbit);
      }

      Vector<wad::LumpInfo> read_all() override
      {
          Vector<wad::LumpInfo> lumps;
          wad::Section section {};
          Header header;
          read_into(stream_, header);

          stream_.seekg(header.infotableofs);
          size_t numlumps = header.numlumps;

          table_.clear();
          for (size_t i = 0; i < numlumps; ++i) {
              Directory dir;
              read_into(stream_, dir);

              std::size_t size {};
              while (size < 8 && dir.name[size]) ++size;
              String name { dir.name, size };

              if (dir.size == 0) {
                  if (name == "T_START") {
                      section = wad::Section::textures;
                  } else if (name == "G_START") {
                      section = wad::Section::graphics;
                  } else if (name == "S_START") {
                      section = wad::Section::sprites;
                  } else if (name == "DS_START") {
                      section = wad::Section::sounds;
                  } else if (name == "T_END") {
                      section = wad::Section::normal;
                  } else if (name == "G_END") {
                      section = wad::Section::normal;
                  } else if (name == "S_END") {
                      section = wad::Section::normal;
                  } else if (name == "DS_END") {
                      section = wad::Section::normal;
                  } else if (name == "ENDOFWAD") {
                      break;
                  } else {
                      println("Unknown WAD directory: {}", name);
                  }
                  continue;
              }

              lumps.push_back({ name, section, table_.size() });
              table_.emplace_back(dir.filepos, dir.size);
          }

          return lumps;
      }

      bool set_buffer(wad::Lump& lump, size_t index) override
      {
          std::istringstream ss;
          auto& info = table_[index];

          if (info.size && !info.cache.str().size()) {
              stream_.seekg(info.filepos);
              String str(info.size, 0);
              stream_.read(&str[0], info.size);
              info.cache.str(std::move(str));
          }

          lump.buffer(std::make_unique<DoomLump>(info.cache));

          return true;
      }
  };
}

UniquePtr<wad::Mount> wad::doom_loader(StringView path)
{
    std::ifstream file(path.to_string(), std::ios::binary);
    Header header;
    read_into(file, header);
    if (memcmp(header.id, "IWAD", 4) == 0 || memcmp(header.id, "PWAD", 4) == 0) {
        return std::make_unique<DoomFormat>(path);
    } else {
        return nullptr;
    }
}
