
#include "pixmap.h"

Pixmap *Pixmap_New(uint16_t width, uint16_t height, PixmapFormat fmt)
{
    Pixmap *pixmap;

    pixmap = malloc(sizeof(*pixmap));
    pixmap->width = width;
    pixmap->height = height;
    pixmap->fmt = fmt;

    return pixmap;
}

void Pixmap_Free(Pixmap *ptr)
{
    free(ptr);
}

uint16_t Pixmap_GetWidth(Pixmap *pixmap)
{
    return pixmap->width;
}

uint16_t Pixmap_GetHeight(Pixmap *pixmap)
{
    return pixmap->height;
}