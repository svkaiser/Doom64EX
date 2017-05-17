#include <fstream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <imp/detail/Image.hh>
#include "Rom.hh"

void Deflate_Decompress(byte * input, byte * output);
void Wad_Decompress(byte * input, byte * output);

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

  /* Graphics lumps aren't located in the graphics directory, so we have to manually put them there */
  const StringView gfx_names_[] = {
      "SYMBOLS", "USLEGAL", "TITLE", "EVIL",
      "FIRE", "CLOUD", "IDCRED1", "IDCRED2",
      "WMSCRED1", "WMSCRED2", "FINAL",
      "JPCPAK", "PLLEGAL", "JPLEGAL", "JAPFONT"
      };

  /* These are also graphics lumps, but they use the sprite format */
  const StringView gfx_sprites_[] = {
      "SFONT", "STATUS", "SPACE", "MOUNTA",
      "MOUNTB", "MOUNTC", "JPMSG01", "JPMSG02",
      "JPMSG03", "JPMSG04", "JPMSG05", "JPMSG06",
      "JPMSG07", "JPMSG08", "JPMSG09", "JPMSG10",
      "JPMSG11", "JPMSG12", "JPMSG13", "JPMSG14",
      "JPMSG15", "JPMSG16", "JPMSG18", "JPMSG19",
      "JPMSG20", "JPMSG21", "JPMSG22", "JPMSG23",
      "JPMSG24", "JPMSG25", "JPMSG26", "JPMSG27",
      "JPMSG28", "JPMSG29", "JPMSG30", "JPMSG31",
      "JPMSG32", "JPMSG33", "JPMSG34", "JPMSG35",
      "JPMSG36", "JPMSG37", "JPMSG38", "JPMSG39",
      "JPMSG40", "JPMSG41", "JPMSG42", "JPMSG43",
      "JPMSG44", "JPMSG45"
  };

  enum struct Special {
      none,
      gfx_cloud,
      gfx_fire
  };

  /* From Wadgen's wad.c
   *
   * Based off of JaguarDoom's decompression algorithm.
   * This is a rather simple LZSS-type algorithm that was used
   * on all lumps in JaguardDoom and PSXDoom.
   * Doom64 only uses this on the Sprites and GFX lumps, but uses
   * a more sophisticated algorithm for everything else.
   */
  [[gnu::unused]]
  String lzss_decompress_(std::istream& in)
  {
      String out;

      int getidbyte {};
      int idbyte {};
      while (!in.eof()) {
          if (!getidbyte)
              idbyte = in.get();

          /* assign a new idbyte every 8th loop */
          getidbyte = (getidbyte + 1) & 7;

          if (idbyte & 1) {
              /* begin decompressing and get position */
              char c;

              in.get(c);
              uint16 pos = static_cast<uint8>(c) << 4;

              in.get(c);
              pos |= static_cast<uint8>(c) >> 4;

              /* setup length */
              int len = (static_cast<uint8>(c) & 0xf) + 1;
              if (len == 1)
                  break;

              /* setup string */
              auto source = out.end() - pos - 1;

              assert(pos < len);

              /* copy source into output */
              std::copy_n(source, len, out.end());
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

  enum struct Compression {
      none,
      lzss,
      n64
  };

  struct Info {
      String name;
      Compression compression;
      ImageFormat format;
      Special special;
      std::istringstream cache;
      size_t pos;
      size_t size;

      Info(String name, Compression compression, ImageFormat format, Special special, size_t pos, size_t size):
          name(name),
          compression(compression),
          format(format),
          special(special),
          pos(pos),
          size(size) {}

      std::istream& stream(std::istream& rom) {
          if (!cache.str().empty()) {
              cache.seekg(0);
              return cache;
          }

          String buf;
          String lump;
          rom.seekg(pos);

          switch (compression) {
          case Compression::none:
              lump.resize(size);
              rom.read(&lump[0], size);
              break;

          case Compression::lzss:
              buf.resize(size);
              lump.resize(size);
              rom.read(&buf[0], size);
              Wad_Decompress(reinterpret_cast<byte *>(&buf[0]), reinterpret_cast<byte *>(&lump[0]));
              break;

          case Compression::n64:
              buf.resize(size);
              lump.resize(size);
              rom.read(&buf[0], size);
              Deflate_Decompress(reinterpret_cast<byte *>(&buf[0]), reinterpret_cast<byte *>(&lump[0]));
              break;
          }

          switch (special) {
          case Special::none:
              break;

          case Special::gfx_cloud:
              /*
               * CLOUD lump has an invalid header, but is otherwise an 8bpp
               * image with Rgba5551 palette just like the other Graphics
               * images.
               */

              /* word 0: compression (0xffff is -1) */
              /* word 1: unused (zeroes) */
              /* word 2: width 64px (big endian 0x40) */
              /* word 3: height 64px (big endian 0x40) */
              lump.replace(0, 8, "\xff\xff\0\0\0\x40\0\x40"_sv);
              break;

          case Special::gfx_fire:
              break;
          }

          cache.str(std::move(lump));
          return cache;
      }
  };

  class RomFormat : public wad::Mount {
      std::istringstream rom_ {};
      std::vector<Info> infos_ {};
      Header header_ {};
      WadHeader wad_header_ {};
      size_t wad_pos_ {};

  public:
      RomFormat(StringView name)
      {
          rom_.exceptions(rom_.failbit | rom_.badbit);

          String rom_data;

          {
              std::ifstream file(name, std::ios::binary);

              file.seekg(0, file.end);
              rom_data.resize(static_cast<size_t>(file.tellg()));
              file.seekg(0, file.beg);

              file.read(&rom_data[0], rom_data.size());
          }

          if (rom_data.size() < sizeof(Header))
              throw "ROM is too small";

          std::copy_n(rom_data.data(), sizeof(Header), reinterpret_cast<char*>(&header_));

          // Swap bytes if the ROM is big-endian.
          if (memcmp(header_.name, _n64_name, 6) == 0) {
              for (size_t i = 0; i < rom_data.size(); i += 2)
                  std::swap(rom_data[i], rom_data[i+1]);
          }

          if (memcmp(header_.name, _z64_name, 6) != 0)
              fatal("Not a valid Doom 64 ROM");

          // Put the entire rom into the string stream
          rom_.str(std::move(rom_data));

          // Find the location of the WAD
          for (const auto& loc : _rom_iwad) {
              if (loc.country_id == header_.country_id && loc.version == header_.version_id) {
                  rom_.seekg(loc.position);
                  wad_pos_ = loc.position;
              }
          }

          if (wad_pos_ == 0) {
              fatal("WAD not found in Doom 64 ROM");
          }

          read_into(rom_, wad_header_);

          if (memcmp(wad_header_.id, "IWAD", 4) != 0)
              fatal("Not an IWAD");
      }

      Vector<wad::LumpInfo> read_all() override
      {
          Vector<wad::LumpInfo> lumps;
          wad::Section section {};
          ImageFormat format {};

          rom_.seekg(wad_pos_ + wad_header_.infotableofs);
          for (std::size_t i = 0; i < wad_header_.numlumps; ++i) {
              WadDir dir;
              read_into(rom_, dir);
              std::size_t len = 0;
              while (len < 8 && dir.name[len]) ++len;

              bool compressed = dir.name[0] < 0;
              dir.name[0] &= 0x7f;

              std::size_t size {};
              while (size < 8 && dir.name[size]) ++size;
              String name { dir.name, size };

              if (name.substr(0, 4) == "DEMO")
                  continue;

              if (section == wad::Section::graphics)
                  section = wad::Section::normal;

              bool gfx_found {};
              for (auto n : gfx_names_) {
                  if (name == n) {
                      section = wad::Section::graphics;
                      format = ImageFormat::n64gfx;
                      gfx_found = true;
                  }
              }
              if (!gfx_found) {
                  for (auto n : gfx_sprites_) {
                      if (name == n) {
                          section = wad::Section::graphics;
                          format = ImageFormat::n64sprite;
                          gfx_found = true;
                      }
                  }
              }
              if (!gfx_found && dir.size == 0) {
                  if (name == "T_START") {
                      section = wad::Section::textures;
                      format = ImageFormat::n64texture;
                  } else if (name == "S_START") {
                      section = wad::Section::sprites;
                      format = ImageFormat::n64sprite;
                  } else if (name == "T_END") {
                      section = wad::Section::normal;
                  } else if (name == "S_END") {
                      section = wad::Section::normal;
                  } else if (name == "ENDOFWAD") {
                      break;
                  } else {
                      println("Unknown WAD directory: {}", name);
                  }
                  continue;
              }

              Compression compression {};
              if (compressed) {
                  if (name.substr(0, 3) == "MAP" || section == wad::Section::textures) {
                      compression = Compression::n64;
                  } else {
                      compression = Compression::lzss;
                  }
              }

              wad::Section section1 = section;
              Special special {};
              if (name == "CLOUD")
                  special = Special::gfx_cloud;
              else if (name == "FIRE")
                  special = Special::gfx_fire;
              else if (name.substr(0, 3) == "PAL") {
                  section = wad::Section::normal;
              }

              lumps.push_back({ name, section, infos_.size() });
              infos_.emplace_back(name, compression, format, special, dir.filepos + wad_pos_, dir.size);
              section = section1;
          }

          return lumps;
      }

      bool set_buffer(wad::Lump& lump, size_t index) override
      {
          auto& l = infos_[index];
          lump.buffer(std::make_unique<wad::RomBuffer>(l.stream(rom_), l.format));

          return true;
      }
  };
}

UniquePtr<wad::Mount> wad::rom_loader(StringView name)
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
