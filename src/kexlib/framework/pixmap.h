
#ifndef DOOM64EX_PIXMAP_H
#define DOOM64EX_PIXMAP_H

#include <framework/pixmap.h>

struct Pixmap {
    uint16_t width;
    uint16_t height;
    uint8_t pitch;
    PixelFormat fmt;

    size_t size;
    void *data;
};

struct pf_info {
    PixelFormat fmt;
    size_t bits;
    size_t bytes;
};

extern struct pf_info pf_table[];

#endif //DOOM64EX_PIXMAP_H
