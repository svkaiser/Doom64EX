
#ifndef __DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_H__
#define __DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_H__

#include "pixel.h"

KEX_C_BEGIN

/* TODO: Relocate into one big enum for all kexlib-related stuff, maybe. */
enum PixmapError {
    PIXMAP_ESUCCESS = 0, // Success
    PIXMAP_EINVALFMT = 1, // Invalid Format
    PIXMAP_EINVALDIM = 2, // Invalid dimensions
};

typedef struct Pixmap Pixmap;
typedef enum PixmapError PixmapError;

KEXAPI Pixmap *Pixmap_New(int width, int height, int pitch, PixelFormat fmt, PixmapError *error);
KEXAPI Pixmap *Pixmap_NewFrom(const void *src, int width, int height, int pitch, PixelFormat fmt, PixmapError *error);
KEXAPI void Pixmap_Free(Pixmap *ptr);

KEXAPI uint16_t Pixmap_GetWidth(const Pixmap *pixmap);
KEXAPI uint16_t Pixmap_GetHeight(const Pixmap *pixmap);
KEXAPI size_t Pixmap_GetSize(const Pixmap *pixmap);

KEXAPI const void *Pixmap_GetData(const Pixmap *pixmap);
KEXAPI void *Pixmap_GetScanline(const Pixmap *pixmap, size_t idx);
KEXAPI PixelRGB8 Pixmap_GetRGB8(const Pixmap *pixmap, int x, int y);

KEXAPI Pixmap *Pixmap_Resize(const Pixmap *src, int new_width, int new_height, PixmapError *error);
KEXAPI Pixmap *Pixmap_Resize_Raw(const void *data, int old_width, int old_height, int pitch, PixelFormat fmt,
                                 int new_width, int new_height, PixmapError *error);

KEX_C_END

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_H__
