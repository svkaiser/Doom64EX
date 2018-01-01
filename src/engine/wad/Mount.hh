#pragma once

#include <imp/Wad>

namespace imp {
  namespace wad {
    struct LumpInfo {
        String name;
        Section section;
        size_t index;

        size_t lump_index {};
        size_t mount_index {};
        size_t section_index {};

        LumpInfo(StringView name, Section section, size_t index):
            name(name.to_string()),
            section(section),
            index(index) {}
    };

    struct Mount {
        enum struct Type {
            doom,
            zip,
            rom
        } const type;

        Mount(Type type): type(type) {}
        virtual ~Mount() {}

        virtual Vector<LumpInfo> read_all() = 0;
        virtual bool set_buffer(Lump& lump, size_t index) = 0;
    };

    using mount_loader = UniquePtr<Mount> (StringView);
    mount_loader doom_loader;
    mount_loader zip_loader;
    mount_loader rom_loader;
  }
}
