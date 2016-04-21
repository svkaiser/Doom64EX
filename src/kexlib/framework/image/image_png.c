
#include "image.h"

static const char png_magic[] = "\x89PNG\x0d\x0a\x1a\x0a";
static K_BOOL png_detect(InStream *stream);
static K_BOOL png_read_header(Image *image);

ImageFormatFn image_format_png = {
    IF_PNG,
    png_detect,
    png_read_header,
};

static K_BOOL png_detect(InStream *stream)
{
    char check[sizeof(png_magic)];

    if (ISRead(stream, check, sizeof(check)) == sizeof(check)) {
        return memcmp(png_magic, check, sizeof(png_magic)) == 0;
    }

    return K_FALSE;
}

static K_BOOL png_read_header(Image *image) {
    return K_FALSE;
}