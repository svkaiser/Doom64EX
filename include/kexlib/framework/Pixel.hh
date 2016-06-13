#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_HH__
#define __DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_HH__

#include <KexDef.hh>

enum struct PixelFormat {
    Unknown,
    RGB,
    RGBA,
    Index8
};

struct PixelFormatType {
    size_t bits;
    bool palette;
};

template <PixelFormat _Format>
struct PixelTraits
{
    constexpr auto format = _Format;
    constexpr size_t bits = 0;
    constexpr size_t bytes = 0;
};

template <>
struct PixelTraits<PixelFormat::RGB>
{
    constexpr auto format = PixelFormat::RGB;
    constexpr size_t bits = 24;
    constexpr size_t bytes = 3;
};

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_HH__
