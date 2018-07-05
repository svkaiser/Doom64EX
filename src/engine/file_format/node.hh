#ifndef __NODE__79843104
#define __NODE__79843104

#include <prelude.hh>

namespace imp {
  namespace wad {
    class Node {
        size_t mount_: sizeof(size_t) / 2;
        size_t fd_: sizeof(size_t) / 2;

    public:
        constexpr Node():
        mount_(0), fd_(0) {}
        constexpr Node(const Node&) = default;
        constexpr Node(Node&&) = default;

        size_t mount() const
        { return mount_; }

        void set_mount(size_t mount)
        { mount_ = mount; }

        size_t fd() const
        { return fd_; }
    };
  }
}

#endif //__NODE__79843104
