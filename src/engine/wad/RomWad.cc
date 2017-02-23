#include <fstream>
#include "WadFormat.hh"

namespace {
  template <class T>
  void read_into(std::istream& s, T& x)
  {
      s.read(reinterpret_cast<char*>(&x), sizeof(T));
  }

  constexpr const char _z64_name[] = "Doom64";
  constexpr const char _n64_name[] = "oDmo46";

  struct RomIwad {
      std::size_t position;
      char country_id;
      char version;
  };

#if 0
  RomIwad _rom_iwad[4] {
      { 0x6df60, 'P', 0 },
      { 0x64580, 'J', 0 },
      { 0x63d10, 'E', 0 },
      { 0x63dc0, 'E', 1 }
  };
#endif

  struct WadHeader {
      char id[4];
      uint32 numlumps;
      uint32 infotableofs;
  };

  struct WadDir {
      uint32 filepos;
      uint32 size;
      char name[8];
  };

  struct Header {
      byte x1;		/* initial PI_BSB_DOM1_LAT_REG value */
      byte x2;		/* initial PI_BSB_DOM1_PGS_REG value */
      byte x3;		/* initial PI_BSB_DOM1_PWD_REG value */
      byte x4;		/* initial PI_BSB_DOM1_RLS_REG value */
      uint32 clock_rate;
      uint32 boot_address;
      uint32 release;
      uint32 crc1;
      uint32 crc2;
      uint32 unknown0;
      uint32 unknown1;
      char name[20];
      uint32 unknown2;
      uint16 unknown3;
      byte unknown4;
      byte manufacturer;
      uint16 cart_id;
      char country_id;
      byte version_id;
  };

  static_assert(sizeof(Header) == 64, "N64 ROM header struct must be sizeof 64");

  class RomFormat : public wad::Format {
  public:
      RomFormat(StringView name)
      {
          std::ifstream rom(name);

          Header header;
          read_into(rom, header);

          rom.seekg(0x63d10);
          WadHeader wad_header;
          read_into(rom, wad_header);

          if (memcmp(wad_header.id, "IWAD", 4) && memcmp(wad_header.id, "PWAD", 4))
              throw "Not a WAD";

          rom.seekg(0x63d10 + wad_header.infotableofs);
          for (std::size_t i = 0; i < wad_header.numlumps; ++i) {
              WadDir dir;
              read_into(rom, dir);
              std::size_t len = 0;
              while (len < 8 && dir.name[len]) ++len;

              bool compressed = dir.name[0] < 0;
              dir.name[0] &= 0x7f;

              println("> {}: {}{}", String { dir.name, len }, dir.size, compressed ? " (compressed)" : "");
          }
          std::exit(0);
      }

      ~RomFormat() override {}

      UniquePtr<wad::Reader> reader() override { return {}; }

      Optional<wad::Lump> find(std::size_t id) override { return {}; }

      Optional<wad::Lump> find(StringView name) override { return {}; }
  };
}

UniquePtr<wad::Format> wad::rom_loader(StringView name)
{
    std::ifstream rom(name);
    if (!rom.is_open())
        return nullptr;

    Header header;
    read_into(rom, header);

    if (memcmp(header.name, _z64_name, 6) && memcmp(header.name, _n64_name, 6))
        return nullptr;

    return std::make_unique<RomFormat>(name);
}
