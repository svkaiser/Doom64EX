#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_TRAITS_H__
#define __DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_TRAITS_H__

#include "pixel.h"

#ifdef __cplusplus

namespace kexlib {
template<typename T>
struct pixel_traits {
};

template<>
struct pixel_traits<PixelRGB24> {
    static constexpr PixelFormat format = PF_RGB24;
    static constexpr size_t bits = 24;
    static constexpr size_t bytes = 3;
    static constexpr bool palette = false;

    /* identity */
    static constexpr PixelRGB24 convert_from(PixelRGB24 from)
    {
        return from;
    }

    static constexpr PixelRGB24 convert_from(PixelBGR24 from)
    {
        return PixelRGB24(from.r, from.g, from.b);
    }

    static constexpr PixelRGB24 convert_from(PixelRGBA32 from)
    {
        return PixelRGB24(from.r, from.g, from.b);
    }

    static constexpr PixelRGB24 convert_from(PixelABGR32 from)
    {
        return PixelRGB24(from.r, from.g, from.b);
    }

    static constexpr PixelRGB24 convert_from(PixelBGRA32 from)
    {
        return PixelRGB24(from.r, from.g, from.b);
    }
};

template<>
struct pixel_traits<PixelBGR24> {
    static constexpr PixelFormat format = PF_BGR24;
    static constexpr size_t bits = 24;
    static constexpr size_t bytes = 3;
    static constexpr bool palette = false;

    /* identity */
    static constexpr PixelBGR24 convert_from(PixelBGR24 from)
    {
        return from;
    }

    static constexpr PixelBGR24 convert_from(PixelRGB24 from)
    {
        return PixelBGR24(from.r, from.g, from.b);
    }

    static constexpr PixelBGR24 convert_from(PixelRGBA32 from)
    {
        return PixelBGR24(from.r, from.g, from.b);
    }

    static constexpr PixelBGR24 convert_from(PixelABGR32 from)
    {
        return PixelBGR24(from.r, from.g, from.b);
    }

    static constexpr PixelBGR24 convert_from(PixelBGRA32 from)
    {
        return PixelBGR24(from.r, from.g, from.b);
    }
};

template<>
struct pixel_traits<PixelRGBA32> {
    static constexpr PixelFormat format = PF_RGBA32;
    static constexpr size_t bits = 32;
    static constexpr size_t bytes = 4;
    static constexpr bool palette = false;

    static constexpr PixelRGBA32 convert_from(PixelRGB24 from)
    {
        return PixelRGBA32(from.r, from.g, from.b, 0xff);
    }

    static constexpr PixelRGBA32 convert_from(PixelBGR24 from)
    {
        return PixelRGBA32(from.r, from.g, from.b, 0xff);
    }

    /* identity */
    static constexpr PixelRGBA32 convert_from(PixelRGBA32 from)
    {
        return from;
    }

    static constexpr PixelRGBA32 convert_from(PixelABGR32 from)
    {
        return PixelRGBA32(from.r, from.g, from.b, from.a);
    }

    static constexpr PixelRGBA32 convert_from(PixelBGRA32 from)
    {
        return PixelRGBA32(from.r, from.g, from.b, from.a);
    }
};

template<>
struct pixel_traits<PixelABGR32> {
    static constexpr PixelFormat format = PF_ABGR32;
    static constexpr size_t bits = 32;
    static constexpr size_t bytes = 4;
    static constexpr bool palette = false;

    static constexpr PixelABGR32 convert_from(PixelRGB24 from)
    {
        return PixelABGR32(from.r, from.g, from.b, 0xff);
    }

    static constexpr PixelABGR32 convert_from(PixelBGR24 from)
    {
        return PixelABGR32(from.r, from.g, from.b, 0xff);
    }

    static constexpr PixelABGR32 convert_from(PixelRGBA32 from)
    {
        return PixelABGR32(from.r, from.g, from.b, from.a);
    }

    /* identity */
    static constexpr PixelABGR32 convert_from(PixelABGR32 from)
    {
        return from;
    }

    static constexpr PixelABGR32 convert_from(PixelBGRA32 from)
    {
        return PixelABGR32(from.r, from.g, from.b, from.a);
    }
};

template<>
struct pixel_traits<PixelBGRA32> {
    static constexpr PixelFormat format = PF_BGRA32;
    static constexpr size_t bits = 32;
    static constexpr size_t bytes = 4;
    static constexpr bool palette = false;

    static constexpr PixelBGRA32 convert_from(PixelRGB24 from)
    {
        return PixelBGRA32(from.r, from.g, from.b, 0xff);
    }

    static constexpr PixelBGRA32 convert_from(PixelBGR24 from)
    {
        return PixelBGRA32(from.r, from.g, from.b, 0xff);
    }

    static constexpr PixelBGRA32 convert_from(PixelRGBA32 from)
    {
        return PixelBGRA32(from.r, from.g, from.b, from.a);
    }

    static constexpr PixelBGRA32 convert_from(PixelABGR32 from)
    {
        return PixelBGRA32(from.r, from.g, from.b, from.a);
    }

    /* identity */
    static constexpr PixelBGRA32 convert_from(PixelBGRA32 from)
    {
        return from;
    }
};
};

#endif //__cplusplus

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_TRAITS_H__
