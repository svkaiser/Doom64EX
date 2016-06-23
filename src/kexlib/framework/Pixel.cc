#include <framework/Pixel>

using namespace kex::gfx;

namespace {

template<typename _T>
constexpr auto pixel_traits_gen()
{
    using traits_type = PixelTraits<_T>;
    return PixelTypeinfo {traits_type::format, traits_type::bits, traits_type::bytes};
}

const PixelTypeinfo formats[] = {
    pixel_traits_gen<Index8>(),
    pixel_traits_gen<Rgb>(),
    pixel_traits_gen<Rgba>(),
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
