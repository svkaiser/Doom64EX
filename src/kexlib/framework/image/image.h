
#ifndef DOOM64EX_IMAGE_H
#define DOOM64EX_IMAGE_H

#include <framework/image.h>

typedef K_BOOL (*image_detect_fn)(InStream *stream);
typedef K_BOOL (*image_read_header_fn)(Image *image);

struct ImageFormatFn {
    ImageFormat format;
    image_detect_fn detect;
    image_read_header_fn read_header;
};

struct image_header {
    uint32_t width;
    uint32_t height;
};

struct Image {
    ImageFormat format;

    struct image_header header;
    InStream *stream;
};

typedef struct ImageFormatFn ImageFormatFn;

extern ImageFormatFn image_format_png;

#endif //DOOM64EX_IMAGE_H
