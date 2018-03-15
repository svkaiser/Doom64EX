#pragma once

#include <imp/Image>

#include <system/Rom.hh>

namespace imp {
  namespace wad {
    struct RomSpriteInfo {
        bool is_weapon {};
        String palette_name {};
    };

    class RomBuffer : public LumpBuffer {
        std::istream& stream_;
        ImageFormat format_;
        RomSpriteInfo sprite_info_;

    public:
        RomBuffer(std::istream& stream, ImageFormat format, const RomSpriteInfo& info):
            stream_(stream),
            format_(format),
            sprite_info_(info) {}

        std::istream& stream() override
        { return stream_; }

        ImageFormat format() const
        { return format_; }

        const auto& sprite_info() const
        { return sprite_info_; }
    };
  }
}
