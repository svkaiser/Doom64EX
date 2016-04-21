
    #include "pixmap.h"

template<typename Color>
static Pixmap *pixmap_resample_nearest(const Pixmap *src, int new_width, int new_height)
{
    Pixmap *out;
    int x, y;
    double dx, dy;
    Color *srcline, *dstline;

    out = Pixmap_New(new_width, new_height, src->pitch, src->fmt, NULL);

    dx = (double) src->width / new_width;
    dy = (double) src->height / new_height;

    for (y = 0; y < new_height; y++) {
        srcline = (Color *) Pixmap_GetScanline(src, (size_t) (dy * y));
        dstline = (Color *) Pixmap_GetScanline(out, (size_t) y);

        for (x = 0; x < new_width; x++) {
            dstline[x] = srcline[(int) (dx * x)];
        }
    }

    return out;
}

Pixmap *Pixmap_Resample(const Pixmap *src, int new_width, int new_height, PixmapInterp interp, PixmapExtrap extrap)
{
    switch(src->fmt) {
    case PF_PAL8:
        return pixmap_resample_nearest<PixelPAL8>(src, new_width, new_height);

    case PF_RGB24:
        return pixmap_resample_nearest<PixelRGB24>(src, new_width, new_height);

    case PF_BGR24:
        return pixmap_resample_nearest<PixelBGR24>(src, new_width, new_height);

    case PF_RGBA32:
        return pixmap_resample_nearest<PixelRGBA32>(src, new_width, new_height);

    case PF_ABGR32:
        return pixmap_resample_nearest<PixelABGR32>(src, new_width, new_height);

    case PF_BGRA32:
        return pixmap_resample_nearest<PixelBGRA32>(src, new_width, new_height);

    default:
        return NULL;
    }
}