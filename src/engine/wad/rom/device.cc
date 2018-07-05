#include <fstream>
#include <sstream>
#include <utility>
#include <algorithm>
#include <imp/detail/Image.hh>
#include <set>
#include <easy/profiler.h>
#include <utility/endian.hh>
#include "rom_private.hh"
#include "../wad_loaders.hh"
#include <system/n64_rom.hh>

using namespace imp::wad;

Vector<String> rom_textures;
std::set<String> rom_weapon_sprites;

std::string get_midi(size_t midi);
namespace {
  template<class T>
  void read_into(std::istream &s, T &x) {
      s.read(reinterpret_cast<char *>(&x), sizeof(T));
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

  /*
    Graphics lumps aren't located in the graphics directory, so we have to
    manually put them there.

    In sorted order.
  */
  const StringView gfx_names_[] = {
      "CLOUD", "EVIL", "FINAL", "FIRE",
      "IDCRED1", "IDCRED2", "JAPFONT", "JPCPAK",
      "JPLEGAL", "PLLEGAL", "SYMBOLS", "TITLE",
      "USLEGAL", "WMSCRED1", "WMSCRED2"
  };

  /*
    These are also graphics lumps, but they use the sprite format.

    In sorted order.
  */
  const StringView gfx_sprites_[] = {
      "JPMSG01", "JPMSG02", "JPMSG03", "JPMSG04",
      "JPMSG05", "JPMSG06", "JPMSG07", "JPMSG08",
      "JPMSG09", "JPMSG10", "JPMSG11", "JPMSG12",
      "JPMSG13", "JPMSG14", "JPMSG15", "JPMSG16",
      "JPMSG18", "JPMSG19", "JPMSG20", "JPMSG21",
      "JPMSG22", "JPMSG23", "JPMSG24", "JPMSG25",
      "JPMSG26", "JPMSG27", "JPMSG28", "JPMSG29",
      "JPMSG30", "JPMSG31", "JPMSG32", "JPMSG33",
      "JPMSG34", "JPMSG35", "JPMSG36", "JPMSG37",
      "JPMSG38", "JPMSG39", "JPMSG40", "JPMSG41",
      "JPMSG42", "JPMSG43", "JPMSG44", "JPMSG45",
      "MOUNTA", "MOUNTB", "MOUNTC", "SFONT",
      "SPACE", "STATUS"
  };

  const StringView snd_names_[] = {
      "NOSOUND", "SNDPUNCH", "SNDSPAWN", "SNDEXPLD",
      "SNDIMPCT", "SNDPSTOL", "SNDSHTGN", "SNDPLSMA",
      "SNDBFG", "SNDSAWUP", "SNDSWIDL", "SNDSAW1",
      "SNDSAW2", "SNDMISLE", "SNDBFGXP", "SNDPSTRT",
      "SNDPSTOP", "SNDDORUP", "SNDDORDN", "SNDSCMOV",
      "SNDSWCH1", "SNDSWCH2", "SNDITEM", "SNDSGCK",
      "SNDOOF1", "SNDTELPT", "SNDOOF2", "SNDSHT2F",
      "SNDLOAD1", "SNDLOAD2", "SNDPPAIN", "SNDPLDIE",
      "SNDSLOP", "SNDZSIT1", "SNDZSIT2", "SNDZSIT3",
      "SNDZDIE1", "SNDZDIE2", "SNDZDIE3", "SNDZACT",
      "SNDPAIN1", "SNDPAIN2", "SNDDBACT", "SNDSCRCH",
      "SNDISIT1", "SNDISIT2", "SNDIDIE1", "SNDIDIE2",
      "SNDIACT", "SNDSGSIT", "SNDSGATK", "SNDSGDIE",
      "SNDB1SIT", "SNDB1DIE", "SNDHDSIT", "SNDHDDIE",
      "SNDSKATK", "SNDB2SIT", "SNDB2DIE", "SNDPESIT",
      "SNDPEPN", "SNDPEDIE", "SNDBSSIT", "SNDBSDIE",
      "SNDBSLFT", "SNDBSSMP", "SNDFTATK", "SNDFTSIT",
      "SNDFTHIT", "SNDFTDIE", "SNDBDMSL", "SNDRVACT",
      "SNDTRACR", "SNDDART", "SNDRVHIT", "SNDCYSIT",
      "SNDCYDTH", "SNDCYHOF", "SNDMETAL", "SNDDOR2U",
      "SNDDOR2D", "SNDPWRUP", "SNDLASER", "SNDBUZZ",
      "SNDTHNDR", "SNDLNING", "SNDQUAKE", "SNDDRTHT",
      "SNDRCACT", "SNDRCATK", "SNDRCDIE", "SNDRCPN",
      "SNDRCSIT", "MUSAMB01", "MUSAMB02", "MUSAMB03",
      "MUSAMB04", "MUSAMB05", "MUSAMB06", "MUSAMB07",
      "MUSAMB08", "MUSAMB09", "MUSAMB10", "MUSAMB11",
      "MUSAMB12", "MUSAMB13", "MUSAMB14", "MUSAMB15",
      "MUSAMB16", "MUSAMB17", "MUSAMB18", "MUSAMB19",
      "MUSAMB20", "MUSFINAL", "MUSDONE", "MUSINTRO",
      "MUSTITLE"
  };
}

class imp::wad::rom::Device : public IDevice {
    std::istringstream rom_ {};
    WadHeader wad_header_ {};
    String palette_name {};

public:
    Device(std::istringstream&& iwad):
        rom_(std::move(iwad))
    {
        rom_.exceptions(rom_.failbit | rom_.badbit);
        read_into(rom_, wad_header_);

        if (memcmp(wad_header_.id, "IWAD", 4) != 0) {
            log::fatal("Not an IWAD");
        }
    }

    Vector<ILumpPtr> read_all() override
    {
        EASY_FUNCTION(profiler::colors::Green);
        Vector<ILumpPtr> lumps;
        wad::Section section {};
        bool is_weapon {};

        enum {
            F_NONE,
            F_GFX,
            F_SPR,
            F_TEX,
            F_PAL
        } format {};

        String sprite_substr {};
        SharedPtr<Palette> sprite_pal {};

        SpriteLump* sprite_lump_ptr {};
        rom_.seekg(wad_header_.infotableofs);
        for (std::size_t i = 0; i < wad_header_.numlumps; ++i) {
            auto lump_pos = static_cast<std::streampos>(rom_.tellg());
            Hack hack {};

            WadDir dir;
            read_into(rom_, dir);
            std::size_t len = 0;
            while (len < 8 && dir.name[len]) ++len;

            dir.name[0] &= 0x7f;

            std::size_t size {};
            while (size < 8 && dir.name[size]) ++size;
            String name { dir.name, size };

            // TODO: It'd be cool to actually be able to read and use the demo
            // files
            if (name.substr(0, 4) == "DEMO") {
                log::debug("Ignoring lump '{}'", name);
                continue;
            }

            if (section == wad::Section::graphics) {
                section = wad::Section::normal;
                format = F_NONE;
            }

            bool gfx_found {};
            for (auto n : gfx_names_) {
                if (name == n) {
                    section = wad::Section::graphics;
                    format = F_GFX;
                    gfx_found = true;

                    if (name == "CLOUD") {
                        hack = Hack::cloud;
                    }
                    break;
                }
            }

            if (!gfx_found) {
                for (auto n : gfx_sprites_) {
                    if (name == n) {
                        section = wad::Section::graphics;
                        format = F_SPR;
                        gfx_found = true;
                        break;
                    }
                }
            }

            if (!gfx_found && dir.size == 0) {
                if (name == "T_START") {
                    section = wad::Section::textures;
                    format = F_TEX;
                } else if (name == "S_START") {
                    section = wad::Section::sprites;
                    format = F_SPR;
                } else if (name == "T_END") {
                    section = wad::Section::normal;
                    format = F_NONE;
                } else if (name == "S_END") {
                    section = wad::Section::normal;
                    format = F_NONE;
                } else if (name == "ENDOFWAD") {
                    break;
                } else {
                    log::warn("Unknown WAD directory: {}", name);
                }
                continue;
            }

            auto format1 = format;
            wad::Section section1 = section;
            if (name.substr(0, 3) == "PAL") {
                section = wad::Section::normal;
                palette_name = name;
                format = F_PAL;
            }

            if (section == wad::Section::textures) {
                rom_textures.push_back(name);
            }

            Info lump_info { name, section, lump_pos, hack };
            ILumpPtr lump_ptr;

            if (format == F_TEX) {
                lump_ptr = std::make_unique<TextureLump>(*this, lump_info);
            } else if (format == F_GFX) {
                lump_ptr = std::make_unique<GfxLump>(*this, lump_info);
            } else if (format == F_SPR) {
                if (is_weapon && sprite_lump_ptr && sprite_lump_ptr->name().substr(0, 4) != name.substr(0, 4)) {
                    sprite_lump_ptr = nullptr;
                }

                auto lump = std::make_unique<SpriteLump>(*this, lump_info, is_weapon, sprite_lump_ptr);

                if (is_weapon && !sprite_lump_ptr) {
                    sprite_lump_ptr = lump.get();
                }

                lump_ptr = std::move(lump);
            } else if (format == F_PAL) {
                lump_ptr = std::make_unique<PaletteLump>(*this, lump_info);
            } else {
                lump_ptr = std::make_unique<NormalLump>(*this, lump_info);
            }

            lumps.emplace_back(std::move(lump_ptr));
            section = section1;
            format = format1;

            // Every sprite after RECTO0 is a weapon sprite.
            if (!is_weapon && section == wad::Section::sprites && name == "RECTO0")
                is_weapon = true;
        }

        for (size_t i {}; i < sizeof(snd_names_) / sizeof(*snd_names_); ++i) {
            auto& name = snd_names_[i];

            auto lump_info = Info { name.to_string(), Section::sounds, 0 };
            auto lump_ptr = std::make_unique<SoundLump>(*this, lump_info, i);

            lumps.emplace_back(std::move(lump_ptr));
        }

        return lumps;
    }

    /*! Load a lump's data at a given position */
    std::istringstream load(Info info)
    {
        std::istringstream iss, raw;
        rom_.seekg(info.pos);

        WadDir dir;
        read_into(rom_, dir);

        {
            String rawstr;
            rawstr.resize(dir.size);
            rom_.seekg(static_cast<std::streamoff>(dir.filepos));
            rom_.read(&rawstr[0], dir.size);
            raw.str(std::move(rawstr));
        }

        if (dir.name[0] < 0) {
            // If the sign bit of the first char is set (ie. it's negative),
            // then the lump is compressed.
            if (info.section == Section::textures || info.name.substr(0, 3) == "MAP") {
                iss.str(deflate(raw));
            } else {
                auto data = lzss(raw);

                if (info.hack == Hack::cloud) {
                    /*
                     * CLOUD lump has an invalid header, but is otherwise an 8bpp
                     * image with Rgba5551 palette just like the other Graphics
                     * images.
                     */

                    /* word 0: compression (0xffff is -1) */
                    /* word 1: unused (zeroes) */
                    /* word 2: width 64px (big endian 0x40) */
                    /* word 3: height 64px (big endian 0x40) */

                    data.replace(0, 8, "\xff\xff\0\0\0\x40\0\x40"s);
                }

                iss.str(data);
            }
        } else {
            iss = std::move(raw);
        }

        return iss;
    }
};

std::istringstream wad::rom::Lump::p_stream()
{
    return device_.load(info_);
}

IDevice& wad::rom::Lump::device()
{
    return device_;
}

UniquePtr<std::istream> wad::rom::SoundLump::stream()
{
    return std::make_unique<std::istringstream>(get_midi(track_));
}

IDevicePtr wad::rom_loader(StringView path)
{
    sys::N64Rom rom(path);
    if (!rom.is_open())
        return nullptr;

    return std::make_unique<rom::Device>(rom.iwad());
}
