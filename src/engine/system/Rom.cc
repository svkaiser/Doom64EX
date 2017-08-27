
#include <fstream>
#include <imp/App>
#include <imp/Prelude>

#include "Rom.hh"

namespace {
  bool was_init_ {};
  std::ifstream rom_;

  bool swapped_ {};

  struct Location {
      std::size_t offset;
      std::size_t size;
  };

  struct VersionLocation {
      char country;
      char version;
      Location wad;
      Location sn64;
      Location sseq;
  };

  const VersionLocation locations_[4] = {
      { 'P', 0, { 0x63f60, 0x5d6cdc }, { 0x63ac40, 0 }, { 0x646620, 0 } },
      { 'J', 0, { 0x64580, 0x5d8478 }, { 0, 0 }, { 0x6483e0, 0 } },
      { 'E', 0, { 0x63d10, 0x5d18b0 }, { 0, 0 }, { 0, 0 } },
      { 'E', 1, { 0x63dc0, 0x5d301c }, { 0, 0 }, { 0, 0 } }
  };

  const VersionLocation* location_ {};

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

  std::istringstream load_(const Location& l)
  {
      std::string b;
      b.resize(l.size);

      rom_.seekg(l.offset);
      rom_.read(&b[0], l.size);

      if (swapped_) {
          for (size_t i {}; i + 1 < b.size(); i += 2) {
              std::swap(b[i], b[i+1]);
          }
      }

      std::istringstream s;
      s.str(std::move(b));
      return s;
  }
}

void rom::init()
{
    /* Used to detect endianess. Padded to 20 characters. */
    constexpr auto norm_name = "Doom64              "_sv;
    constexpr auto swap_name = "oDmo46              "_sv;

    if (was_init_)
        fatal("Rom already initialized");

    Header header;

    auto path = app::find_data_file("doom64.rom");
    if (!path)
        fatal("Coultn't find N64 ROM (doom64.rom)");

    rom_.open(path.value());
    rom_.read(reinterpret_cast<char*>(&header), sizeof(header));

    char country {};
    char version {};
    if (norm_name.icompare(header.name) == 0) {
        country = header.country;
        version = header.version;
        swapped_ = false;
    } else if (swap_name.icompare(header.name) == 0) {
        country = header.version;
        version = header.country;
        swapped_ = true;
    } else {
        fatal("Could not detect ROM");
    }

    for (const auto& l : locations_) {
        if (l.country == country && l.version == version) {
            location_ = &l;
            break;
        }
    }

    if (!location_) {
        fatal("WAD not found in Doom 64 ROM. (Country: {}, Version: {})",
              country, version);
    } else {
        println("(Country: {}, Version: {:d})", country, version);
    }

    was_init_ = true;
}

std::istringstream rom::wad()
{
    return load_(location_->wad);
}

std::istringstream rom::sn64()
{
    return load_(location_->sn64);
}

std::istringstream rom::sseq()
{
    return load_(location_->sseq);
}
