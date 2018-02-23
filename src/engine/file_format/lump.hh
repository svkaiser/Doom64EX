#ifndef __LUMP__54634468
#define __LUMP__54634468

#include <istream>
#include "metadata.hh"

namespace imp {
  class Image;

  namespace wad {
    struct Buffer {
        virtual StringView real_path() const = 0;

        virtual std::istream& stream() = 0;

        virtual Image as_image();
    };

    class Lump {
        UniquePtr<Buffer> buffer_;
        const Metadata& meta_;

    protected:
        Lump(UniquePtr<Buffer>&& buffer, const Metadata& meta):
            buffer_(std::move(buffer)),
            meta_(meta) {}

    public:
        Lump() = delete;
        Lump(const Lump&) = delete;
        Lump(Lump&&) = default;

        std::istream& stream();

        Image as_image();

        StringView name() const
        { return meta_.name; }

        Section section() const
        { return meta_.section; }

        StringView real_path() const
        { return buffer_->real_path(); }
    };
  }
}

#endif //__LUMP__54634468
