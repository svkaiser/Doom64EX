#include <png.h>

#include "image.h"

static const char png_magic[] = "\x89PNG\x0d\x0a\x1a\x0a";

static K_BOOL _png_detect(InStream *stream);

static K_BOOL _png_read_header(Image *image);

ImageFormatFn image_format_png = {
    IF_PNG,
    _png_detect,
    _png_read_header,
};

static void _png_read_fn(png_structp ctx, png_bytep area, size_t size)
{
    InStream *stream;

    stream = png_get_io_ptr(ctx);

    ISRead(stream, area, size);
}

static K_BOOL _png_detect(InStream *stream)
{
    char check[sizeof(png_magic)];

    if (ISRead(stream, check, sizeof(check)) == sizeof(check)) {
        return memcmp(png_magic, check, sizeof(png_magic)) == 0;
    }

    return K_FALSE;
}

static K_BOOL _png_read_header(Image *image)
{
    png_structp structp;
    png_infop infop;
    int bit_depth, color_type;
    int interlace_method;
    int compression_method;
    int filter_method;

    if (!(structp = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0))) {
        fprintf(stderr, "%s: Failed to create read struct\n", __PRETTY_FUNCTION__);
        return K_FALSE;
    }

    if (!(infop = png_create_info_struct(structp))) {
        png_destroy_read_struct(&structp, NULL, NULL);
        fprintf(stderr, "%s: Failed to create info struct\n", __PRETTY_FUNCTION__);
        return K_FALSE;
    }

    if (setjmp(png_jmpbuf(structp))) {
        png_destroy_read_struct(&structp, &infop, NULL);
        fprintf(stderr, "%s: Failed to setjmp\n", __PRETTY_FUNCTION__);
        return K_FALSE;
    }

    png_set_read_fn(structp, image->stream, _png_read_fn);

    png_read_info(structp, infop);

    png_get_IHDR(structp,
                 infop,
                 &image->header.width,
                 &image->header.height,
                 &bit_depth,
                 &color_type,
                 &interlace_method,
                 &compression_method,
                 &filter_method);

    return K_TRUE;
}