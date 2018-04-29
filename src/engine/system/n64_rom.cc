
#include <fstream>
#include <platform/app.hh>
#include <prelude.hh>
#include <boost/algorithm/string.hpp>

#include "n64_rom.hh"

namespace {
  const sys::N64Version g_versions[4] = {
      { 'P', 0, { 0x63f60, 0x5d6cdc }, { 0x63ac40, 0x50000 }, { 0x646620, 0x50000 }, {0x83c40, 0x50000} },
      { 'J', 0, { 0x64580, 0x5d8478 }, { 0, 0 }, { 0x6483e0, 0 }, {0, 0} },
      { 'E', 0, { 0x63d10, 0x5d18b0 }, { 0x6355c0, 0x50000 }, { 0x640fa0, 0x50000 }, {0x6552a0, 0x500000} },
      { 'E', 1, { 0x63dc0, 0x5d301c }, { 0x63ac40, 0x50000 }, { 0x646620, 0x50000 }, {0, 0} }
  };

  struct Header {
      uint8 x1;		/* initial PI_BSB_DOM1_LAT_REG value */
      uint8 x2;		/* initial PI_BSB_DOM1_PGS_REG value */
      uint8 x3;		/* initial PI_BSB_DOM1_PWD_REG value */
      uint8 x4;		/* initial PI_BSB_DOM1_RLS_REG value */
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
      uint8 unknown4;
      uint8 manufacturer;
      uint16 cart_id;

      char country;
      char version;
  };

  static_assert(sizeof(Header) == 64, "N64 ROM header struct must be sizeof 64");
}

std::istringstream sys::N64Rom::m_load(const sys::N64Loc &loc)
{
    assert(m_file.is_open());
    assert(m_rom_version != nullptr);

    std::string buf;
    buf.resize(loc.size);

    m_file.seekg(loc.offset);
    m_file.read(&buf[0], loc.size);

    if (m_swapped) {
        for (size_t i {}; i + 1 < buf.size(); i += 2) {
            std::swap(buf[i], buf[i+1]);
        }
    }

    std::istringstream iss;
    iss.str(buf);
    iss.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
    return iss;
}

bool sys::N64Rom::open(StringView path)
{
    return false;

    /* Used to detect endianess. Padded to 20 characters. */
    constexpr auto norm_name = "Doom64              "_sv;
    constexpr auto swap_name = "oDmo46              "_sv;

    Header header;

    m_file.open(path.to_string());
    m_file.read(reinterpret_cast<char*>(&header), sizeof(header));

    char country {};
    char version {};
    if (boost::iequals(norm_name, header.name)) {
        country = header.country;
        version = header.version;
        m_swapped = false;
    } else if (boost::iequals(swap_name, header.name)) {
        country = header.version;
        version = header.country;
        m_swapped = true;
    } else {
        m_error = "Could not detect ROM";
    }

    for (const auto& l : g_versions) {
        if (l.country_id == country && l.version_id == version) {
            m_rom_version = &l;
            assert(l.sn64.size != 0 && l.sn64.offset != 0);
            assert(l.sseq.size != 0 && l.sseq.offset != 0);
            assert(l.iwad.size != 0 && l.iwad.offset != 0);
            assert(l.pcm.size != 0 && l.pcm.offset != 0);
            break;
        }
    }

    if (!m_rom_version) {
        m_error = fmt::format("WAD not found in Doom 64 ROM. (Country: {}, Version: {})",
                              country, version);
        return false;
    } else {
        m_version = fmt::format("(Country: {}, Version: {:d})", country, version);
    }

    return true;
}

std::istringstream sys::N64Rom::iwad()
{
    return m_load(m_rom_version->iwad);
}

std::istringstream sys::N64Rom::sn64()
{
    return m_load(m_rom_version->sn64);
}

std::istringstream sys::N64Rom::sseq()
{
    return m_load(m_rom_version->sseq);
}

std::istringstream sys::N64Rom::pcm()
{
    return m_load(m_rom_version->pcm);
}
