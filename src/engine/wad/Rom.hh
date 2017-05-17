#pragma once

#include <imp/Image>
#include "Mount.hh"

namespace imp {
  namespace wad {
    class RomBuffer : public LumpBuffer {
        std::istream& stream_;
        ImageFormat format_;

    public:
        RomBuffer(std::istream& stream, ImageFormat format):
            stream_(stream),
            format_(format) {}

        std::istream& stream() override
        { return stream_; }

        ImageFormat format() const
        { return format_; }
    };
  }
}
