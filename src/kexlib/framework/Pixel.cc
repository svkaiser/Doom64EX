#include <framework/Pixel>

using namespace kex::gfx;

namespace {

template<PixelFormat _T>
constexpr auto pixel_traits_gen()
{
    using traits_type = PixelTraits<_T>;
    return PixelTypeinfo {traits_type::format, traits_type::bits, traits_type::bytes};
}

const PixelTypeinfo formats[] = {
    pixel_traits_gen<PixelFormat::unknown>(),
    pixel_traits_gen<PixelFormat::index8>(),
    pixel_traits_gen<PixelFormat::rgb>(),
    pixel_traits_gen<PixelFormat::rgba>(),
};

}

const PixelTypeinfo &kex::gfx::GetPixelTypeinfo(PixelFormat pFormat)
{
    for (const PixelTypeinfo &x : formats)
    {
        if (x.format == pFormat)
            return x;
    }

    return formats[0];
}
