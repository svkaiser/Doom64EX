
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

#endif //DOOM64EX_PIXMAP_H
