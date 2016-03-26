
#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_H__
#define __DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_H__

#include "../kexlib.h"

KEX_C_BEGIN

enum PixelFormat {
    PF_INVALID  = 0x00,

    /* Palette */
    PF_PAL8     = 0x01,

    /* RGB */
    PF_RGB24    = 0x02,
    PF_BGR24    = 0x03,

    /* RGB Alpha */
    PF_RGBA32   = 0x04,
    PF_ABGR32   = 0x05,
    PF_BGRA32   = 0x06,

    PF_NUM
};

struct PixelPAL8 {
    uint8_t c;
};

struct PixelRGB24 {
    uint8_t r;
    uint8_t g;
    uint8_t b;

#ifdef __cplusplus
    constexpr PixelRGB24(): r(0), g(0), b(0) {}
    constexpr PixelRGB24(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b) {}
#endif
};

struct PixelBGR24 {
    uint8_t b;
    uint8_t g;
    uint8_t r;

#ifdef __cplusplus
    constexpr PixelBGR24(): r(0), g(0), b(0) {}
    constexpr PixelBGR24(uint8_t r, uint8_t g, uint8_t b): r(r), g(g), b(b) {}
#endif
};

struct PixelRGBA32 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

#ifdef __cplusplus
    constexpr PixelRGBA32(): r(0), g(0), b(0), a(0) {}
    constexpr PixelRGBA32(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {}
#endif
};

struct PixelABGR32 {
    uint8_t a;
    uint8_t b;
    uint8_t g;
    uint8_t r;

#ifdef __cplusplus
    constexpr PixelABGR32(): r(0), g(0), b(0), a(0) {}
    constexpr PixelABGR32(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {}
#endif
};

struct PixelBGRA32 {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;

#ifdef __cplusplus
    constexpr PixelBGRA32(): r(0), g(0), b(0), a(0) {}
    constexpr PixelBGRA32(uint8_t r, uint8_t g, uint8_t b, uint8_t a): r(r), g(g), b(b), a(a) {}
#endif
};

typedef enum PixelFormat PixelFormat;
typedef struct PixelPAL8 PixelPAL8;
typedef struct PixelRGB24 PixelRGB24;
typedef struct PixelBGR24 PixelBGR24;
typedef struct PixelRGBA32 PixelRGBA32;
typedef struct PixelABGR32 PixelABGR32;
typedef struct PixelBGRA32 PixelBGRA32;

typedef PixelRGB24 rgb24_t;
typedef PixelBGR24 bgr24_t;
typedef PixelRGBA32 rgba32_t;
typedef PixelABGR32 abgr32_t;
typedef PixelBGRA32 bgra32_t;

KEXAPI const PixelRGB24 PixelRGB24_Black;   // #000
KEXAPI const PixelRGB24 PixelRGB24_Blue;    // #00f
KEXAPI const PixelRGB24 PixelRGB24_Green;   // #0f0
KEXAPI const PixelRGB24 PixelRGB24_Cyan;    // #0ff
KEXAPI const PixelRGB24 PixelRGB24_Red;     // #f00
KEXAPI const PixelRGB24 PixelRGB24_Magenta; // #f0f
KEXAPI const PixelRGB24 PixelRGB24_Yellow;  // #ff0
KEXAPI const PixelRGB24 PixelRGB24_White;   // #fff

KEX_C_END

#ifdef __cplusplus
inline bool operator==(const PixelRGB24 &lhs, const PixelRGB24 &rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}
#endif

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_H__
