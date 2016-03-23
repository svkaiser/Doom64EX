
#include <framework/pixel.h>

#include "pixmap.h"

#define SET_ERROR(x) { if (error) *error = x; }

struct pf_info pf_table[] = {
    { PF_RGB8, 24, 3 },
    { PF_RGBA8, 32, 4 },
};

uint16_t Pixmap_GetWidth(const Pixmap *pixmap)
{
    return pixmap->width;
}

uint16_t Pixmap_GetHeight(const Pixmap *pixmap)
{
    return pixmap->height;
}

size_t Pixmap_GetSize(const Pixmap *pixmap)
{
    return (size_t) pixmap_pad_width(pixmap->width, pixmap->pitch) * pixmap->height * pf_table[pixmap->fmt].bytes;
}

const void *Pixmap_GetData(const Pixmap *pixmap)
{
    return pixmap->data;
}

void *Pixmap_GetScanline(const Pixmap *pixmap, size_t idx)
{
    return pixmap->data + idx * pixmap_pad_width(pixmap->width, pixmap->pitch) * pf_table[pixmap->fmt].bytes;
}

PixelRGB8 Pixmap_GetRGB8(const Pixmap *pixmap, int x, int y)
{
    uint16_t padded_width;

    if (x < 0 || x >= pixmap->width || y < 0 || y >= pixmap->height) {
        return PixelRGB8_Black;
    }

    padded_width = pixmap_pad_width(pixmap->width, pixmap->pitch);

    switch (pixmap->fmt) {
    case PF_RGB8:
        return ((rgb8_t *)pixmap->data)[y * padded_width + x];

    case PF_RGBA8: {
        rgb8_t rgb8;
        rgba8_t rgba8;

        rgba8 = ((rgba8_t *)pixmap->data)[y * padded_width + x];
        rgb8.r = rgba8.r;
        rgb8.g = rgba8.g;
        rgb8.b = rgba8.b;

        return rgb8;
    }


    default:
        return PixelRGB8_Black;
    }
}
