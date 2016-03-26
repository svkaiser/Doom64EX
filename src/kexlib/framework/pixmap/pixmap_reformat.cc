
#include <framework/pixel_traits.hh>
#include "pixmap_priv.h"

template <typename From, typename To>
static void pixmap_reformat_from_to(Pixmap **pixmap, PixelFormat new_fmt)
{
    int x, y;
    Pixmap *src;
    Pixmap *dst;
    From *srcline;
    To *dstline;
    bool inplace;

    inplace = kexlib::pixel_traits<From>::bytes == kexlib::pixel_traits<To>::bytes;

    src = *pixmap;
    if (inplace) {
        dst = *pixmap;
        dst->fmt = new_fmt;
    } else {
        dst = Pixmap_New(src->width, src->height, src->pitch, kexlib::pixel_traits<To>::format, NULL);
    }

    for (y = 0; y < src->height; y++) {
        srcline = (From *) Pixmap_GetScanline(src, y);
        dstline = (To *) Pixmap_GetScanline(dst, y);

        for (x = 0; x < src->width; x++) {
            dstline[x] = kexlib::pixel_traits<To>::convert_from(srcline[x]);
        }
    }

    if (!inplace) {
        Pixmap_Free(src);
        *pixmap = dst;
    }
};

template <typename From>
static void pixmap_reformat_to(Pixmap **pixmap, PixelFormat new_fmt)
{
    switch (new_fmt) {
    case PF_RGB24:
        pixmap_reformat_from_to<From, PixelRGB24>(pixmap, new_fmt);

    case PF_BGR24:
        pixmap_reformat_from_to<From, PixelBGR24>(pixmap, new_fmt);
        break;

    case PF_RGBA32:
        pixmap_reformat_from_to<From, PixelRGBA32>(pixmap, new_fmt);
        break;

    case PF_ABGR32:
        pixmap_reformat_from_to<From, PixelABGR32>(pixmap, new_fmt);
        break;

    case PF_BGRA32:
        pixmap_reformat_from_to<From, PixelBGRA32>(pixmap, new_fmt);
        break;

    default:
        break;
    }
}

void Pixmap_Reformat_InPlace(Pixmap **pixmap, PixelFormat new_fmt)
{
    if ((*pixmap)->fmt == new_fmt) {
        // Job well done.
        return;
    }

    switch ((*pixmap)->fmt) {
    case PF_RGB24:
        pixmap_reformat_to<PixelRGB24>(pixmap, new_fmt);
        break;

    case PF_BGR24:
        pixmap_reformat_to<PixelBGR24>(pixmap, new_fmt);
        break;

    case PF_RGBA32:
        pixmap_reformat_to<PixelRGBA32>(pixmap, new_fmt);
        break;

    case PF_ABGR32:
        pixmap_reformat_to<PixelABGR32>(pixmap, new_fmt);
        break;

    case PF_BGRA32:
        pixmap_reformat_to<PixelBGRA32>(pixmap, new_fmt);
        break;

    default:
        break;
    }
}