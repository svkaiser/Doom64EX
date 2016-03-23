
#include "pixmap.h"

Pixmap *Pixmap_Resize(const Pixmap *src, int new_width, int new_height, PixmapError *error)
{
    Pixmap *pixmap;
    int y;
    int width, height;
    void *srcline, *dstline;

    if (!(pixmap = Pixmap_New(new_width, new_height, src->pitch, src->fmt, error))) {
        return NULL;
    }

    width = MIN(new_width, src->width);
    height = MIN(new_height, src->height);

    for (y = 0; y < height; y++) {
        srcline = Pixmap_GetScanline(src, (size_t) y);
        dstline = Pixmap_GetScanline(pixmap, (size_t) y);

        memcpy(dstline, srcline, width * pf_table[src->fmt].bytes);
    }

    return pixmap;
}

Pixmap *Pixmap_Resize_Raw(const void *data, int old_width, int old_height, int pitch, PixelFormat fmt,
                          int new_width, int new_height, PixmapError *error)
{
    Pixmap src;

    src.width = (uint16_t) old_width;
    src.height = (uint16_t) old_height;
    src.pitch = (uint8_t) pitch;
    src.fmt = fmt;

    src.data = (void *) data;

    return Pixmap_Resize(&src, new_width, new_height, error);
}