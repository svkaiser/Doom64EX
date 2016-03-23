
#ifndef DOOM64EX_PIXMAP_H
#define DOOM64EX_PIXMAP_H

#include <framework/pixmap.h>

struct pf_info {
    PixelFormat fmt;
    size_t bits;
    size_t bytes;
};

static inline uint16_t pixmap_pad_width(uint16_t width, uint8_t pitch)
{
    if (pitch == 0)
        return width;

    return (uint16_t) (((width / pitch) + 1) * pitch);
}

Pixmap *pixmap_alloc(int width, int height, int pitch, PixelFormat fmt, PixmapError *error);

extern struct pf_info pf_table[];

#endif //DOOM64EX_PIXMAP_H
