
#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_H__
#define __DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_H__

#include "../kexlib.h"

KEX_C_BEGIN

typedef enum PixmapFormat {
    PF_RGB8,
} PixmapFormat;

typedef struct Pixmap Pixmap;

KEXAPI Pixmap *Pixmap_New(uint16_t width, uint16_t height, PixmapFormat fmt);
KEXAPI void Pixmap_Free(Pixmap *ptr);

KEXAPI uint16_t Pixmap_GetWidth(Pixmap *pixmap);
KEXAPI uint16_t Pixmap_GetHeight(Pixmap *pixmap);

KEX_C_END

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_H__
