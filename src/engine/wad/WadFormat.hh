#include <imp/Wad>

namespace imp {
  namespace wad {
    struct LumpInfo {
        String lump_name;
        std::size_t lump_index {};
        Section section;
        std::size_t section_index {};
        std::size_t mount {};
        std::size_t mount_index;

        LumpInfo(String lump_name, Section section, std::size_t index):
            lump_name(lump_name),
            section(section),
            mount_index(index) {}

        LumpInfo(const LumpInfo&) = delete;

        LumpInfo(LumpInfo&&) = default;

        LumpInfo& operator=(const LumpInfo&) = delete;

        LumpInfo& operator=(LumpInfo&&) = default;
    };

    struct Format {
        using loader = UniquePtr<Format> (*)(StringView);

        virtual ~Format() {}

        virtual Vector<LumpInfo> read_all() = 0;

        virtual UniquePtr<BasicLump> find(std::size_t lump_id, std::size_t mount_id) = 0;
    };

    UniquePtr<Format> doom_loader(StringView);
    UniquePtr<Format> zip_loader(StringView);
    UniquePtr<Format> rom_loader(StringView);
  }
}
