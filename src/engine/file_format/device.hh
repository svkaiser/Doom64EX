#ifndef __DEVICE__20952143
#define __DEVICE__20952143

#include "section.hh"
#include "section_iterator.hh"
#include "metadata.hh"

namespace imp {
  namespace wad {
    struct Device {
        virtual ~Device() {}

        virtual Optional<Metadata> stat(Section section, StringView node) = 0;

        virtual UniquePtr<Buffer> open(Node node) = 0;

        virtual SectionIterator list(Section section) = 0;
    };

    using device_loader = UniquePtr<Device> (*)(StringView path);
  }
}

#endif //__DEVICE__20952143
