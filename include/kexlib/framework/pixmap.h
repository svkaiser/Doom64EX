
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

enum PixmapInterp {
    PIXMAP_INTERP_NEAREST,
};

enum PixmapExtrap {
    PIXMAP_EXTRAP_NEAREST,
};

struct Pixmap {
    uint16_t width;
    uint16_t height;
    uint8_t pitch;
    PixelFormat fmt;

    void *data;
};

typedef struct Pixmap Pixmap;
typedef enum PixmapError PixmapError;
typedef enum PixmapInterp PixmapInterp;
typedef enum PixmapExtrap PixmapExtrap;

KEXAPI Pixmap *Pixmap_New(int width, int height, int pitch, PixelFormat fmt, PixmapError *error);
KEXAPI Pixmap *Pixmap_NewFrom(const void *src, int width, int height, int pitch, PixelFormat fmt, PixmapError *error);
KEXAPI Pixmap *Pixmap_Clone(const Pixmap *src, PixmapError *error);
KEXAPI void Pixmap_Raw(Pixmap *dst, const void *data, int width, int height, int pitch, PixelFormat fmt);
KEXAPI void Pixmap_Free(Pixmap *ptr);

KEXAPI uint16_t Pixmap_GetWidth(const Pixmap *pixmap);
KEXAPI uint16_t Pixmap_GetHeight(const Pixmap *pixmap);
KEXAPI size_t Pixmap_GetSize(const Pixmap *pixmap);

KEXAPI const void *Pixmap_GetData(const Pixmap *pixmap);
KEXAPI void *Pixmap_GetScanline(const Pixmap *pixmap, size_t idx);
KEXAPI PixelRGB24 Pixmap_GetRGB(const Pixmap *pixmap, int x, int y);

KEXAPI Pixmap *Pixmap_Resize(const Pixmap *src, int new_width, int new_height, PixmapError *error);
KEXAPI Pixmap *Pixmap_Resample(const Pixmap *src, int new_width, int new_height, PixmapInterp interp, PixmapExtrap extrap);
KEXAPI void Pixmap_Reformat_InPlace(Pixmap **pixmap, PixelFormat new_fmt);

KEX_C_END

#endif //__DOOM64EX_KEXLIB_FRAMEWORK_PIXMAP_H__
