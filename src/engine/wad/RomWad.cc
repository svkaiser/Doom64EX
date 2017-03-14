#include <fstream>
#include <sstream>
#include <utility>
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

  RomIwad _rom_iwad[4] {
      { 0x6df60, 'P', 0 },
      { 0x64580, 'J', 0 },
      { 0x63d10, 'E', 0 },
      { 0x63dc0, 'E', 1 }
  };

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
      /* From Wadgen's wad.c
       *
       * Based off of JaguarDoom's decompression algorithm.
       * This is a rather simple LZSS-type algorithm that was used
       * on all lumps in JaguardDoom and PSXDoom.
       * Doom64 only uses this on the Sprites and GFX lumps, but uses
       * a more sophisticated algorithm for everything else.
       */
      String lzss_decompress_(std::istream& in)
      {
          String out;

          int getidbyte {};
          char idbyte {};
          while (!in.eof()) {
              if (!getidbyte)
                  in.get(idbyte);

              /* assign a new idbyte every 8th loop */
              getidbyte = (getidbyte + 1) & 7;

              if (idbyte & 1) {
                  /* begin decompressing and get position */
                  uint16 pos;
                  read_into(in, pos);

                  /* setup length */
                  char len {};
                  in.get(len);
                  len = static_cast<char>((len & 0xf) + 1);
                  if (len == 1)
                      break;

                  /* setup string */
                  auto source = out.substr(out.size() - pos - 1);

                  /* copy source into output */
                  for (int i = 0; i < len; ++i)
                      out.append(source);
              } else {
                  /* not compressed, just output the byte as is */
                  char c;
                  in.get(c);
                  out.push_back(c);
              }

              /* shift to next bit and begin the check at the beginning */
              idbyte >>= 1;
          }

          return out;
      }

  public:
      RomFormat(StringView name)
      {
          std::istringstream rom;
          rom.exceptions(rom.failbit | rom.badbit);

          {
              std::ifstream file(name, std::ios::binary);
              rom.str({ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() });
          }

          Header rom_header;
          read_into(rom, rom_header);

          // Swap bytes if the ROM is big-endian.
          if (memcmp(rom_header.name, _n64_name, 6) == 0) {
              auto str = rom.str();
              for (size_t i = 0; i < str.size(); i += 2)
                  std::swap(str[i], str[i+1]);
              rom.str(str);
              read_into(rom, rom_header);
          }

          if (memcmp(rom_header.name, _z64_name, 6) != 0)
              fatal("Not a valid Doom 64 ROM");

          // Find the location of the WAD
          std::size_t wad_pos {};
          for (const auto& loc : _rom_iwad) {
              if (loc.country_id == rom_header.country_id && loc.version == rom_header.version_id) {
                  rom.seekg(loc.position, std::ios::beg);
                  wad_pos = loc.position;
              }
          }

          if (wad_pos == 0) {
              fatal("WAD not found in Doom 64 ROM");
          }

          WadHeader wad_header;
          read_into(rom, wad_header);

          if (memcmp(wad_header.id, "IWAD", 4) != 0)
              fatal("Not an IWAD");

          rom.seekg(wad_pos + wad_header.infotableofs);
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

      Vector<wad::LumpInfo> read_all() override { return {}; }

      UniquePtr<wad::BasicLump> find(size_t lump_id, size_t mount_id) override { return {}; }
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
