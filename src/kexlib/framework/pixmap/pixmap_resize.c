
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