#ifndef __METADATA__54286472
#define __METADATA__54286472

#include "section.hh"
#include "node.hh"

namespace imp {
  namespace wad {
    struct Metadata {
        String name {};
        Section section {};
        Node node {};

        Metadata(String name, Section section, Node node):
            name(name), section(section), node(node) {}
    };
  }
}

#endif //__METADATA__54286472
