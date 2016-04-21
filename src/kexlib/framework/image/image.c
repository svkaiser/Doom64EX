
#include "image.h"

ImageFormat Image_DetectFormat(InStream *stream)
{
    if (image_format_png.detect(stream)) {
        return IF_PNG;
    }

    return IF_UNKNOWN;
}

Image *Image_Load(InStream *stream)
{
    ImageFormat format;
    Image *image;

    format = Image_DetectFormat(stream);

    if (format == IF_UNKNOWN)
        return NULL;

    image = malloc(sizeof(*image));
    image->format = format;
    image->stream = stream;

    switch (format) {
    case IF_PNG:
        image_format_png.read_header(image);
        break;

    default:
        break;
    }
}