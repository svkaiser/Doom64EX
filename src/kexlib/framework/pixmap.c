
#include <framework/pixel.h>

#include "pixmap.h"

#define SET_ERROR(x) { if (error) *error = x; }

struct pf_info pf_table[] = {
    { PF_RGB8, 24, 3 },
    { PF_RGBA8, 32, 4 },
};

static uint16_t pad_width(uint16_t width, uint8_t pitch)
{
    if (pitch == 0)
        return width;

    return (uint16_t) (((width / pitch) + 1) * pitch);
}

static Pixmap *alloc_pixmap(int width, int height, int pitch, PixelFormat fmt, PixmapError *error)
{
    Pixmap *pixmap;

    if (width < 0 || width >= UINT16_MAX ||
            height < 0 || height >= UINT16_MAX ||
            pitch < 0 || pitch >= UINT8_MAX) {
        SET_ERROR(PIXMAP_EINVALDIM);
        return NULL;
    }

    if (fmt < 0 || fmt >= PF_NUM) {
        SET_ERROR(PIXMAP_EINVALFMT);
        return NULL;
    }

    pixmap = malloc(sizeof(*pixmap));
    pixmap->width = (uint16_t) width;
    pixmap->height = (uint16_t) height;
    pixmap->pitch = (uint8_t) pitch;
    pixmap->fmt = fmt;

    pixmap->data = malloc(Pixmap_GetSize(pixmap));

    SET_ERROR(PIXMAP_ESUCCESS)

    return pixmap;
}

Pixmap *Pixmap_New(int width, int height, int pitch, PixelFormat fmt, PixmapError *error)
{
    Pixmap *pixmap;

    if (!(pixmap = alloc_pixmap(width, height, pitch, fmt, error))) {
        return NULL;
    }

    memset(pixmap->data, 0, Pixmap_GetSize(pixmap));

    return pixmap;
}

Pixmap *Pixmap_NewFrom(const void *src, int width, int height, int pitch, PixelFormat fmt, PixmapError *error)
{
    Pixmap *pixmap;

    if (!(pixmap = alloc_pixmap(width, height, pitch, fmt, error))) {
        return NULL;
    }

    memcpy(pixmap->data, src, Pixmap_GetSize(pixmap));

    return pixmap;
}

void Pixmap_Free(Pixmap *ptr)
{
    free(ptr->data);
    free(ptr);
}

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
    return (size_t) pad_width(pixmap->width, pixmap->pitch) * pixmap->height * pf_table[pixmap->fmt].bytes;
}

const void *Pixmap_GetData(const Pixmap *pixmap)
{
    return pixmap->data;
}

void *Pixmap_GetScanline(const Pixmap *pixmap, size_t idx)
{
    return pixmap->data + idx * pad_width(pixmap->width, pixmap->pitch) * pf_table[pixmap->fmt].bytes;
}

PixelRGB8 Pixmap_GetRGB8(const Pixmap *pixmap, int x, int y)
{
    uint16_t padded_width;

    if (x < 0 || x >= pixmap->width || y < 0 || y >= pixmap->height) {
        return PixelRGB8_Black;
    }

    padded_width = pad_width(pixmap->width, pixmap->pitch);

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
