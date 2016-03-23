
#include "pixmap.h"

static Pixmap *pixmap_resample_rgba8(const Pixmap *src, int new_width, int new_height, PixmapInterp interp, PixmapExtrap extrap)
{
    Pixmap *pixmap;
    int x, y;
    double dx, dy;
    rgba8_t *srcline, *dstline;

    pixmap = Pixmap_New(new_width, new_height, src->pitch, src->fmt, NULL);

    dx = (double) src->width / new_width;
    dy = (double) src->height / new_height;

    for (y = 0; y < new_height; y++) {
        srcline = Pixmap_GetScanline(src, (size_t) (dy * y));
        dstline = Pixmap_GetScanline(pixmap, (size_t) y);

        for (x = 0; x < new_width; x++) {
            dstline[x] = srcline[(int) (dx * x)];
        }
    }

    return pixmap;
}

Pixmap *Pixmap_Resample(const Pixmap *src, int new_width, int new_height, PixmapInterp interp, PixmapExtrap extrap)
{
    switch(src->fmt) {
    case PF_RGBA8:
        return pixmap_resample_rgba8(src, new_width, new_height, interp, extrap);

    default:
        return NULL;
    }
}

Pixmap *Pixmap_Resample_Raw(const void *data, int old_width, int old_height, int pitch, PixelFormat fmt,
                            int new_width, int new_height, PixmapInterp interp, PixmapExtrap extrap)
{
    Pixmap src;

    src.width = (uint16_t) old_width;
    src.height = (uint16_t) old_height;
    src.pitch = (uint8_t) pitch;
    src.fmt = fmt;

    src.data = (void *) data;

    return Pixmap_Resample(&src, new_width, new_height, interp, extrap);
}