
#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_PALETTE_H__
#define __DOOM64EX_KEXLIB_FRAMEWORK_PALETTE_H__

#include "pixel.h"

KEX_C_BEGIN

struct Palette {
    PixelFormat palette_fmt;
    PixelFormat pixel_fmt;
    void *data;
};

typedef struct Palette Palette;

KEXAPIPalette *Palette_New(PixelFormat palette_fmt, PixelFormat pixel_fmt);


KEX_C_END

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PALETTE_H__
