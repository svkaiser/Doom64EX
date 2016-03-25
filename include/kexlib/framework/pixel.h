
#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_H__
#define __DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_H__

#include "../kexlib.h"

KEX_C_BEGIN

enum PixelFormat {
    PF_RGB8,
    PF_RGBA8,
    PF_ABGR8,

    PF_NUM
};

struct PixelRGB8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct PixelRGBA8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct PixelABGR8 {
    uint8_t a;
    uint8_t b;
    uint8_t g;
    uint8_t r;
};

typedef enum PixelFormat PixelFormat;
typedef struct PixelRGB8 PixelRGB8;
typedef struct PixelRGBA8 PixelRGBA8;
typedef struct PixelABGR8 PixelABGR8;

typedef PixelRGB8 rgb8_t;
typedef PixelRGBA8 rgba8_t;

KEXAPI PixelRGB8 PixelRGB8_Black;   // #000
KEXAPI PixelRGB8 PixelRGB8_Blue;    // #00f
KEXAPI PixelRGB8 PixelRGB8_Green;   // #0f0
KEXAPI PixelRGB8 PixelRGB8_Cyan;    // #0ff
KEXAPI PixelRGB8 PixelRGB8_Red;     // #f00
KEXAPI PixelRGB8 PixelRGB8_Magenta; // #f0f
KEXAPI PixelRGB8 PixelRGB8_Yellow;  // #ff0
KEXAPI PixelRGB8 PixelRGB8_White;   // #fff

KEX_C_END

#ifdef __cplusplus
inline bool operator==(const PixelRGB8 &lhs, const PixelRGB8 &rhs)
{
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
}
#endif

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PIXEL_H__
