#include <fstream>
#include "WadFormat.hh"

namespace {
  template <class T>
  std::istream& read_into(std::istream& s, T& x)
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
      wad::Section section;
      size_t filepos;
      size_t size;
      String name;
      SharedPtr<char> data;
  };

  class DoomReader : public wad::Reader {
      using iterator = typename Vector<Info>::const_iterator;

      iterator it_;
      iterator begin_;
      iterator end_;

  public:
      DoomReader(iterator begin, iterator end):
          it_(begin),
          begin_(begin),
          end_(end) {}

      ~DoomReader() override {}

      bool poll() override
      {
          if (it_ == end_)
              return false;

          this->section = it_->section;
          this->id = it_ - begin_;
          this->name = it_->name;
          ++it_;
      }
  };

  class DoomFormat : public wad::Format {
      std::ifstream stream_;
      Vector<Info> table_;

  public:
      DoomFormat(StringView path):
          stream_(path, std::ios::binary)
      {
          wad::Section section {};
          Header header;
          read_into(stream_, header);

          stream_.seekg(header.infotableofs);
          size_t numlumps = header.numlumps;
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

              table_.push_back({ section, dir.filepos, dir.size, name, nullptr });
          }
      }

      ~DoomFormat() override {}

      UniquePtr<wad::Reader> reader() override
      {
          return std::make_unique<DoomReader>(table_.cbegin(), table_.cend());
      }

      Optional<wad::Lump> find(std::size_t id) override
      {
          auto& info = table_[id];

          if (info.size && !info.data) {
              info.data.reset(new char[info.size]);
              stream_.seekg(info.filepos);
              stream_.read(info.data.get(), info.size);
          }

          return make_optional<wad::Lump>(info.name, info.data, info.size, info.section);
      }

      Optional<wad::Lump> find(StringView) override
      { return nullopt; }
  };
}

UniquePtr<wad::Format> wad::doom_loader(StringView path)
{
    std::ifstream file(path, std::ios::binary);
    Header header;
    read_into(file, header);
    if (memcmp(header.id, "IWAD", 4) == 0 || memcmp(header.id, "PWAD", 4) == 0) {
        return std::make_unique<DoomFormat>(path);
    } else {
        return nullptr;
    }
}
