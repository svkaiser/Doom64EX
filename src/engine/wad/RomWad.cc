#include <fstream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <imp/detail/Image.hh>
#include <set>
#include <easy/profiler.h>
#include <utility/endian.hh>
#include "RomWad.hh"
#include "deflate-N64.h"

String Deflate_Decompress(std::istream&);

Vector<String> rom_textures;
std::set<String> rom_weapon_sprites;

std::string get_midi(size_t midi);
namespace {
  template <class T>
  void read_into(std::istream& s, T& x)
  {
      s.read(reinterpret_cast<char*>(&x), sizeof(T));
  }

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

  /* Graphics lumps aren't located in the graphics directory, so we have to manually put them there */
  const StringView gfx_names_[] = {
      "SYMBOLS",  "USLEGAL",  "TITLE",   "EVIL",
      "FIRE",     "CLOUD",    "IDCRED1", "IDCRED2",
      "WMSCRED1", "WMSCRED2", "FINAL",
      "JPCPAK",   "PLLEGAL",  "JPLEGAL", "JAPFONT"
  };

  /* These are also graphics lumps, but they use the sprite format */
  const StringView gfx_sprites_[] = {
      "SFONT",   "STATUS",  "SPACE",   "MOUNTA",
      "MOUNTB",  "MOUNTC",  "JPMSG01", "JPMSG02",
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

  const StringView snd_names_[] = {
      "NOSOUND",  "SNDPUNCH", "SNDSPAWN", "SNDEXPLD",
      "SNDIMPCT", "SNDPSTOL", "SNDSHTGN", "SNDPLSMA",
      "SNDBFG",   "SNDSAWUP", "SNDSWIDL", "SNDSAW1",
      "SNDSAW2",  "SNDMISLE", "SNDBFGXP", "SNDPSTRT",
      "SNDPSTOP", "SNDDORUP", "SNDDORDN", "SNDSCMOV",
      "SNDSWCH1", "SNDSWCH2", "SNDITEM",  "SNDSGCK",
      "SNDOOF1",  "SNDTELPT", "SNDOOF2",  "SNDSHT2F",
      "SNDLOAD1", "SNDLOAD2", "SNDPPAIN", "SNDPLDIE",
      "SNDSLOP",  "SNDZSIT1", "SNDZSIT2", "SNDZSIT3",
      "SNDZDIE1", "SNDZDIE2", "SNDZDIE3", "SNDZACT",
      "SNDPAIN1", "SNDPAIN2", "SNDDBACT", "SNDSCRCH",
      "SNDISIT1", "SNDISIT2", "SNDIDIE1", "SNDIDIE2",
      "SNDIACT",  "SNDSGSIT", "SNDSGATK", "SNDSGDIE",
      "SNDB1SIT", "SNDB1DIE", "SNDHDSIT", "SNDHDDIE",
      "SNDSKATK", "SNDB2SIT", "SNDB2DIE", "SNDPESIT",
      "SNDPEPN",  "SNDPEDIE", "SNDBSSIT", "SNDBSDIE",
      "SNDBSLFT", "SNDBSSMP", "SNDFTATK", "SNDFTSIT",
      "SNDFTHIT", "SNDFTDIE", "SNDBDMSL", "SNDRVACT",
      "SNDTRACR", "SNDDART",  "SNDRVHIT", "SNDCYSIT",
      "SNDCYDTH", "SNDCYHOF", "SNDMETAL", "SNDDOR2U",
      "SNDDOR2D", "SNDPWRUP", "SNDLASER", "SNDBUZZ",
      "SNDTHNDR", "SNDLNING", "SNDQUAKE", "SNDDRTHT",
      "SNDRCACT", "SNDRCATK", "SNDRCDIE", "SNDRCPN",
      "SNDRCSIT", "MUSAMB01", "MUSAMB02", "MUSAMB03",
      "MUSAMB04", "MUSAMB05", "MUSAMB06", "MUSAMB07",
      "MUSAMB08", "MUSAMB09", "MUSAMB10", "MUSAMB11",
      "MUSAMB12", "MUSAMB13", "MUSAMB14", "MUSAMB15",
      "MUSAMB16", "MUSAMB17", "MUSAMB18", "MUSAMB19",
      "MUSAMB20", "MUSFINAL", "MUSDONE",  "MUSINTRO",
      "MUSTITLE"
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
   *
   * ---
   *
   * As with other members of the LZ- family of compression algorithms, the
   * compressed data is a stream of "codes". Each code is either a character
   * literal, or a dictionary pointer. The dictionary is the currently
   * decompressed buffer, with the rightmost element being the latest
   * decompressed character. The pointer is an offset-length pair, with the
   * offset being from the right. Ie. an offset of 0 refers to the latest char.
   *
   * This compression scheme prefixes a bitset of size 8 (a byte) which encodes
   * the type of the next 8 codes. If the bit is 0 then it's a character
   * literal, otherwise it's a dictionary pointer.
   *
   * A character literal is an 1-byte code, which is written as is to the output
   * buffer.
   *
   * A dictionary pointer is a 2-byte code with the first 12 bits being the
   * offset, and the next 4 being the length. If these 4 bits are all 0s, then
   * the decompression terminates (EOS). There is little point in encoding a
   * single character with a dictionary pointer, so the length is incremented by
   * 1. We get a possible length of [2, 16].
   */
  String lzss_decompress_(std::istream& in)
  {
      String out;

      int getidbyte {};
      int idbyte {};

      for (;;) {
          if (getidbyte == 0) {
              idbyte = in.get();
          }

          /* assign a new idbyte every 8th loop */
          getidbyte = (getidbyte + 1) & 7;

          if (idbyte & 1) {
              /* dictionary pointer */
              int off = (in.get() << 4u) | (in.peek() >> 4u);
              int len = in.get() & 0xfu;

              /* if length == 0, then we've reached end of stream */
              if (len == 0)
                  break;

              auto beg = out.size() - off - 1;
              auto end = beg + len + 1;

              /* copy dictionary into output */
              for (; beg < end; ++beg)
                  out.push_back(out[beg]);
          } else {
              /* character literal */
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
      String name {};
      Compression compression {};
      ImageFormat format {};
      Special special {};
      std::istringstream cache {};
      size_t pos {};
      size_t size {};
      size_t midi {};
      bool is_weapon {};
      String palette_name {};

      Info(StringView name, Compression compression, ImageFormat format, Special special, size_t pos, size_t size, bool is_weapon, const String& palette_name):
          name(name.to_string()),
          compression(compression),
          format(format),
          special(special),
          pos(pos),
          size(size),
          is_weapon(is_weapon),
          palette_name(palette_name) {}

      Info(StringView name, size_t midi):
          name(name.to_string()),
          midi(midi + 1) {}

      std::istream& stream(std::istream& rom) {
          if (!cache.str().empty()) {
              cache.seekg(0);
              return cache;
          }

          if (midi > 0) {
              cache.str(get_midi(midi - 1));
          } else {
              String lump;
              String buf;
              rom.seekg(pos);

              switch (compression) {
              case Compression::none:
                  lump.resize(size);
                  rom.read(&lump[0], size);
                  break;

              case Compression::lzss:
                  lump = lzss_decompress_(rom);
                  break;

              case Compression::n64:
                  lump = Deflate_Decompress(rom);
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
                  lump.replace(0, 8, "\xff\xff\0\0\0\x40\0\x40"s);
                  break;

              case Special::gfx_fire:
                  //lump.replace(0, 8, "\xff\xff\0\0\0\x40\0\x40"_sv);
                  break;
              }
              cache.str(std::move(lump));
          }

          return cache;
      }
  };

  class RomFormat : public wad::Mount {
      std::istringstream rom_ {};
      std::vector<Info> infos_ {};
      WadHeader wad_header_ {};
      String palette_name {};

  public:
      RomFormat():
          Mount(Type::rom),
          rom_(rom::wad())
      {
          rom_.exceptions(rom_.failbit | rom_.badbit);
          read_into(rom_, wad_header_);

          if (memcmp(wad_header_.id, "IWAD", 4) != 0)
              fatal("Not an IWAD");
      }

      Vector<wad::LumpInfo> read_all() override
      {
          EASY_FUNCTION(profiler::colors::Green);
          Vector<wad::LumpInfo> lumps;
          wad::Section section {};
          ImageFormat format {};
          bool recto0_hack {};

          rom_.seekg(wad_header_.infotableofs);
          for (std::size_t i = 0; i < wad_header_.numlumps; ++i) {
              WadDir dir;
              read_into(rom_, dir);
              std::size_t len = 0;
              while (len < 8 && dir.name[len]) ++len;

              if (wad_header_.numlumps == i + 1)
                  println("> WAD End: {:x}", static_cast<size_t>(rom_.tellg()));

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
                  palette_name = name;
              }

              if (section == wad::Section::textures) {
                  rom_textures.push_back(name);
              }

              bool is_weapon {};
              if (section == wad::Section::sprites && recto0_hack)
                  is_weapon = true;

              if (section == wad::Section::sprites && name == "RECTO0")
                  recto0_hack = true;

              lumps.push_back({ name, section, infos_.size() });
              infos_.emplace_back(name, compression, format, special, dir.filepos, dir.size, is_weapon, palette_name);
              section = section1;
          }

          for (size_t i {}; i < sizeof(snd_names_) / sizeof(*snd_names_); ++i) {
              auto& s = snd_names_[i];
              lumps.push_back({ s, wad::Section::sounds, infos_.size() });
              infos_.emplace_back(s, i);
          }

          return lumps;
      }

      bool set_buffer(wad::Lump& lump, size_t index) override
      {
          EASY_FUNCTION(profiler::colors::Green);
          auto& l = infos_[index];
          wad::RomSpriteInfo info { l.is_weapon, l.palette_name };
          lump.buffer(std::make_unique<wad::RomBuffer>(l.stream(rom_), l.format, info));

          return true;
      }
  };
}

UniquePtr<wad::Mount> wad::rom_loader(StringView path)
{
    String basename = path.to_string();
    auto it = basename.rfind('/');
    if (it != String::npos) {
        basename.erase(0, it + 1);
    }

    if (basename != "doom64.rom")
        return nullptr;

    return std::make_unique<RomFormat>();
}
