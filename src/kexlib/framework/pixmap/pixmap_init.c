
#include <framework/pixel.h>

#include "pixmap_priv.h"

#define SET_ERROR(x) { if (error) *error = x; }

struct pf_info pf_table[] = {
    { PF_INVALID, 0, 0 },
    { PF_PAL8, 8, 1 },
    { PF_RGB24, 24, 3 },
    { PF_BGR24, 24, 3 },
    { PF_RGBA32, 32, 4 },
    { PF_ABGR32, 32, 4 },
    { PF_BGRA32, 32, 4 },
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

PixelRGB24 Pixmap_GetRGB(const Pixmap *pixmap, int x, int y)
{
    uint16_t padded_width;
    rgb24_t rgb24;

    if (x < 0 || x >= pixmap->width || y < 0 || y >= pixmap->height) {
        return PixelRGB24_Black;
    }

    padded_width = pixmap_pad_width(pixmap->width, pixmap->pitch);

    switch (pixmap->fmt) {
    case PF_BGR24: {
        bgr24_t bgr24;

        bgr24 = ((bgr24_t *) pixmap->data)[y * padded_width + x];
        rgb24.r = bgr24.r;
        rgb24.g = bgr24.g;
        rgb24.b = bgr24.b;

        return rgb24;
    }

    case PF_RGB24:
        return ((rgb24_t *)pixmap->data)[y * padded_width + x];

    case PF_RGBA32: {
        rgba32_t rgba32;

        rgba32 = ((rgba32_t *)pixmap->data)[y * padded_width + x];
        rgb24.r = rgba32.r;
        rgb24.g = rgba32.g;
        rgb24.b = rgba32.b;

        return rgb24;
    }

    case PF_ABGR32: {
        abgr32_t abgr32;

        abgr32 = ((abgr32_t *)pixmap->data)[y * padded_width + x];
        rgb24.r = abgr32.r;
        rgb24.g = abgr32.g;
        rgb24.b = abgr32.b;

        return rgb24;
    }

    default:
        return PixelRGB24_Black;
    }
}
