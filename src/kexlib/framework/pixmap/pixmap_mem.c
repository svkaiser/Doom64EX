
#include "pixmap_priv.h"

#define SET_ERROR(x) { if (error) *error = x; }

Pixmap *pixmap_alloc(int width, int height, int pitch, PixelFormat fmt, PixmapError *error)
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

    if (!(pixmap = pixmap_alloc(width, height, pitch, fmt, error))) {
        return NULL;
    }

    memset(pixmap->data, 0, Pixmap_GetSize(pixmap));

    return pixmap;
}

Pixmap *Pixmap_NewFrom(const void *src, int width, int height, int pitch, PixelFormat fmt, PixmapError *error)
{
    Pixmap *pixmap;

    if (!(pixmap = pixmap_alloc(width, height, pitch, fmt, error))) {
        return NULL;
    }

    memcpy(pixmap->data, src, Pixmap_GetSize(pixmap));

    return pixmap;
}

Pixmap *Pixmap_Clone(const Pixmap *src, PixmapError *error)
{
    return Pixmap_NewFrom(src->data, src->width, src->height, src->pitch, src->fmt, error);
}

void Pixmap_Raw(Pixmap *dst, const void *data, int width, int height, int pitch, PixelFormat fmt)
{
    dst->width = (uint16_t) width;
    dst->height = (uint16_t) height;
    dst->pitch = (uint8_t) pitch;
    dst->fmt = fmt;
    dst->data = (void *) data;
}

void Pixmap_Free(Pixmap *ptr)
{
    if (ptr) {
        free(ptr->data);
        free(ptr);
    }
}